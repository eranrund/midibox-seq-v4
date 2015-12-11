/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: MbCvSeq.h 1457 2012-04-04 21:16:22Z tk $
/*
 * MIDIbox CV Sequencer Prototype
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_SEQ_H
#define _MB_CV_SEQ_H

#include <mios32.h>
#include "MbCvStructs.h"


class MbCvSeq
{
public:
    // Constructor
    MbCvSeq();

    // Destructor
    ~MbCvSeq();

    // sequencer init function
    void init(void);

    // input parameters
    bool seqEnabled;
    bool seqClockReq;
    bool seqRestartReq;
    bool seqStopReq;

    u8 seqPatternNumber;
    u8 seqPatternLength;
    u8 seqResolution;
    bool seqSynchToMeasure;

    // variables
    bool seqRunning;
    u8 seqPos;
    u8 seqCurrentPattern;
    u16 seqDivCtr;

protected:
    bool seqEnabledSaved;
};

#endif /* _MB_CV_SEQ_H */
