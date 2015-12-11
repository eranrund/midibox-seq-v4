/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: MbCvMidiVoice.cpp 1960 2014-02-09 20:21:24Z tk $
/*
 * MIDIbox CV MIDI Voice Handlers
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <string.h>
#include "MbCvMidiVoice.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvMidiVoice::MbCvMidiVoice()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvMidiVoice::~MbCvMidiVoice()
{
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Voice init function
/////////////////////////////////////////////////////////////////////////////
void MbCvMidiVoice::init(void)
{
    midivoiceEnabledPorts = 0x1011; // OSC1, IN1 and USB1
    midivoiceChannel = 1; // 0 disables the channel
    midivoiceSplitLower = 0x00;
    midivoiceSplitUpper = 0x7f;
    midivoiceTranspose = 0x40;
    midivoiceModWheel = 0;
    midivoicePitchBender = 0;
    midivoiceLastVoice = 0;
    midivoiceCCNumber = 16;
    midivoiceAftertouch = 0;
    midivoiceCCValue = 64;
    midivoiceNRPNValue = 8192;

    notestackReset();
}


/////////////////////////////////////////////////////////////////////////////
// returns true if Port, MIDI channel (and optionally split zone) matches
/////////////////////////////////////////////////////////////////////////////
bool MbCvMidiVoice::isAssignedPort(mios32_midi_port_t port)
{
    u8 subportMask = (1 << (port&3));
    u8 portClass = ((port-0x10) & 0xc)>>2;
    u8 portMask = subportMask << (4*portClass);

    return (midivoiceEnabledPorts & portMask);
}

bool MbCvMidiVoice::isAssigned(u8 chn)
{
    return chn == (midivoiceChannel-1);
}

bool MbCvMidiVoice::isAssigned(u8 chn, u8 note)
{
    return chn == (midivoiceChannel-1) &&
        note >= midivoiceSplitLower &&
        (!midivoiceSplitUpper || note <= midivoiceSplitUpper);
}


/////////////////////////////////////////////////////////////////////////////
// Notestack Handling
/////////////////////////////////////////////////////////////////////////////
void MbCvMidiVoice::notestackReset(void)
{
    NOTESTACK_Init(&midivoiceNotestack,
                   NOTESTACK_MODE_PUSH_TOP,
                   &midivoiceNotestackItems[0],
                   CV_SE_NOTESTACK_SIZE);
}

void MbCvMidiVoice::notestackPush(u8 note, u8 velocity)
{
    NOTESTACK_Push(&midivoiceNotestack, note, velocity);
}

s32 MbCvMidiVoice::notestackPop(u8 note)
{
    return NOTESTACK_Pop(&midivoiceNotestack, note);
}

/////////////////////////////////////////////////////////////////////////////
// maps incoming CC and NRPN
/////////////////////////////////////////////////////////////////////////////
void MbCvMidiVoice::setCC(u8 ccNumber, u8 value)
{
    // currently only a single CC value is supported, but could be more in future?
    if( ccNumber == midivoiceCCNumber )
        midivoiceCCValue = value;
}

void MbCvMidiVoice::setNRPN(u16 value)
{
    // always assigned to NRPN LSB 0, MSB depends on channel (decoded outside)
    midivoiceNRPNValue = value; // 14bit
}

/////////////////////////////////////////////////////////////////////////////
// help functions for editor
/////////////////////////////////////////////////////////////////////////////
void MbCvMidiVoice::setPortEnabled(const u8& portIx, const bool& enable)
{
    if( enable )
        midivoiceEnabledPorts |= (1 << portIx);
    else
        midivoiceEnabledPorts &= ~(1 << portIx);
}

bool MbCvMidiVoice::getPortEnabled(const u8& portIx)
{
    return (midivoiceEnabledPorts & (1 << portIx)) ? 1 : 0;
}
