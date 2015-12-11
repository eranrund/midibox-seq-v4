// $Id: app.h 1919 2014-01-08 19:13:48Z tk $
/*
 * Header file of application
 *
 * ==========================================================================
 *
 *  Copyright (C) <year> <your name> (<your email address>)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _APP_H
#define _APP_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void APP_Init(void);
extern void APP_Background(void);
extern void APP_Tick(void);
extern void APP_MIDI_Tick(void);
extern void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
extern void APP_SRIO_ServicePrepare(void);
extern void APP_SRIO_ServiceFinish(void);
extern void APP_DIN_NotifyToggle(u32 pin, u32 pin_value);
extern void APP_ENC_NotifyChange(u32 encoder, s32 incrementer);
extern void APP_AIN_NotifyChange(u32 pin, u32 pin_value);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8 hw_enabled;

#endif /* _APP_H */
