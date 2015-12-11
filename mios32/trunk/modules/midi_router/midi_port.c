// $Id: midi_port.c 2095 2014-11-30 18:26:36Z tk $
/*
 * MIDI Port functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>
#include <osc_client.h>

#include "midi_port.h"

/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  mios32_midi_port_t port;
  char name[5];
} midi_port_entry_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// has to be kept in synch with MIDI_PORT_InIxGet() !!!
static const midi_port_entry_t in_ports[MIDI_PORT_NUM_IN_PORTS] = {
  // port ID  Name
  { DEFAULT, "Def." },
#if MIDI_PORT_NUM_IN_PORTS_USB >= 1
  { USB0,    "USB1" },
#endif
#if MIDI_PORT_NUM_IN_PORTS_USB >= 2
  { USB1,    "USB2" },
#endif
#if MIDI_PORT_NUM_IN_PORTS_USB >= 3
  { USB2,    "USB3" },
#endif
#if MIDI_PORT_NUM_IN_PORTS_USB >= 4
  { USB3,    "USB4" },
#endif
#if MIDI_PORT_NUM_IN_PORTS_UART >= 1
  { UART0,   "IN1 " },
#endif
#if MIDI_PORT_NUM_IN_PORTS_UART >= 2
  { UART1,   "IN2 " },
#endif
#if MIDI_PORT_NUM_IN_PORTS_UART >= 3
  { UART2,   "IN3 " },
#endif
#if MIDI_PORT_NUM_IN_PORTS_UART >= 4
  { UART3,   "IN4 " },
#endif
#if MIDI_PORT_NUM_IN_PORTS_OSC >= 1
  { OSC0,    "OSC1" },
#endif
#if MIDI_PORT_NUM_IN_PORTS_OSC >= 2
  { OSC1,    "OSC2" },
#endif
#if MIDI_PORT_NUM_IN_PORTS_OSC >= 3
  { OSC2,    "OSC3" },
#endif
#if MIDI_PORT_NUM_IN_PORTS_OSC >= 4
  { OSC3,    "OSC4" },
#endif
#if MIDI_PORT_NUM_IN_PORTS_SPIM >= 1
  { SPIM0,    "SPI1" },
#endif
#if MIDI_PORT_NUM_IN_PORTS_SPIM >= 2
  { SPIM1,    "SPI2" },
#endif
#if MIDI_PORT_NUM_IN_PORTS_SPIM >= 3
  { SPIM2,    "SPI3" },
#endif
#if MIDI_PORT_NUM_IN_PORTS_SPIM >= 4
  { SPIM3,    "SPI4" },
#endif
};

// has to be kept in synch with MIDI_PORT_OutIxGet() !!!
static const midi_port_entry_t out_ports[MIDI_PORT_NUM_OUT_PORTS] = {
  // port ID  Name
  { DEFAULT, "Def." },
#if MIDI_PORT_NUM_OUT_PORTS_USB >= 1
  { USB0,    "USB1" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_USB >= 2
  { USB1,    "USB2" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_USB >= 3
  { USB2,    "USB3" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_USB >= 4
  { USB3,    "USB4" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_UART >= 1
  { UART0,   "OUT1" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_UART >= 2
  { UART1,   "OUT2" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_UART >= 3
  { UART2,   "OUT3" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_UART >= 4
  { UART3,   "OUT4" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_OSC >= 1
  { OSC0,    "OSC1" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_OSC >= 2
  { OSC1,    "OSC2" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_OSC >= 3
  { OSC2,    "OSC3" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_OSC >= 4
  { OSC3,    "OSC4" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_SPIM >= 1
  { SPIM0,   "SPI1" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_SPIM >= 2
  { SPIM1,   "SPI2" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_SPIM >= 3
  { SPIM2,   "SPI3" },
#endif
#if MIDI_PORT_NUM_OUT_PORTS_SPIM >= 4
  { SPIM3,   "SPI4" },
#endif
};

// has to be kept in synch with MIDI_PORT_ClkIxGet() !!!
static const midi_port_entry_t clk_ports[MIDI_PORT_NUM_CLK_PORTS] = {
  // port ID  Name
#if MIDI_PORT_NUM_CLK_PORTS_USB >= 1
  { USB0,    "USB1" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_USB >= 2
  { USB1,    "USB2" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_USB >= 3
  { USB2,    "USB3" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_USB >= 4
  { USB3,    "USB4" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_UART >= 1
  { UART0,   "MID1" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_UART >= 2
  { UART1,   "MID2" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_UART >= 3
  { UART2,   "MID3" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_UART >= 4
  { UART3,   "MID4" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_OSC >= 1
  { OSC0,    "OSC1" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_OSC >= 2
  { OSC1,    "OSC2" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_OSC >= 3
  { OSC2,    "OSC3" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_OSC >= 4
  { OSC3,    "OSC4" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_SPIM >= 1
  { SPIM0,   "SPI1" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_SPIM >= 2
  { SPIM1,   "SPI2" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_SPIM >= 3
  { SPIM2,   "SPI3" },
#endif
#if MIDI_PORT_NUM_CLK_PORTS_SPIM >= 4
  { SPIM3,   "SPI4" },
#endif
};

// for MIDI In/Out monitor
static u8 midi_out_ctr[MIDI_PORT_NUM_OUT_PORTS];
static mios32_midi_package_t midi_out_package[MIDI_PORT_NUM_OUT_PORTS];

static u8 midi_in_ctr[MIDI_PORT_NUM_IN_PORTS];
static mios32_midi_package_t midi_in_package[MIDI_PORT_NUM_IN_PORTS];

static midi_port_mon_filter_t midi_port_mon_filter;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_PORT_Init(u32 mode)
{
  int i;

  // init MIDI In/Out monitor variables
  for(i=0; i<MIDI_PORT_NUM_OUT_PORTS; ++i) {
    midi_out_ctr[i] = 0;
    midi_out_package[i].ALL = 0;
  }

  for(i=0; i<MIDI_PORT_NUM_IN_PORTS; ++i) {
    midi_in_ctr[i] = 0;
    midi_in_package[i].ALL = 0;
  }

  midi_port_mon_filter.ALL = 0;
  midi_port_mon_filter.MIDI_CLOCK = 1;
  midi_port_mon_filter.ACTIVE_SENSE = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of available MIDI IN/OUT port
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_PORT_InNumGet(void)
{
  return MIDI_PORT_NUM_IN_PORTS;
}

s32 MIDI_PORT_OutNumGet(void)
{
  return MIDI_PORT_NUM_OUT_PORTS;
}

s32 MIDI_PORT_ClkNumGet(void)
{
  return MIDI_PORT_NUM_CLK_PORTS;
}


/////////////////////////////////////////////////////////////////////////////
// Returns name of MIDI IN/OUT port in (4 characters + zero terminator)
// port_ix is within 0..MIDI_PORT_In/OutNumGet()-1
/////////////////////////////////////////////////////////////////////////////
char *MIDI_PORT_InNameGet(u8 port_ix)
{
  if( port_ix >= MIDI_PORT_NUM_IN_PORTS )
    return "----";
  else
    return (char *)in_ports[port_ix].name;
}

char *MIDI_PORT_OutNameGet(u8 port_ix)
{
  if( port_ix >= MIDI_PORT_NUM_OUT_PORTS )
    return "----";
  else
    return (char *)out_ports[port_ix].name;
}

char *MIDI_PORT_ClkNameGet(u8 port_ix)
{
  if( port_ix >= MIDI_PORT_NUM_CLK_PORTS )
    return "----";
  else
    return (char *)clk_ports[port_ix].name;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the MIOS32 MIDI Port ID of a MIDI IN/OUT port
// port_ix is within 0..MIDI_PORT_In/OutNumGet()-1
/////////////////////////////////////////////////////////////////////////////
mios32_midi_port_t MIDI_PORT_InPortGet(u8 port_ix)
{
  if( port_ix >= MIDI_PORT_NUM_IN_PORTS )
    return 0xff; // dummy interface
  else
    return in_ports[port_ix].port;
}

mios32_midi_port_t MIDI_PORT_OutPortGet(u8 port_ix)
{
  if( port_ix >= MIDI_PORT_NUM_OUT_PORTS )
    return 0xff; // dummy interface
  else
    return out_ports[port_ix].port;
}

mios32_midi_port_t MIDI_PORT_ClkPortGet(u8 port_ix)
{
  if( port_ix >= MIDI_PORT_NUM_CLK_PORTS )
    return 0xff; // dummy interface
  else
    return clk_ports[port_ix].port;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the MBSEQ MIDI Port Index of a MIOS32 MIDI IN/OUT port
/////////////////////////////////////////////////////////////////////////////
u8 MIDI_PORT_InIxGet(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<MIDI_PORT_NUM_IN_PORTS; ++ix) {
    if( in_ports[ix].port == port )
      return ix;
  }

  return 0; // return first ix if not found
}

u8 MIDI_PORT_OutIxGet(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<MIDI_PORT_NUM_OUT_PORTS; ++ix) {
    if( out_ports[ix].port == port )
      return ix;
  }

  return 0; // return first ix if not found
}

u8 MIDI_PORT_ClkIxGet(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<MIDI_PORT_NUM_CLK_PORTS; ++ix) {
    if( clk_ports[ix].port == port )
      return ix;
  }

  return 0; // return first ix if not found
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if MIOS32 MIDI In/Out Port is available
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_PORT_InCheckAvailable(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<MIDI_PORT_NUM_IN_PORTS; ++ix) {
    if( in_ports[ix].port == port ) {
      if( port >= 0xf0 )
	return 1; // Bus is always available
      else if( (port & 0xf0) == OSC0 )
	return 1; // TODO: check for ethernet connection here
      else
	return MIOS32_MIDI_CheckAvailable(port);
    }
  }
  return 0; // port not available
}

s32 MIDI_PORT_OutCheckAvailable(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<MIDI_PORT_NUM_OUT_PORTS; ++ix) {
    if( out_ports[ix].port == port ) {
      if( port == 0x80 ) {
	//return SEQ_CV_IfGet() ? 1 : 0;
	return 0;
      } else if ( port >= 0xf0 )
	return 1; // Bus is always available
      else if( (port & 0xf0) == OSC0 )
	return 1; // TODO: check for ethernet connection here
      else
	return MIOS32_MIDI_CheckAvailable(port);
    }
  }
  return 0; // port not available
}

s32 MIDI_PORT_ClkCheckAvailable(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<MIDI_PORT_NUM_CLK_PORTS; ++ix) {
    if( clk_ports[ix].port == port ) {
      if( port == 0x80 ) {
	//return SEQ_CV_IfGet() ? 1 : 0;
	return 0;
      } else if ( port >= 0xf0 )
	return 1; // Bus is always available
      else if( (port & 0xf0) == OSC0 )
	return 1; // TODO: check for ethernet connection here
      else
	return MIOS32_MIDI_CheckAvailable(port);
    }
  }
  return 0; // port not available
}

/////////////////////////////////////////////////////////////////////////////
// Returns the last sent MIDI package of given output port
/////////////////////////////////////////////////////////////////////////////
mios32_midi_package_t MIDI_PORT_OutPackageGet(mios32_midi_port_t port)
{
  u8 port_ix = 0;
  mios32_midi_package_t empty;
  empty.ALL = 0;

  if( port != DEFAULT ) {
    port_ix = MIDI_PORT_OutIxGet(port);

    if( !port_ix )
      return empty; // port not supported...
  }

  if( !midi_out_ctr[port_ix] )
    return empty; // package expired

  return midi_out_package[port_ix];
}


/////////////////////////////////////////////////////////////////////////////
// Returns the last received MIDI package of given input port
/////////////////////////////////////////////////////////////////////////////
mios32_midi_package_t MIDI_PORT_InPackageGet(mios32_midi_port_t port)
{
  u8 port_ix = 0;
  mios32_midi_package_t empty;
  empty.ALL = 0;

  if( port != DEFAULT ) {
    port_ix = MIDI_PORT_InIxGet(port);

    if( !port_ix )
      return empty; // port not supported...
  }

  if( !midi_in_ctr[port_ix] )
    return empty; // package expired

  return midi_in_package[port_ix];
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set MIDI monitor filter option
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_PORT_MonFilterSet(midi_port_mon_filter_t filter)
{
  midi_port_mon_filter = filter;
  return 0; // no error
}

midi_port_mon_filter_t MIDI_PORT_MonFilterGet(void)
{
  return midi_port_mon_filter;
}


/////////////////////////////////////////////////////////////////////////////
// Handles the MIDI In/Out counters
// Should be called periodically each mS
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_PORT_Period1mS(void)
{
  int i;
  static u8 predivider = 0;

  // counters are handled each 10 mS
  if( ++predivider >= 10 ) {
    predivider = 0;

    for(i=0; i<MIDI_PORT_NUM_OUT_PORTS; ++i)
      if( midi_out_ctr[i] )
	--midi_out_ctr[i];

    for(i=0; i<MIDI_PORT_NUM_IN_PORTS; ++i)
      if( midi_in_ctr[i] )
	--midi_in_ctr[i];
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Called by MIDI Tx Notificaton hook if a MIDI event should be sent
// Allows to provide additional MIDI ports
// If 1 is returned, package will be filtered!
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_PORT_NotifyMIDITx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // MIDI Out monitor function
  u8 mon_filtered = 0;
  if( midi_port_mon_filter.MIDI_CLOCK && package.evnt0 == 0xf8 )
    mon_filtered = 1;
  else if( midi_port_mon_filter.ACTIVE_SENSE && package.evnt0 == 0xfe )
    mon_filtered = 1;

  if( !mon_filtered ) {
    int port_ix = -1;
    if( port != DEFAULT ) {
      port_ix = MIDI_PORT_OutIxGet(port);

      if( !port_ix )
	port_ix = -1; // port not mapped
    }

    if( port_ix >= 0 ) {
      midi_out_package[port_ix] = package;
      midi_out_ctr[port_ix] = 20; // 2 seconds lifetime
    }
  }


  // DIN Sync Event (0xf9 sent over port 0xff)
  if( port == 0xff && package.evnt0 == 0xf9 ) {
    //seq_core_din_sync_pulse_ctr = 2 + SEQ_CV_ClkPulseWidthGet(); // to generate a pulse with configurable length (+1 for 1->0 transition, +1 to compensate jitter)
    return 1; // filter package
  }

  if( (port & 0xf0) == OSC0 ) { // OSC1..4 port
    if( OSC_CLIENT_SendMIDIEvent(port & 0x0f, package) >= 0 )
      return 1; // filter package
  }

  return 0; // don't filter package
}


/////////////////////////////////////////////////////////////////////////////
// Called by MIDI Rx Notificaton hook in app.c if a MIDI event is received
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_PORT_NotifyMIDIRx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // MIDI In monitor function
  u8 mon_filtered = 0;
  if( midi_port_mon_filter.MIDI_CLOCK && package.evnt0 == 0xf8 )
    mon_filtered = 1;
  else if( midi_port_mon_filter.ACTIVE_SENSE && package.evnt0 == 0xfe )
    mon_filtered = 1;

  if( !mon_filtered ) {
    int port_ix = -1;
    if( port != DEFAULT ) {
      port_ix = MIDI_PORT_InIxGet(port);

      if( !port_ix )
	port_ix = -1; // port not mapped
    }

    if( port_ix >= 0 ) {
      midi_in_package[port_ix] = package;
      midi_in_ctr[port_ix] = 20; // 2 seconds lifetime
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the name of an event in a string
// num_chars: currently only 5 supported, long event names could be added later
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_PORT_EventNameGet(mios32_midi_package_t package, char *label, u8 num_chars)
{
  // currently only 5 chars supported...
  if( package.type == 0xf || package.evnt0 >= 0xf8 ) {
    switch( package.evnt0 ) {
    case 0xf8: strcpy(label, " CLK "); break;
    case 0xfa: strcpy(label, "START"); break;
    case 0xfb: strcpy(label, "CONT."); break;
    case 0xfc: strcpy(label, "STOP "); break;
    default:
      sprintf(label, " %02X  ", package.evnt0);
    }
  } else if( package.type < 8 ) {
    strcpy(label, "SysEx");
  } else {
    switch( package.event ) {
      case NoteOff:
      case NoteOn:
      case PolyPressure:
      case CC:
      case ProgramChange:
      case Aftertouch:
      case PitchBend:
	// could be enhanced later
        sprintf(label, "%02X%02X ", package.evnt0, package.evnt1);
	break;

      default:
	// print first two bytes for unsupported events
        sprintf(label, "%02X%02X ", package.evnt0, package.evnt1);
    }
  }

#if 0
  // TODO: enhanced messages
  if( num_chars > 5 ) {
  }
#endif

  return 0; // no error
}
