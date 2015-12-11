// $Id: app.h 851 2010-01-25 20:37:27Z tk $
/*
 * Header file of application
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
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

#ifdef __cplusplus
extern "C" {
#endif

extern void APP_Init(void);
extern void APP_Background(void);
extern void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern void APP_SRIO_ServicePrepare(void);
extern void APP_SRIO_ServiceFinish(void);
extern void APP_DIN_NotifyToggle(u32 pin, u32 pin_value);
extern void APP_ENC_NotifyChange(u32 encoder, s32 incrementer);
extern void APP_AIN_NotifyChange(u32 pin, u32 pin_value);


extern void SID_TASK_Period1mS(void);
extern void SID_TASK_Period1mS_LowPrio(void);
extern void SID_TASK_Period1S(void);

extern s32 APP_StopwatchInit(void);
extern s32 APP_StopwatchReset(void);
extern s32 APP_StopwatchCapture(void);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* _APP_H */
