// $Id: app.c 1843 2013-11-01 20:54:02Z tk $
/*
 * Demo application for 8xSSD1306 OLEDs
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
#define MAX_LCDS 16
  int num_lcds = mios32_lcd_parameters.num_x * mios32_lcd_parameters.num_y;
  if( num_lcds > MAX_LCDS ) {
    MIOS32_MIDI_SendDebugMessage("WARNING: this application only supports up to 16 displays!\n");
    num_lcds = MAX_LCDS;
  }

  // clear LCDs
  {
    u8 n;
    for(n=0; n<num_lcds; ++n) {
      MIOS32_LCD_DeviceSet(n);
      MIOS32_LCD_Clear();
    }
  }

  u8 vmeter_icon_ctr[MAX_LCDS][2] = {{0,5},{3,14},{7,1},{3,9},{13,6},{10,2},{1,4},{6,2},{13,6},{10,2},{1,4},{6,2},{1,2},{13,14},{5,5},{6,1}}; // memo: 28 icons (14 used)
  u8 vmeter_icon_dir[MAX_LCDS][2] = {{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1}};
  u8 vmeter_icon_delay_ctr[MAX_LCDS][2] = {{1,4},{1,4},{1,4},{1,4},{1,4},{1,4},{1,4},{1,4},{1,4},{1,4},{1,4},{1,4},{1,4},{1,4},{1,4},{1,4}};
  const u8 vmeter_icon_x[2] = {0, 120}; // memo: icon width 8
  const u8 vmeter_icon_y[2] = {12, 12}; // memo: icon height 32

  u8 hmeter_icon_ctr[MAX_LCDS][2] = {{6,11},{2,27},{23,1},{15,6},{18,9},{10,12},{3,25},{26,7},{18,9},{10,12},{3,25},{26,7},{6,9},{18,18},{20,10},{3,10}}; // memo: 28 icons (14 used)
  u8 hmeter_icon_dir[MAX_LCDS][2] = {{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0}};
  u8 hmeter_icon_delay_ctr[MAX_LCDS][2] = {{4,2},{4,2},{4,2},{4,2},{4,2},{4,2},{4,2},{4,2},{4,2},{4,2},{4,2},{4,2},{4,2},{4,2},{4,2},{4,2}};
  const u8 hmeter_icon_x[2] = {20, 80}; // memo: icon width 28
  const u8 hmeter_icon_y[2] = {60, 60}; // memo: icon height 8

  // print configured LCD parameters
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("SSD1306 Demo started.");
  MIOS32_MIDI_SendDebugMessage("Configured LCD Parameters in MIOS32 Bootloader Info Range:\n");
  MIOS32_MIDI_SendDebugMessage("lcd_type: 0x%02x (%s)\n", mios32_lcd_parameters.lcd_type, MIOS32_LCD_LcdTypeName(mios32_lcd_parameters.lcd_type));
  MIOS32_MIDI_SendDebugMessage("num_x:    %4d\n", mios32_lcd_parameters.num_x);
  MIOS32_MIDI_SendDebugMessage("num_y:    %4d\n", mios32_lcd_parameters.num_y);
  MIOS32_MIDI_SendDebugMessage("width:    %4d\n", mios32_lcd_parameters.width);
  MIOS32_MIDI_SendDebugMessage("height:   %4d\n", mios32_lcd_parameters.height);
  MIOS32_MIDI_SendDebugMessage("Testing %d LCDs\n", num_lcds);

  if( mios32_lcd_parameters.lcd_type != MIOS32_LCD_TYPE_GLCD_SSD1306 && mios32_lcd_parameters.lcd_type != MIOS32_LCD_TYPE_GLCD_SSD1306_ROTATED ) {
    // print warning if correct LCD hasn't been selected
    MIOS32_MIDI_SendDebugMessage("WARNING: your core module hasn't been configured for the SSD1306 GLCD!\n");
    MIOS32_MIDI_SendDebugMessage("Please do this with the bootloader update application!\n");
  }



  // print static screen
  MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);

  // endless loop - LED will flicker on each iteration
  while( 1 ) {
    // wait some mS
    MIOS32_DELAY_Wait_uS(10000);

    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    u8 n;
    for(n=0; n<num_lcds; ++n) {
      int i;
#if 0
      // X/Y "position" of displays
      const u8 lcd_x[MAX_LCDS] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}; // CS#0..7
      const u8 lcd_y[MAX_LCDS] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};

      // X/Y "position" of displays
      u8 x_offset = 128*lcd_x[n];
      u8 y_offset = 64*lcd_y[n];
#else
      // TK: expired! LCDs now addressed via MIOS32_LCD_DeviceSet()
      u8 x_offset = 0;
      u8 y_offset = 0;
      MIOS32_LCD_DeviceSet(n);
#endif

      // print text
      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
      MIOS32_LCD_GCursorSet(x_offset + 6*6, y_offset + 1*8);
      MIOS32_LCD_PrintFormattedString("SSD1306 #%d", n+1);

      MIOS32_LCD_GCursorSet(x_offset + 6*6, y_offset + 2*8);
      MIOS32_LCD_PrintString("powered by    ");

      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_BIG);
      MIOS32_LCD_GCursorSet(x_offset + 3*6, y_offset + 3*8);
      MIOS32_LCD_PrintString("MIOS32");

      // print vmeter icons
      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_METER_ICONS_V); // memo: 28 icons, 14 used, icon size: 8x32
      for(i=0; i<2; ++i) {
	if( ++vmeter_icon_delay_ctr[n][i] ) {
	  vmeter_icon_delay_ctr[n][i] = 0;
	  if( vmeter_icon_dir[n][i] ) {
	    if( ++vmeter_icon_ctr[n][i] >= 13 )
	      vmeter_icon_dir[n][i] = 0;
	  } else {
	    if( --vmeter_icon_ctr[n][i] < 1 )
	      vmeter_icon_dir[n][i] = 1;
	  }
	}
	MIOS32_LCD_GCursorSet(vmeter_icon_x[i]+x_offset, vmeter_icon_y[i]+y_offset);
	MIOS32_LCD_PrintChar(vmeter_icon_ctr[n][i]);
      }

      // print hmeter icons
      for(i=0; i<2; ++i) {
	MIOS32_LCD_FontInit((u8 *)GLCD_FONT_METER_ICONS_H); // memo: 28 icons, 14 used, icon size: 28x8
	if( ++hmeter_icon_delay_ctr[n][i] > 7 ) {
	  hmeter_icon_delay_ctr[n][i] = 0;
	  if( hmeter_icon_dir[n][i] ) {
	    if( ++hmeter_icon_ctr[n][i] >= 13 )
	      hmeter_icon_dir[n][i] = 0;
	  } else {
	    if( --hmeter_icon_ctr[n][i] < 1 )
	      hmeter_icon_dir[n][i] = 1;
	  }
	}
	MIOS32_LCD_GCursorSet(hmeter_icon_x[i]+x_offset, hmeter_icon_y[i]+y_offset);
	MIOS32_LCD_PrintChar(hmeter_icon_ctr[n][i]);

	MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	if( i == 0 ) {
	  MIOS32_LCD_GCursorSet(0+x_offset, hmeter_icon_y[i]+y_offset);
	  MIOS32_LCD_PrintFormattedString("%d", hmeter_icon_ctr[n][i]*4);
	} else {
	  MIOS32_LCD_GCursorSet(128-3*6+x_offset, hmeter_icon_y[i]+y_offset);
	  MIOS32_LCD_PrintFormattedString("%3d", hmeter_icon_ctr[n][i]*4);
	}
      }
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
