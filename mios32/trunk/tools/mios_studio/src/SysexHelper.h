/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: SysexHelper.h 1131 2010-12-22 13:28:11Z tk $
/*
 * Sysex Help Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SYSEX_HELPER_H
#define _SYSEX_HELPER_H

#include "includes.h"


class SysexHelper
{
public:
    //==============================================================================
    SysexHelper(void);
    ~SysexHelper();

    //==============================================================================
    static bool isValidMios8Header(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios8Header(const uint8 &deviceId);
    static bool isValidMios32Header(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios32Header(const uint8 &deviceId);

    //==============================================================================
    static bool isValidMios8Acknowledge(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios8Acknowledge(const uint8 &deviceId);
    static bool isValidMios32Acknowledge(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios32Acknowledge(const uint8 &deviceId);

    //==============================================================================
    static bool isValidMios8Error(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios8Error(const uint8 &deviceId);
    static bool isValidMios32Error(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios32Error(const uint8 &deviceId);

    //==============================================================================
    static bool isValidMios8DebugMessage(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios8DebugMessage(const uint8 &deviceId);
    static bool isValidMios32DebugMessage(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios32DebugMessage(const uint8 &deviceId);

    //==============================================================================
    static bool isValidMios8WriteBlock(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMios8WriteBlock(const uint8 &deviceId, const uint32 &address, const uint8 &extension, const uint32 &size, uint8 &checksum);
    static bool isValidMios32WriteBlock(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMios32WriteBlock(const uint8 &deviceId, const uint32 &address, const uint32 &size, uint8 &checksum);

    //==============================================================================
    static bool isValidMios8UploadRequest(const uint8 *data, const uint32 &size, const int &deviceId);
    static bool isValidMios32UploadRequest(const uint8 *data, const uint32 &size, const int &deviceId);

    //==============================================================================
    static bool isValidMios32Query(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios32Query(const uint8 &deviceId);

    //==============================================================================
    static MidiMessage createMidiMessage(Array<uint8> &dataArray);
    static MidiMessage createMidiMessage(Array<uint8> &dataArray, const int &lastPos);

    //==============================================================================
    static String decodeMiosErrorCode(uint8 errorCode);

    //==============================================================================
    static bool isValidMidio128Header(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMidio128Header();
    static bool isValidMidio128ReadBlock(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMidio128ReadBlock(const uint8 &deviceId, const uint8 &block);
    static bool isValidMidio128WriteBlock(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMidio128WriteBlock(const uint8 &deviceId, const uint8 &block, const uint8 *data);
    static bool isValidMidio128Acknowledge(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMidio128Ping(const uint8 &deviceId);

    //==============================================================================
    static bool isValidMbCvHeader(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbCvHeader(const int &deviceId);
    static bool isValidMbCvReadPatch(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbCvReadPatch(const uint8 &deviceId, const uint8 &patch);
    static bool isValidMbCvWritePatch(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbCvWritePatch(const uint8 &deviceId, const uint8 &patch, const uint8 *data);
    static bool isValidMbCvAcknowledge(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbCvPing(const uint8 &deviceId);

    //==============================================================================
    static bool isValidMbhpMfHeader(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbhpMfHeader(const int &deviceId);
    static bool isValidMbhpMfReadPatch(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbhpMfReadPatch(const uint8 &deviceId, const uint8 &patch);
    static bool isValidMbhpMfWritePatch(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbhpMfWritePatch(const uint8 &deviceId, const uint8 &patch, const uint8 *data);
    static bool isValidMbhpMfWriteDirect(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbhpMfWriteDirect(const uint8 &deviceId, const uint8 &addr, const uint8 *data, const uint32 &size);
    static bool isValidMbhpMfFadersGet(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbhpMfFadersGet(const uint8 &deviceId, const uint8& startFader);
    static bool isValidMbhpMfFadersSet(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbhpMfFadersSet(const uint8 &deviceId, const uint8& startFader, const Array<uint16> mfValues);
    static bool isValidMbhpMfTraceRequest(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbhpMfTraceRequest(const uint8 &deviceId, const uint8 &traceFader, const uint8 &traceScale);
    static bool isValidMbhpMfTraceDump(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbhpMfTraceDump(const uint8 &deviceId, const Array<uint16> dump);
    static bool isValidMbhpMfAcknowledge(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMbhpMfPing(const uint8 &deviceId);


protected:
};

#endif /* _SYSEX_HELPER_H */
