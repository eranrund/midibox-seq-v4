/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: PluginProcessor.cpp 2070 2014-09-27 23:35:11Z tk $
/*
 * Audio Processing Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <mios32.h>

// for output to console (stderr)
// should be at least 1 to inform the user on fatal errors
#define DEBUG_VERBOSE_LEVEL 1


// update frequency of MBSID
#define MBSID_UPDATE_FRQ 1000


// sampling method
//#define RESID_SAMPLING_METHOD SAMPLE_RESAMPLE_INTERPOLATE
#define RESID_SAMPLING_METHOD SAMPLE_INTERPOLATE
//#define RESID_SAMPLING_METHOD SAMPLE_FAST

// SID frequency
#define RESID_FREQUENCY 1000000

// selected Model (could be variable later)
#define RESID_MODEL MOS8580

// play testtone at startup?
// nice for first checks of the emulation w/o MIDI input
#define RESID_PLAY_TESTTONE 0



// these global variables are used by ReSID
double mixer_value1;
double mixer_value2;
double mixer_value3;


//==============================================================================
MidiboxSidAudioProcessor::MidiboxSidAudioProcessor()
{
    patch = 0;
    bank = 0;
    gain = 1.0f;
    zeromem (&lastPosInfo, sizeof (lastPosInfo));
    lastPosInfo.timeSigNumerator = 4;
    lastPosInfo.timeSigDenominator = 4;
    lastPosInfo.bpm = 120;
  
    // reSID integration
    mixer_value1 = 1.0f;
    mixer_value2 = 1.0f;
    mixer_value3 = 1.0f;
    reSidSampleRate = 44100.0f;

#if SID_NUM
    reSidEnabled = 1;
    for(int i=0; i<SID_NUM; ++i) {
        reSID[i] = new SID;
        reSID[i]->set_chip_model(RESID_MODEL);
        reSID[i]->reset();
        if( !reSID[i]->set_sampling_parameters(RESID_FREQUENCY, RESID_SAMPLING_METHOD, reSidSampleRate) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
            fprintf(stderr, "Initialisation of reSID[%d] failed for unexpected reasons! All SIDs disabled now!\n", i);
#endif
            reSidEnabled = 0;
        }
    }
#else
    reSidEnabled = 0;
#if DEBUG_VERBOSE_LEVEL >= 1
    fprintf(stderr, "SID_NUM == 0: emulation disabled!\n");
#endif
#endif
  
    // MBSID integration
    // temporary code!
    mbSidUpdateCounter = 0;  

    // initialize my private SID registers
    for(int sid=0; sid<SID_NUM; ++sid)
        for(int reg=0; reg<SID_REGS_NUM; ++reg) {
            sidRegs[sid].ALL[reg] = 0;
            sidRegsShadow[sid].ALL[reg] = 0;
        }

    // trigger reset
    RESID_Update(2);
    
    // change pointer to physical SID registers
    u8 sid = 0;
    sid_regs_t *sidRegLPtr = &sidRegs[2*sid+0];
    sid_regs_t *sidRegRPtr = &sidRegs[2*sid+1];
    mbSidEnvironment.mbSid[0].init(sid, sidRegLPtr, sidRegRPtr, &mbSidEnvironment.mbSidClock);
    midiProcessing.mbSidEnvironment = &mbSidEnvironment;
}

MidiboxSidAudioProcessor::~MidiboxSidAudioProcessor()
{
#if SID_NUM
    for(int i=0; i<SID_NUM; ++i) {
        delete reSID[i];
    }
#endif
}

//==============================================================================
const String MidiboxSidAudioProcessor::getName() const
{
    return ProjectInfo::projectName;
}

int MidiboxSidAudioProcessor::getNumParameters()
{
    return 3;
}

float MidiboxSidAudioProcessor::getParameter (int index)
{
    switch( index ) {
    case 0: return gain;
    case 1: return (float)bank;
    case 2: return (float)patch;
    }
  
    return 0.0f;
}

void MidiboxSidAudioProcessor::setParameter (int index, float newValue)
{
    switch( index ) {
    case 0:
        if( gain != newValue ) {
            gain = newValue;
	
            // if this is changing the gain, broadcast a change message which
            // our editor will pick up.
            sendChangeMessage();
        }
        break;

    case 1:
        if( bank != (unsigned char)newValue ) {
            bank = (unsigned char)newValue;
            midiProcessing.sendMidiEvent(0xc0, patch, 0x00); // TODO: send bank CC
            sendChangeMessage();
        }
        break;

    case 2:
        if( patch != (unsigned char)newValue ) {
            patch = (unsigned char)newValue;
            midiProcessing.sendMidiEvent(0xc0, patch, 0x00); // TODO: send bank CC
            sendChangeMessage();
        }
        break;
    }
}

const String MidiboxSidAudioProcessor::getParameterName (int index)
{
    switch( index ) {
    case 0: return String("gain");
    case 1: return String("bank");
    case 2: return String("patch");
    }

    return String::empty;
}

const String MidiboxSidAudioProcessor::getParameterText (int index)
{
    switch( index ) {
    case 0: return String (gain, 2);
    case 1: return String ((float)bank, 2);
    case 2: return String ((float)patch, 2);
    }
  
    return String::empty;
}

const String MidiboxSidAudioProcessor::getInputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

const String MidiboxSidAudioProcessor::getOutputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

bool MidiboxSidAudioProcessor::isInputChannelStereoPair (int index) const
{
    return false;
}

bool MidiboxSidAudioProcessor::isOutputChannelStereoPair (int index) const
{
    return true;
}

bool MidiboxSidAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MidiboxSidAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MidiboxSidAudioProcessor::silenceInProducesSilenceOut() const
{
    return false;
}

double MidiboxSidAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MidiboxSidAudioProcessor::getNumPrograms()
{
    return 128;
}

int MidiboxSidAudioProcessor::getCurrentProgram()
{
    return patch;
}

void MidiboxSidAudioProcessor::setCurrentProgram (int index)
{
    patch = index;
    midiProcessing.sendMidiEvent(0xc0, patch, 0x00); // TODO: send bank CC
    sendChangeMessage();
}

const String MidiboxSidAudioProcessor::getProgramName (int index)
{
    return getPatchNameFromBank(bank, index);
}

void MidiboxSidAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void MidiboxSidAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // do your pre-playback setup stuff here..
    keyboardState.reset();
    keyboardState.addListener(&midiProcessing);

#if SID_NUM
    reSidEnabled = 1;
    reSidSampleRate = sampleRate;

    for(int i=0; i<SID_NUM; ++i) {
        reSID[i]->reset();
        if( !reSID[i]->set_sampling_parameters(RESID_FREQUENCY, RESID_SAMPLING_METHOD, reSidSampleRate) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
            fprintf(stderr, "Initialisation of reSID[%d] failed at sample rate %7.2f Hz! All SIDs disabled now!\n", i, sampleRate);
#endif
            reSidEnabled = 0;
        }

#if RESID_PLAY_TESTTONE
        if( reSidEnabled ) {
#if DEBUG_VERBOSE_LEVEL >= 2
            fprintf(stderr, "playing testtone on SID %d\n", i);
#endif
            reSID[i]->write(0x18, 0x0f); // Volume
            reSID[i]->write(0x05, 0x00); // Attack/Decay
            reSID[i]->write(0x06, 0xf8); // Sustain/Release
            reSID[i]->write(0x00, 16777 & 0xff); // Frequency Low
            reSID[i]->write(0x01, 16777 >> 8); // Frequency High
            reSID[i]->write(0x04, 0x11); // Waveform + Gate
        }
#endif
    }
#endif
}

void MidiboxSidAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void MidiboxSidAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    // poll for new MIDI events
    midiProcessing.tick();

    // have a go at getting the current time from the host, and if it's changed, tell
    // our UI to update itself.
    AudioPlayHead::CurrentPositionInfo pos;
  
    if (getPlayHead() != 0 && getPlayHead()->getCurrentPosition (pos)) {
        // send MIDI Start if position is 0
        if( lastPosInfo.ppqPosition != 0 && pos.ppqPosition == 0 )
            mbSidEnvironment.bpmRestart();

        // update BPM
        mbSidEnvironment.bpmSet((float)pos.bpm);

        // update position
        if (memcmp (&pos, &lastPosInfo, sizeof (pos)) != 0) {
            lastPosInfo = pos;
            sendChangeMessage();
        }
    } else {
        zeromem (&lastPosInfo, sizeof (lastPosInfo));
        lastPosInfo.timeSigNumerator = 4;
        lastPosInfo.timeSigDenominator = 4;
        lastPosInfo.bpm = 120;
        mbSidEnvironment.bpmSet(lastPosInfo.bpm);
        mbSidEnvironment.bpmRestart();
    }

#if RESID_PLAY_TESTTONE == 0
    // pass MIDI events to application
    midiProcessing.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);
#endif

    // determine how many channels have to be services
    int numChannels = getNumInputChannels();

#if SID_NUM
    if( reSidEnabled ) {
        if( numChannels < SID_NUM )
            numChannels = SID_NUM;
        if( numChannels > getNumOutputChannels() )
            numChannels = getNumOutputChannels();
    
        // number of samples which have to be rendered
        int numSamples = buffer.getNumSamples();
    
        // add SID sound(s) to output(s)
        // TK: this nested loop isn't optimal for CPU load, but we have to ensure that all SIDs are in lock-step
        for(int i=0; i<numSamples; ++i) {
            // update sound engine
            mbSidUpdateCounter += (double)MBSID_UPDATE_FRQ / reSidSampleRate;
            if( mbSidUpdateCounter >= 1.0 ) {
                mbSidUpdateCounter -= 1.0;
#if RESID_PLAY_TESTTONE == 0
                mbSidEnvironment.tick();
                RESID_Update(0);
#endif
            }

            for(int channel = 0; channel < numChannels; ++channel) {
                // poll for next sample
                short sample_buf;
                cycle_count delta_t = 1;
                while( !reSID[channel]->clock(delta_t, &sample_buf, 1) )
                    if( !delta_t ) // delta_t can be changed by clock()
                        delta_t = 1;

                float currentSample = (float)sample_buf / 32768.0;
                *buffer.getSampleData(channel, i) = currentSample;
            }
        }
    }
#endif

    // for each of our input channels, we'll attenuate its level by the
    // amount that our volume parameter is set to.
    for (int channel = 0; channel < numChannels; ++channel) {
        buffer.applyGain (channel, 0, buffer.getNumSamples(), gain);
    }
  
    // in case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = numChannels; i < getNumOutputChannels(); ++i) {
        buffer.clear (i, 0, buffer.getNumSamples());
    }

    // if any midi messages come in, use them to update the keyboard state object. This
    // object sends notification to the UI component about key up/down changes
    keyboardState.processNextMidiBuffer (midiMessages,
                                         0, buffer.getNumSamples(),
                                         true);
}

//==============================================================================
bool MidiboxSidAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* MidiboxSidAudioProcessor::createEditor()
{
    return new MidiboxSidAudioProcessorEditor (this);
}

//==============================================================================
void MidiboxSidAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    // create an outer XML element..
    XmlElement xmlState ("MYPLUGINSETTINGS");
  
    // add some attributes to it..
    xmlState.setAttribute ("pluginVersion", 2);
    xmlState.setAttribute ("gainLevel", gain);
    xmlState.setAttribute ("bank", bank);
    xmlState.setAttribute ("patch", patch);
    xmlState.setAttribute ("sysexMidiIn", lastSysexMidiIn);
    xmlState.setAttribute ("sysexMidiOut", lastSysexMidiOut);
  
    // you could also add as many child elements as you need to here..
  
  
    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary (xmlState, destData);
}

void MidiboxSidAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    XmlElement* const xmlState = getXmlFromBinary (data, sizeInBytes);
    
    if (xmlState != 0)
        {
            // check that it's the right type of xml..
            if (xmlState->hasTagName ("MYPLUGINSETTINGS"))
                {
                    // ok, now pull out our parameters..
                    gain = (float) xmlState->getDoubleAttribute ("gainLevel", gain);
                    bank = (float) xmlState->getDoubleAttribute ("bank", bank);
                    patch = (float) xmlState->getDoubleAttribute ("patch", patch);
      
					lastSysexMidiIn = xmlState->getStringAttribute ("sysexMidiIn", lastSysexMidiIn);
					lastSysexMidiOut = xmlState->getStringAttribute ("sysexMidiOut", lastSysexMidiOut);
      
                    sendChangeMessage();
                }
    
            delete xmlState;
        }
}


// called from editor to display current patch name
const String MidiboxSidAudioProcessor::getPatchName()
{
    char buffer[22];

    char patchName[17];
    mbSidEnvironment.mbSid[0].mbSidPatch.nameGet(patchName);
    
    sprintf(buffer, "%c%03d %s",
            'A' + bank,
            patch + 1,
            patchName);

    return String(buffer);
}

const String MidiboxSidAudioProcessor::getPatchNameFromBank(int bank, int patch)
{
    char buffer[22];

    char patchName[17];
    mbSidEnvironment.bankPatchNameGet(bank, patch, patchName);
    
    sprintf(buffer, "%c%03d %s",
            'A' + bank,
            patch + 1,
            patchName);

    return String(buffer);
}

/////////////////////////////////////////////////////////////////////////////
// Updates all RESID registers
// IN: <mode>: if 0: only register changes will be transfered to SID(s)
//             if 1: force transfer of all registers
//             if 2: reset SID, thereafter force transfer of all registers
// OUT: returns < 0 if update failed
/////////////////////////////////////////////////////////////////////////////
static const u8 update_order[] = { 
   0,  1,  2,  3,  5,  6, // voice 1 w/o osc control register
   7,  8,  9, 10, 12, 13, // voice 2 w/o osc control register
  14, 15, 16, 17, 19, 20, // voice 3 w/o osc control register
   4, 11, 18,             // voice 1/2/3 control registers
  21, 22, 23, 24,         // remaining SID registers
   // 25, 26, 27, 28, 29, 30, 31 // SwinSID registers
};

s32 MidiboxSidAudioProcessor::RESID_Update(u32 mode)
{
    // trigger reset?
    if( mode == 2 ) {
        for(int i=0; i<SID_NUM; ++i)
            reSID[i]->reset();
    }

    // check for updates
    for(int i=0; i<(int)sizeof(update_order); ++i) {
        u8 reg = update_order[i];

        for(int sid=0; sid<SID_NUM; ++sid) {
            u8 data;
            if( (data=sidRegs[sid].ALL[reg]) != sidRegsShadow[sid].ALL[reg] || mode >= 1 ) {
                sidRegsShadow[sid].ALL[reg] = data;
                reSID[sid]->write(reg, data);
            }
        }
    }

  return 0; // no error
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiboxSidAudioProcessor();
}
