/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: MidiKeyboard.h 945 2010-03-06 01:41:53Z tk $
/*
 * MIDI Keyboard
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIDI_KEYBOARD_H
#define _MIDI_KEYBOARD_H

#include "../includes.h"
#include "MidiSlider.h"
#include <vector>

class MiosStudio; // forward declaration

class MidiKeyboard
    : public Component
    , public MidiKeyboardStateListener
    , public SliderListener
{
public:
    //==============================================================================
    MidiKeyboard(MiosStudio *_miosStudio);
    ~MidiKeyboard();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    void handleNoteOn(MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity);
    void handleNoteOff(MidiKeyboardState *source, int midiChannel, int midiNoteNumber);

    void setMidiChannel(const int& midiChannel);
    int getMidiChannel(void);

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus);

    //==============================================================================
    void sliderValueChanged(Slider* sliderThatWasMoved);

protected:
    //==============================================================================
    std::vector<MidiSlider*> midiSlider;
    MidiKeyboardComponent *midiKeyboardComponent;
    MidiKeyboardState keyboardState;
    Label* midiChannelLabel;
    Slider* midiChannelSlider;

    //==============================================================================
    MiosStudio *miosStudio;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MidiKeyboard (const MidiKeyboard&);
    const MidiKeyboard& operator= (const MidiKeyboard&);
};

#endif /* _MIDI_KEYBOARD_H */
