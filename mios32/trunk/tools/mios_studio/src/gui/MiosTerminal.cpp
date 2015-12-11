/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: MiosTerminal.cpp 1759 2013-04-26 22:25:16Z tk $
/*
 * MIOS Terminal Component
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MiosTerminal.h"
#include "MiosStudio.h"


//==============================================================================
MiosTerminal::MiosTerminal(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
    , gotFirstMessage(0)
{
    addAndMakeVisible(terminalLogBox = new LogBox(T("MIOS Terminal")));
    terminalLogBox->addEntry(Colours::grey, T("MIOS Terminal ready."));

    addAndMakeVisible(inputLine = new CommandLineEditor());
    inputLine->setTextToShowWhenEmpty(T("(send a command to MIOS32 application)"), Colours::grey);
    inputLine->addListener(this);

    setSize(400, 200);
}

MiosTerminal::~MiosTerminal()
{
}

//==============================================================================
void MiosTerminal::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void MiosTerminal::resized()
{
    terminalLogBox->setBounds(4, 4, getWidth()-8, getHeight()-8-24-4);
    inputLine->setBounds(4, getHeight()-4-24, getWidth()-8, 24);
}

//==============================================================================
bool MiosTerminal::execCommand(const String& command)
{
    Array<uint8> dataArray = SysexHelper::createMios32DebugMessage(miosStudio->uploadHandler->getDeviceId());
    dataArray.add(0x00); // input string
    for(int i=0; i<command.length(); ++i)
        dataArray.add(command[i] & 0x7f);
    dataArray.add('\n');
    dataArray.add(0xf7);
    MidiMessage message = SysexHelper::createMidiMessage(dataArray);
    miosStudio->sendMidiMessage(message);

    inputLine->setText(String::empty);

    if( !gotFirstMessage )
        terminalLogBox->clear();
    gotFirstMessage = 1;

#if 0
    String timeStampStr = T("input");
#else
    double timeStamp = Time::getMillisecondCounter() / 1000.0; // Note: it's intented that this is the system up time
    String timeStampStr = (timeStamp > 0)
        ? String::formatted(T("%8.3f"), timeStamp)
        : T("now");
#endif
    terminalLogBox->addEntry(Colours::grey, "[" + timeStampStr + "] " + command);

    return true;
}

//==============================================================================
void MiosTerminal::textEditorTextChanged(TextEditor &editor)
{
}

void MiosTerminal::textEditorReturnKeyPressed(TextEditor &editor)
{
    if( &editor == inputLine ) {
        execCommand(inputLine->getText());
    }
}

void MiosTerminal::textEditorEscapeKeyPressed(TextEditor &editor)
{
    editor.setText(String::empty);
}

void MiosTerminal::textEditorFocusLost(TextEditor &editor)
{
}


//==============================================================================
void MiosTerminal::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    uint8 *data = (uint8 *)message.getRawData();
    uint32 size = message.getRawDataSize();
    int messageOffset = 0;

    bool messageReceived = false;
    if( runningStatus == 0xf0 &&
        SysexHelper::isValidMios32DebugMessage(data, size, -1) &&
        (data[7] == 0x40 || data[7] == 0x00) ) { // allow 0x40 (received) and 0x00 (sent) terminal message
                                                 // 0x00 is allowed for the "feedback test" which is described in the MIDI troubleshooting guide
            messageOffset = 8;
            messageReceived = true;
    }

    if( messageReceived ) {
        String str = "";

        for(int i=messageOffset; i<size; ++i) {
            if( data[i] < 0x80 ) {
                if( data[i] != '\n' || size < (i+1) )
                    str += String::formatted(T("%c"), data[i] & 0x7f);
            }
        }

        if( !gotFirstMessage )
            terminalLogBox->clear();
        gotFirstMessage = 1;

        double timeStamp = message.getTimeStamp() ? message.getTimeStamp() : ((double)Time::getMillisecondCounter() / 1000.0);
        String timeStampStr = (timeStamp > 0)
            ? String::formatted(T("%8.3f"), timeStamp)
            : T("now");

        String terminalStr = "[" + timeStampStr + "] " + str;
        if( miosStudio->runningInBatchMode() ) {
            std::cout << terminalStr << std::endl;
        } else {
            terminalLogBox->addEntry(Colours::black, terminalStr);
        }
    }
}
