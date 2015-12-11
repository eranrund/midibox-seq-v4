// $Id: app.c 1919 2014-01-08 19:13:48Z tk $
/*
 * MIOS32 Tutorial #013: Controlling Motorfaders
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // configure motorfaders
  // see http://www.ucapps.de/mbhp_mf.html for details about the parameters
  int mf;
  for(mf=1; mf<MIOS32_MF_NUM; ++mf) {
    mios32_mf_config_t mf_config = MIOS32_MF_ConfigGet(mf);
    mf_config.cfg.deadband = MIOS32_AIN_DEADBAND;
    mf_config.cfg.pwm_period = 3;
    mf_config.cfg.pwm_duty_cycle_up = 1;
    mf_config.cfg.pwm_duty_cycle_down = 1;
    MIOS32_MF_ConfigSet(mf, mf_config);
  }
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the main task which also handles DIN, ENC
// and AIN events. You could add more jobs here, but they shouldn't consume
// more than 300 uS to ensure the responsiveness of buttons, encoders, pots.
// Alternatively you could create a dedicated task for application specific
// jobs as explained in $MIOS32_PATH/apps/tutorials/006_rtos_tasks
/////////////////////////////////////////////////////////////////////////////
void APP_Tick(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_Tick(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // move motorfader on incoming pitchbend events
  if( midi_package.type == PitchBend ) {
    int mf = midi_package.chn;
    u16 pitchbend_value_14bit = (midi_package.evnt2 << 7) | (midi_package.evnt1 & 0x7f);
    int mf_pos_12bit = pitchbend_value_14bit >> 2;
    MIOS32_MF_FaderMove(mf, mf_pos_12bit);
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  // toggle Status LED on each AIN value change
  MIOS32_BOARD_LED_Set(0x0001, ~MIOS32_BOARD_LED_Get());

  // convert 12bit value to 14bit value
  u16 value_14bit = pin_value << 2;

  // send Pitch Bender event
  MIOS32_MIDI_SendPitchBend(DEFAULT, pin, value_14bit);

  // if first fader has been moved, move the remaining faders to the same position
  if( pin == 0 ) {
    int mf;
    for(mf=1; mf<MIOS32_MF_NUM; ++mf)
      MIOS32_MF_FaderMove(mf, pin_value);
  }
}
