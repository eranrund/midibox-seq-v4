// $Id: mbng_enc.h 1629 2012-12-28 16:47:12Z tk $
/*
 * Encoder access functions for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNG_ENC_H
#define _MBNG_ENC_H

#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_ENC_Init(u32 mode);

extern s32 MBNG_ENC_FastModeSet(u8 multiplier);
extern s32 MBNG_ENC_FastModeGet(void);

extern s32 MBNG_ENC_AutoSpeed(u32 enc, mbng_event_item_t *item, u32 range);

extern s32 MBNG_ENC_NotifyChange(u32 encoder, s32 incrementer);
extern s32 MBNG_ENC_NotifyReceivedValue(mbng_event_item_t *item);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_ENC_H */
