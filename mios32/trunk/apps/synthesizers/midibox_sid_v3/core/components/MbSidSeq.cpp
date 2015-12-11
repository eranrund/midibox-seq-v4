/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: MbSidSeq.cpp 868 2010-02-02 21:32:41Z tk $
/*
 * MIDIbox SID Sequencer Prototype
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
#include "MbSidSeq.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeq::MbSidSeq()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeq::~MbSidSeq()
{
}


/////////////////////////////////////////////////////////////////////////////
// Sequencer init function
/////////////////////////////////////////////////////////////////////////////
void MbSidSeq::init(void)
{
    seqEnabled = false;
    seqClockReq = false;
    seqRestartReq = false;
    seqStopReq = false;

    seqPatternNumber = 0;
    seqPatternLength = 16;
    seqPatternMemory = NULL;
    seqClockDivider = 3;
    seqSynchToMeasure = false;
    seqParameterAssign = 0;

    seqRunning = 0;
    seqPos = 0;
    seqDivCtr = 0;
    seqSubCtr = 0;

    seqEnabledSaved = false;
}
