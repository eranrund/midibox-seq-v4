// $Id: mios32_midi.h 1994 2014-05-18 18:43:33Z tk $
/*
 * Header file for MIDI layer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * ==========================================================================
 */

#ifndef _MIOS32_MIDI_H
#define _MIOS32_MIDI_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// the default MIDI port for MIDI output
#ifndef MIOS32_MIDI_DEFAULT_PORT
#define MIOS32_MIDI_DEFAULT_PORT USB0
#endif


// the default MIDI port for debugging output via MIOS32_MIDI_SendDebugMessage
#ifndef MIOS32_MIDI_DEBUG_PORT
#define MIOS32_MIDI_DEBUG_PORT USB0
#endif


/////////////////////////////////////////////////////////////////////////////
// Uses by MIOS32 SysEx parser
/////////////////////////////////////////////////////////////////////////////

// MIDI commands and acknowledge reply codes
#define MIOS32_MIDI_SYSEX_DEBUG    0x0d
#define MIOS32_MIDI_SYSEX_DISACK   0x0e
#define MIOS32_MIDI_SYSEX_ACK      0x0f

// disacknowledge arguments
#define MIOS32_MIDI_SYSEX_DISACK_LESS_BYTES_THAN_EXP  0x01
#define MIOS32_MIDI_SYSEX_DISACK_MORE_BYTES_THAN_EXP  0x02
#define MIOS32_MIDI_SYSEX_DISACK_WRONG_CHECKSUM       0x03
#define MIOS32_MIDI_SYSEX_DISACK_WRITE_FAILED         0x04
#define MIOS32_MIDI_SYSEX_DISACK_WRITE_ACCESS         0x05
#define MIOS32_MIDI_SYSEX_DISACK_MIDI_TIMEOUT         0x06
#define MIOS32_MIDI_SYSEX_DISACK_WRONG_DEBUG_CMD      0x07
#define MIOS32_MIDI_SYSEX_DISACK_WRONG_ADDR_RANGE     0x08
#define MIOS32_MIDI_SYSEX_DISACK_ADDR_NOT_ALIGNED     0x09
#define MIOS32_MIDI_SYSEX_DISACK_BS_NOT_AVAILABLE     0x0a
#define MIOS32_MIDI_SYSEX_DISACK_OVERRUN              0x0b
#define MIOS32_MIDI_SYSEX_DISACK_FRAME_ERROR          0x0c
#define MIOS32_MIDI_SYSEX_DISACK_UNKNOWN_QUERY        0x0d
#define MIOS32_MIDI_SYSEX_DISACK_INVALID_COMMAND      0x0e
#define MIOS32_MIDI_SYSEX_DISACK_PROG_ID_NOT_ALLOWED  0x0f
#define MIOS32_MIDI_SYSEX_DISACK_UNSUPPORTED_DEBUG    0x10


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


typedef enum {
  DEFAULT    = 0x00,
  MIDI_DEBUG = 0x01,

  USB0 = 0x10,
  USB1 = 0x11,
  USB2 = 0x12,
  USB3 = 0x13,
  USB4 = 0x14,
  USB5 = 0x15,
  USB6 = 0x16,
  USB7 = 0x17,

  UART0 = 0x20,
  UART1 = 0x21,
  UART2 = 0x22,
  UART3 = 0x23,

  IIC0 = 0x30,
  IIC1 = 0x31,
  IIC2 = 0x32,
  IIC3 = 0x33,
  IIC4 = 0x34,
  IIC5 = 0x35,
  IIC6 = 0x36,
  IIC7 = 0x37,

  OSC0 = 0x40,
  OSC1 = 0x41,
  OSC2 = 0x42,
  OSC3 = 0x43,
  OSC4 = 0x44,
  OSC5 = 0x45,
  OSC6 = 0x46,
  OSC7 = 0x47,

  SPIM0 = 0x50,
  SPIM1 = 0x51,
  SPIM2 = 0x52,
  SPIM3 = 0x53,
  SPIM4 = 0x54,
  SPIM5 = 0x55,
  SPIM6 = 0x56,
  SPIM7 = 0x57

} mios32_midi_port_t;


typedef enum {
  NoteOff       = 0x8,
  NoteOn        = 0x9,
  PolyPressure  = 0xa,
  CC            = 0xb,
  ProgramChange = 0xc,
  Aftertouch    = 0xd,
  PitchBend     = 0xe
} mios32_midi_event_t;


typedef enum {
  Chn1,
  Chn2,
  Chn3,
  Chn4,
  Chn5,
  Chn6,
  Chn7,
  Chn8,
  Chn9,
  Chn10,
  Chn11,
  Chn12,
  Chn13,
  Chn14,
  Chn15,
  Chn16
} mios32_midi_chn_t;


typedef union {
  struct {
    u32 ALL;
  };
  struct {
    u8 cin_cable;
    u8 evnt0;
    u8 evnt1;
    u8 evnt2;
  };
  struct {
    u8 type:4;
    u8 cable:4;
    u8 chn:4; // mios32_midi_chn_t
    u8 event:4; // mios32_midi_event_t
    u8 value1;
    u8 value2;
  };

  // C++ doesn't allow to redefine names in anonymous unions
  // as a simple workaround, we rename these redundant names
  struct {
    u8 cin:4;
    u8 dummy1_cable:4;
    u8 dummy1_chn:4; // mios32_midi_chn_t 
    u8 dummy1_event:4; // mios32_midi_event_t 
    u8 note:8;
    u8 velocity:8;
  };
  struct {
    u8 dummy2_cin:4;
    u8 dummy2_cable:4;
    u8 dummy2_chn:4; // mios32_midi_chn_t 
    u8 dummy2_event:4; // mios32_midi_event_t 
    u8 cc_number:8;
    u8 value:8;
  };
  struct {
    u8 dummy3_cin:4;
    u8 dummy3_cable:4;
    u8 dummy3_chn:4; // mios32_midi_chn_t 
    u8 dummy3_event:4; // mios32_midi_event_t
    u8 program_change:8;
    u8 dummy3:8;
  };
} mios32_midi_package_t;


// command states
typedef enum {
  MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN,
  MIOS32_MIDI_SYSEX_CMD_STATE_CONT,
  MIOS32_MIDI_SYSEX_CMD_STATE_END
} mios32_midi_sysex_cmd_state_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_MIDI_Init(u32 mode);

extern s32 MIOS32_MIDI_CheckAvailable(mios32_midi_port_t port);

extern s32 MIOS32_MIDI_RS_OptimisationSet(mios32_midi_port_t port, u8 enable);
extern s32 MIOS32_MIDI_RS_OptimisationGet(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_RS_Reset(mios32_midi_port_t port);

extern s32 MIOS32_MIDI_SendPackage_NonBlocking(mios32_midi_port_t port, mios32_midi_package_t package);
extern s32 MIOS32_MIDI_SendPackage(mios32_midi_port_t port, mios32_midi_package_t package);

extern s32 MIOS32_MIDI_SendEvent(mios32_midi_port_t port, u8 evnt0, u8 evnt1, u8 evnt2);
extern s32 MIOS32_MIDI_SendNoteOff(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel);
extern s32 MIOS32_MIDI_SendNoteOn(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel);
extern s32 MIOS32_MIDI_SendPolyPressure(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 val);
extern s32 MIOS32_MIDI_SendCC(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 cc_number, u8 val);
extern s32 MIOS32_MIDI_SendProgramChange(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 prg);
extern s32 MIOS32_MIDI_SendAftertouch(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 val);
extern s32 MIOS32_MIDI_SendPitchBend(mios32_midi_port_t port, mios32_midi_chn_t chn, u16 val);

extern s32 MIOS32_MIDI_SendSysEx(mios32_midi_port_t port, u8 *stream, u32 count);
extern s32 MIOS32_MIDI_SendMTC(mios32_midi_port_t port, u8 val);
extern s32 MIOS32_MIDI_SendSongPosition(mios32_midi_port_t port, u16 val);
extern s32 MIOS32_MIDI_SendSongSelect(mios32_midi_port_t port, u8 val);
extern s32 MIOS32_MIDI_SendTuneRequest(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendClock(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendTick(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendStart(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendStop(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendContinue(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendActiveSense(mios32_midi_port_t port);
extern s32 MIOS32_MIDI_SendReset(mios32_midi_port_t port);

extern s32 MIOS32_MIDI_SendDebugStringHeader(mios32_midi_port_t port, char command, char first_byte);
extern s32 MIOS32_MIDI_SendDebugStringBody(mios32_midi_port_t port, char *str_from_second_byte, u32 len);
extern s32 MIOS32_MIDI_SendDebugStringFooter(mios32_midi_port_t port);

extern s32 MIOS32_MIDI_SendDebugMessage(const char *format, ...);
extern s32 MIOS32_MIDI_SendDebugString(const char *str);
extern s32 MIOS32_MIDI_SendDebugHexDump(const u8 *src, u32 len);

extern s32 MIOS32_MIDI_ReceivePackage(mios32_midi_port_t port, mios32_midi_package_t package, void *_callback_package);
extern s32 MIOS32_MIDI_Receive_Handler(void *callback_event);

extern s32 MIOS32_MIDI_Periodic_mS(void);

extern s32 MIOS32_MIDI_DirectTxCallback_Init(s32 (*callback_tx)(mios32_midi_port_t port, mios32_midi_package_t package));
extern s32 MIOS32_MIDI_DirectRxCallback_Init(s32 (*callback_rx)(mios32_midi_port_t port, u8 midi_byte));

extern s32 MIOS32_MIDI_SendByteToRxCallback(mios32_midi_port_t port, u8 midi_byte);
extern s32 MIOS32_MIDI_SendPackageToRxCallback(mios32_midi_port_t port, mios32_midi_package_t midi_package);

extern s32 MIOS32_MIDI_DefaultPortSet(mios32_midi_port_t port);
extern mios32_midi_port_t MIOS32_MIDI_DefaultPortGet(void);

extern s32 MIOS32_MIDI_DebugPortSet(mios32_midi_port_t port);
extern mios32_midi_port_t MIOS32_MIDI_DebugPortGet(void);

extern s32 MIOS32_MIDI_DeviceIDSet(u8 device_id);
extern u8  MIOS32_MIDI_DeviceIDGet(void);

extern s32 MIOS32_MIDI_SysExCallback_Init(s32 (*callback_sysex)(mios32_midi_port_t port, u8 sysex_byte));

extern s32 MIOS32_MIDI_DebugCommandCallback_Init(s32 (*callback_debug_command)(mios32_midi_port_t port, char c));
extern s32 MIOS32_MIDI_FilebrowserCommandCallback_Init(s32 (*callback_filebrowser_command)(mios32_midi_port_t port, char c));

extern s32 MIOS32_MIDI_TimeOutCallback_Init(s32 (*callback_timeout)(mios32_midi_port_t port));


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern const u8 mios32_midi_pcktype_num_bytes[16];
extern const u8 mios32_midi_expected_bytes_common[8];
extern const u8 mios32_midi_expected_bytes_system[16];

extern const u8 mios32_midi_sysex_header[5];

#endif /* _MIOS32_MIDI_H */
