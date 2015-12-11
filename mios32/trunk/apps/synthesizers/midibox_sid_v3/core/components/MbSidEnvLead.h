/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: MbSidEnvLead.h 868 2010-02-02 21:32:41Z tk $
/*
 * MIDIbox SID Envelope Generator for Lead Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_ENV_LEAD_H
#define _MB_SID_ENV_LEAD_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidEnv.h"

class MbSidEnvLead : public MbSidEnv
{
public:
    // Constructor
    MbSidEnvLead();

    // Destructor
    ~MbSidEnvLead();

    // ENV init function
    void init(void);

    // ENV handler (returns true when sustain phase reached)
    bool tick(const u8 &updateSpeedFactor);

    // see MbSidEnv.h for input parameters

    // additional parameters for lead engine:
    s8 envDepth;
    u8 envAttackLevel;
    u8 envAttack2;
    u8 envDecayLevel;
    u8 envDecay2;
    u8 envReleaseLevel;
    u8 envRelease2;
    u8 envAttackCurve;
    u8 envDecayCurve;
    u8 envReleaseCurve;

protected:
};

#endif /* _MB_SID_ENV_LEAD_H */
