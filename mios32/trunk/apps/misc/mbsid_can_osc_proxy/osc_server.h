// $Id: osc_server.h 392 2009-03-13 00:23:48Z tk $
/*
 * Header file for uIP OSC daemon/server
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _OSC_SERVER_H
#define _OSC_SERVER_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// can be overruled in mios32_config.h
#ifndef OSC_REMOTE_IP
//                      10        .    0        .    0       .    2
#define OSC_REMOTE_IP ( 10 << 24) | (  0 << 16) | (  0 << 8) | (  2 << 0)
#endif

// can be overruled in mios32_config.h
#ifndef OSC_SERVER_PORT
#define OSC_SERVER_PORT 10000
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef unsigned int uip_udp_appstate_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 OSC_SERVER_Init(u32 mode);
extern s32 OSC_SERVER_AppCall(void);
extern s32 OSC_SERVER_SendPacket(u8 *packet, u32 len);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _OSC_SERVER_H */
