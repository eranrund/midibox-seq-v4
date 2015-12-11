// $Id: app.c 1919 2014-01-08 19:13:48Z tk $
/*
 * MIOS32 Tutorial #010: Scanning up to 128 buttons connected to DINX4 modules
 * see README.txt for details
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
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define MIDI_STARTNOTE 24


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);
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
  // toggle Status LED on each incoming MIDI package
  MIOS32_BOARD_LED_Set(0x0001, ~MIOS32_BOARD_LED_Get());

  // 1) the LED should be turned on whenever a Note On Event with velocity > 0
  // has been received
  if( midi_package.type == NoteOn && midi_package.velocity > 0 ) {
    // determine pin number (add offset, so that the first LED starts at C-2)
    u8 pin = (midi_package.note - MIDI_STARTNOTE) & 0x7f;
    // turn on LED
    MIOS32_DOUT_PinSet(pin, 1);
  }

  // 2) the LED should be turned off whenever a Note Off Event or a Note On
  // event with velocity == 0 has been received (the MIDI spec says, that velocity 0
  // should be handled like Note Off)
  else if( (midi_package.type == NoteOff) ||
	   (midi_package.type == NoteOn && midi_package.velocity == 0) ) {
    // determine pin number (add offset, so that the first LED starts at C-2)
    u8 pin = (midi_package.note - MIDI_STARTNOTE) & 0x7f;
    // turn off LED
    MIOS32_DOUT_PinSet(pin, 0);
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
  // toggle Status LED on each DIN pin change
  MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

  // send MIDI event
  MIOS32_MIDI_SendNoteOn(DEFAULT, Chn1, (pin + MIDI_STARTNOTE) & 0x7f, pin_value ? 0x00 : 0x7f);
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
}
