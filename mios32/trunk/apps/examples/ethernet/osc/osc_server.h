// $Id: osc_server.h 817 2010-01-09 22:57:32Z tk $
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
//                     192        .  168        .    2       .    1
#define OSC_REMOTE_IP (192 << 24) | (168 << 16) | (  2 << 8) | (  1 << 0)
#endif

// can be overruled in mios32_config.h
#ifndef OSC_SERVER_PORT
#define OSC_SERVER_PORT 10000
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// clashes with dhcpc.h
// not used by OSC server anyhow
//typedef unsigned int uip_udp_appstate_t;


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
