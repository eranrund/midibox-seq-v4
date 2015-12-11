/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: MidiProcessing.h 2070 2014-09-27 23:35:11Z tk $
/*
 * MIDI processing routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIDI_PROCESSING_H
#define _MIDI_PROCESSING_H

#include "MbSidEnvironment.h"
#include <queue>

class MidiProcessing : public MidiKeyboardStateListener, public MidiInputCallback
{
public:

    MidiProcessing();
    ~MidiProcessing();

    MbSidEnvironment *mbSidEnvironment;

    // stores MIDI In/Out
	AudioDeviceManager audioDeviceManager;

    // for MidiKeyboardStateListener
    void handleNoteOn (MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity);
    void handleNoteOff (MidiKeyboardState *source, int midiChannel, int midiNoteNumber);
    void processNextMidiBuffer (MidiBuffer& buffer,
                                const int startSample,
                                const int numSamples,
                                const bool injectIndirectEvents);


    // for MidiInputCallback
    void tick(void);
    void processNextMidiEvent (const MidiMessage& message);
    void handleIncomingMidiMessage (MidiInput* source,
                                    const MidiMessage& message);


    // bridge to MIOS32
    void sendMidiEvent(unsigned char evnt0, unsigned char evnt1, unsigned char evnt2);

    // TK: the Juce specific "MidiBuffer" sporatically throws an assertion when overloaded
    // therefore I'm using a std::queue instead
    std::queue<MidiMessage> midiInQueue;

    // stores the running status of incoming events
    unsigned char runningStatus;
};

#endif /* _MIDI_PROCESSING_H */

