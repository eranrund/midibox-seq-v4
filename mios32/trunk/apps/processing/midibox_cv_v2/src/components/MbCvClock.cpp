/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: MbCvClock.cpp 1895 2013-12-20 21:45:42Z tk $
/*
 * MIDIbox CV Clock Generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbCvClock.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvClock::MbCvClock()
{
    updateSpeedFactor = 2;

    // set default clock mode
    clockModeSet(MBCV_CLOCK_MODE_AUTO);

    // set default BPM rate
    bpmCtr = 0;
    bpmSet(120.0);

    clockSlaveMode = false;

    midiStartReq = false;
    midiContinueReq = false;
    midiStopReq = false;

    clkTickCtr = 0;

    incomingClkCtr = 0;
    incomingClkDelay = 0;
    sentClkDelay = 0;
    sentClkCtr = 0;
    clkReqCtr = 0;

    for(int i=0; i<externalClockDivider.size; ++i)
        externalClockDivider[i] = 4;
    for(int i=0; i<externalClockPulseWidth.size; ++i)
        externalClockPulseWidth[i] = 2; // mS
    for(int i=0; i<externalClockPulseWidthCounter.size; ++i)
        externalClockPulseWidthCounter[i] = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvClock::~MbCvClock()
{
}


/////////////////////////////////////////////////////////////////////////////
// should be called peridocally by updateSe
/////////////////////////////////////////////////////////////////////////////
void MbCvClock::tick(void)
{
    // clear previous clock events
    eventClock = false;
    eventStart = false;
    eventStop = false;

    // increment the clock counter, used to measure the delay between two F8 events
    // see also CV_SE_IncomingClk()
    if( incomingClkCtr != 0xffff )
        ++incomingClkCtr;

    // now determine master/slave flag depending on ensemble setup
    switch( clockMode ) {
    case MBCV_CLOCK_MODE_MASTER:
        clockSlaveMode = false;
        break;

    case MBCV_CLOCK_MODE_SLAVE:
        clockSlaveMode = true;
        break;

    default: // MBCV_CLOCK_MODE_AUTO
        // slave mode as long as incoming clock counter < 0xfff
        clockSlaveMode = incomingClkCtr < 0xfff;
        break;
    }

    // only used in slave mode:
    // decrement sent clock delay, send interpolated clock events 3 times
    if( !--sentClkDelay && sentClkCtr < 3 ) {
        ++sentClkCtr;
        ++clkReqCtr;
        sentClkDelay = incomingClkDelay >> 2; // to generate clock 4 times
    }

    // handle MIDI Start Event
    if( midiStartReq ) {
        // take over
        midiStartReq = false;
        eventStart = true;
        isRunning = true;

        // reset clock counter
        clkTickCtr = 0;
    }

    // handle MIDI Stop Event
    if( midiStopReq ) {
        // take over
        midiStopReq = false;
        eventStop = true;
        isRunning = false;
    }

    // handle MIDI continue event
    if( midiContinueReq ) {
        // take over
        midiContinueReq = false;
        eventContinue = true;
        isRunning = true;
    }

    // handle MIDI Clock Event
    if( clkReqCtr ) {
        // decrement counter by one (if there are more requests, they will be handled on next handler invocation)
        --clkReqCtr;

        if( clockSlaveMode )
            eventClock = 1;
    } else {
        if( !clockSlaveMode ) {
            // TODO: check timer overrun flag
            bpmCtr += 1000000;
            if( bpmCtr > bpmCtrMod ) {
                bpmCtr -= bpmCtrMod;
                eventClock = 1;
            }
        }
    }

    if( eventClock ) {
        ++clkTickCtr;

        u8 *externalClockDividerPtr = externalClockDivider.first();
        for(int clk=0; clk < externalClockDivider.size; ++clk, ++externalClockDividerPtr) {
            if( (clkTickCtr % *externalClockDividerPtr) == 0 ) {
                externalClockPulseWidthCounter[clk] = externalClockPulseWidth[clk] * (updateSpeedFactor / 2);
            }
        }
    }

    // service external clocks
    if( !isRunning ) {
        externalClocks = 0;
    } else {
        u16 *externalClockPulseWidthCounterPtr = externalClockPulseWidthCounter.first();
        u8 mask = 1;
        for(int clk=0; clk < externalClockPulseWidthCounter.size; ++clk, ++externalClockPulseWidthCounterPtr, mask <<= 1) {
            if( *externalClockPulseWidthCounterPtr > 0 ) {
                *externalClockPulseWidthCounterPtr -= 1;

                if( *externalClockPulseWidthCounterPtr ) {
                    externalClocks |= mask;
                } else {
                    externalClocks &= ~mask;
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// receives MIDI realtime events for external clocking
/////////////////////////////////////////////////////////////////////////////
void MbCvClock::midiReceiveRealTimeEvent(mios32_midi_port_t port, u8 midi_in)
{
    // TMP:
    // we filter out USB2..15 to ensure that USB interface won't clock multiple times
    if( (port & 0xf0) == USB0 && (port & 0x0f) != 0 )
        return;

    // ensure that following operations are atomic
    MIOS32_IRQ_Disable();

    switch( midi_in ) {
    case 0xf8: // MIDI Clock
        // we've measure a new delay between two F8 events
        incomingClkDelay = incomingClkCtr;
        incomingClkCtr = 0;

        // increment clock counter by 4
        clkReqCtr += 4 - sentClkCtr;
        sentClkCtr = 0;

        // determine new clock delay
        sentClkDelay = incomingClkDelay >> 2;
        break;

    case 0xfa: // MIDI Clock Start
        midiStartReq = true;

        // Auto Mode: immediately switch to slave mode
        // TODO

        // ensure that incoming clock counter < 0xfff (means: no clock received for long time)
        if( incomingClkCtr > 0xfff )
            incomingClkCtr = 0;

        // cancel all requested clocks
        clkReqCtr = 0;
        sentClkCtr = 3;
        break;

    case 0xfb: // MIDI Clock Continue
        midiContinueReq = true;
        break;

    case 0xfc: // MIDI Clock Stop
        midiStopReq = true;
        break;
    }

    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// set/get BPM
/////////////////////////////////////////////////////////////////////////////
void MbCvClock::bpmSet(float bpm)
{
    // ensure that BPM doesn't lead to integer overflow
    if( bpm < 1 )
        bpm = 120.0;
    else if( bpm > 300 )
        bpm = 300.0;

    // how many CV SE ticks for next 96ppqn event?
    // result is fixed point arithmetic * 10000000 (for higher accuracy)
    bpmCtrMod = (u32)(((60.0/bpm) / (2E-3 / (float)updateSpeedFactor)) * (1000000.0/96.0));

    // TODO: not in slave mode
    effectiveBpm = bpm;
}

float MbCvClock::bpmGet(void)
{
    return effectiveBpm;
}


/////////////////////////////////////////////////////////////////////////////
// resets internal Tempo generator
/////////////////////////////////////////////////////////////////////////////
void MbCvClock::bpmRestart(void)
{
    midiStartReq = true;
}


/////////////////////////////////////////////////////////////////////////////
// set/get clock mode
/////////////////////////////////////////////////////////////////////////////
void MbCvClock::clockModeSet(mbcv_clock_mode_t mode)
{
    if( mode < 3 )
        clockMode = mode;
}

mbcv_clock_mode_t MbCvClock::clockModeGet(void)
{
    return clockMode;
}

