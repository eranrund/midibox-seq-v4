/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: MbCvEnvBase.cpp 1960 2014-02-09 20:21:24Z tk $
/*
 * MIDIbox CV Base Class for Envelope Generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbCvEnvBase.h"
#include "MbCvTables.h"
#include "CapChargeCurve.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvEnvBase::MbCvEnvBase()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvEnvBase::~MbCvEnvBase()
{
}



/////////////////////////////////////////////////////////////////////////////
// ENV init function
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvBase::init(void)
{
    // clear flags
    restartReq = false;
    releaseReq = false;
    syncClockReq = false;

    // clear variables
    envModeClkSync = 0;
    envModeKeySync = 1;
    envModeOneshot = 1;
    envModeFast = 0;

    envAmplitude = 64;
    envCurvePos = 0;
    envCurveNeg = 0;

    envDepthPitch = 0;
    envDepthLfo1Amplitude = 0;
    envDepthLfo1Rate = 0;
    envDepthLfo2Amplitude = 0;
    envDepthLfo2Rate = 0;

    envAmplitudeModulation = 0;
    envRateModulation = 0;

    envOut = 0;

    envState = MBCV_ENV_STATE_IDLE;
    envCtr = 0;
    envWaveOut = 0;
    envDelayCtr = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Envelope handler
/////////////////////////////////////////////////////////////////////////////
bool MbCvEnvBase::tick(const u8 &updateSpeedFactor)
{
    envOut = 0;

    return false;
}



/////////////////////////////////////////////////////////////////////////////
// Calculates a step
/////////////////////////////////////////////////////////////////////////////
bool MbCvEnvBase::step(const u16& startValue, const u16& targetValue, const u16& incrementer, const bool& constantDelay)
{
    bool curveInverted = targetValue < startValue;

    if( !constantDelay &&
        ((!curveInverted && startValue >= targetValue) || ( curveInverted && startValue <= targetValue)) ) {
        envCtr = 0xffff; // next stage
    } else {
        s32 newCtr = (s32)envCtr + incrementer;
        if( newCtr < 0xffff )
            envCtr = newCtr;
        else
            envCtr = 0xffff; // next stage
    }


    // Waveshape depending on envCurve
    u16 curveValue = envCtr; // MBCV_ENV_CURVE_LINEAR and other unimplemented
    u8 curve = curveInverted ? envCurveNeg : envCurvePos;
    switch( curve ) {
    case MBCV_ENV_CURVE_EXP1:
        curveValue = capChargeCurve1[curveValue / (65536 / CAP_CHARGE_CURVE_STEPS)];
        break;
    case MBCV_ENV_CURVE_EXP1_INV:
        curveValue = 65535 - capChargeCurve1[(65535 - curveValue) / (65536 / CAP_CHARGE_CURVE_STEPS)];
        break;
    case MBCV_ENV_CURVE_EXP2:
        curveValue = capChargeCurve2[curveValue / (65536 / CAP_CHARGE_CURVE_STEPS)];
        break;
    case MBCV_ENV_CURVE_EXP2_INV:
        curveValue = 65535 - capChargeCurve2[(65535 - curveValue) / (65536 / CAP_CHARGE_CURVE_STEPS)];
        break;
    }

    // scale over range
    if( curveInverted ) {
        u32 scaledValue = ((startValue - targetValue + 1) * (u32)(65535 - curveValue)) >> 16;
        envWaveOut = targetValue + scaledValue;
    } else {
        u32 scaledValue = ((targetValue - startValue + 1) * (u32)curveValue) >> 16;
        envWaveOut = startValue + scaledValue;
    }

    // next stage?
    if( envCtr == 0xffff ) {
        envCtr = 0;
        return true;
    }

    return false; // continue with current stage
}
