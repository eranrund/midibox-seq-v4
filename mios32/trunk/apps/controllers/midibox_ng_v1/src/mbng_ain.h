// $Id: mbng_ain.h 2015 2014-06-02 20:43:08Z tk $
/*
 * AIN access functions for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNG_AIN_H
#define _MBNG_AIN_H

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

extern s32 MBNG_AIN_Init(u32 mode);

extern s32 MBNG_AIN_HandleCalibration(u16 pin_value, u16 min, u16 max, u16 ain_max, u8 spread_center);
extern s32 MBNG_AIN_HandleAinMode(mbng_event_item_t *item, u16 pin_value, u16 prev_pin_value);

extern s32 MBNG_AIN_NotifyChange(u32 pin, u32 pin_value, u8 no_midi);
extern s32 MBNG_AIN_Periodic(void);
extern s32 MBNG_AIN_NotifyReceivedValue(mbng_event_item_t *item);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_AIN_H */
