// $Id: app.c 674 2009-07-29 19:54:48Z tk $
/*
 * Demo application for T6963C GLCD with horizontal orientation
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
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

#include <glcd_font.h>


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  MIOS32_BOARD_LED_Init(0xffffffff); // initialize all LEDs
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // print static screen
  MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);

  // clear LCD
  MIOS32_LCD_Clear();

  // print text
  MIOS32_LCD_GCursorSet(4, 2*8);
  MIOS32_LCD_PrintString("            T6963C horizontal           ");

  MIOS32_LCD_GCursorSet(4, 3*8);
  MIOS32_LCD_PrintString("      ---> low performance!!! <---      ");

  MIOS32_LCD_GCursorSet(30, 5*8);
  MIOS32_LCD_PrintString(" powered by  ");

  MIOS32_LCD_FontInit((u8 *)GLCD_FONT_BIG);
  MIOS32_LCD_GCursorSet(110, 4*8);
  MIOS32_LCD_PrintString("MIOS32");

  // endless loop: print animations
  u32 loop_ctr = 0;
  u8 dir = 1;
  u8 knob_icon_ctr[4] = {0, 3, 6, 9}; // memo: 12 icons
  u8 knob_icon_delay_ctr[4] = {0, 2, 4, 6};
  const u8 knob_icon_x[4] = {0, 212, 0, 212}; // memo: icon width 28
  const u8 knob_icon_y[4] = {0, 0, 40, 40}; // memo: icon height 24

  u8 vmeter_icon_ctr[2] = {0, 5}; // memo: 28 icons (14 used)
  u8 vmeter_icon_dir[2] = {1, 1};
  u8 vmeter_icon_delay_ctr[2] = {1, 4};
  const u8 vmeter_icon_x[2] = {28, 204}; // memo: icon width 8
  const u8 vmeter_icon_y[2] = {16, 16}; // memo: icon height 32

  u8 hmeter_icon_ctr[2] = {6, 11}; // memo: 28 icons (14 used)
  u8 hmeter_icon_dir[2] = {1, 0};
  u8 hmeter_icon_delay_ctr[2] = {4, 2};
  const u8 hmeter_icon_x[2] = {106, 106}; // memo: icon width 28
  const u8 hmeter_icon_y[2] = {0, 56}; // memo: icon height 8

  while( 1 ) {
    s32 i;

    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    // switch direction each 10000 cycles
    if( dir ) {
      if( ++loop_ctr > 10000 ) {
	loop_ctr = 0;
	dir = 0;
      }
    } else {
      if( ++loop_ctr > 10000 ) {
	loop_ctr = 0;
	dir = 1;
      }
    }

    // print turning Knob icons at all edges
    MIOS32_LCD_FontInit((u8 *)GLCD_FONT_KNOB_ICONS); // memo: 12 icons, icon size: 28x24
    for(i=0; i<4; ++i) {
      if( ++knob_icon_delay_ctr[i] > 10 ) {
	knob_icon_delay_ctr[i] = 0;
	if( ++knob_icon_ctr[i] >= 12 )
	  knob_icon_ctr[i] = 0;
      }
      MIOS32_LCD_GCursorSet(knob_icon_x[i], knob_icon_y[i]);
      MIOS32_LCD_PrintChar(knob_icon_ctr[i]);
    }

    // print vmeter icons
    MIOS32_LCD_FontInit((u8 *)GLCD_FONT_METER_ICONS_V); // memo: 28 icons, 14 used, icon size: 8x32
    for(i=0; i<2; ++i) {
      if( ++vmeter_icon_delay_ctr[i] > 5 ) {
	vmeter_icon_delay_ctr[i] = 0;
	if( vmeter_icon_dir[i] ) {
	  if( ++vmeter_icon_ctr[i] >= 13 )
	    vmeter_icon_dir[i] = 0;
	} else {
	  if( --vmeter_icon_ctr[i] < 1 )
	    vmeter_icon_dir[i] = 1;
	}
      }
      MIOS32_LCD_GCursorSet(vmeter_icon_x[i], vmeter_icon_y[i]);
      MIOS32_LCD_PrintChar(vmeter_icon_ctr[i]);
    }

    // print hmeter icons
    MIOS32_LCD_FontInit((u8 *)GLCD_FONT_METER_ICONS_H); // memo: 28 icons, 14 used, icon size: 28x8
    for(i=0; i<2; ++i) {
      if( ++hmeter_icon_delay_ctr[i] > 7 ) {
	hmeter_icon_delay_ctr[i] = 0;
	if( hmeter_icon_dir[i] ) {
	  if( ++hmeter_icon_ctr[i] >= 13 )
	    hmeter_icon_dir[i] = 0;
	} else {
	  if( --hmeter_icon_ctr[i] < 1 )
	    hmeter_icon_dir[i] = 1;
	}
      }
      MIOS32_LCD_GCursorSet(hmeter_icon_x[i], hmeter_icon_y[i]);
      MIOS32_LCD_PrintChar(hmeter_icon_ctr[i]);
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
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
}
