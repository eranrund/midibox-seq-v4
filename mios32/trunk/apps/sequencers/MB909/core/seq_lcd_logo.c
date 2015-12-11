// $Id: seq_lcd_logo.c 1117 2010-10-24 12:43:57Z tk $
/*
 * LCD logo functions
 * Inspired by Waldorf MicroQ screensaver
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

#include "tasks.h"
#include "seq_ui.h"
#include "seq_lcd.h"
#include "seq_lcd_logo.h"



/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////
#define LOGO_X 200
#define LOGO_Y 16

#define LCD_CHAR_X 5
#define LCD_CHAR_Y 8

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const u8 logo[LOGO_X * (LOGO_Y/LCD_CHAR_Y)] = {
#include "seq_lcd_logo.inc"
};


/////////////////////////////////////////////////////////////////////////////
// Init logo functions
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_LOGO_Init(u32 mode)
{
  // we generate our own charset
  // setting "None" results into proper re-initialisation after logo has been displayed
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_None);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Copy part of logo into special characters
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LCD_LOGO_Copy(u8 logo_x_offset, u8 first_special_char)
{
  int lcd_char, lcd_x, lcd_y;
  int logo_x, logo_line_offset;
  u8 buffer[8*8];

  // copy graphical data into 8x8 buffer
  // 8 special characters with 8 pixels each are available
  for(lcd_char=0; lcd_char<8; ++lcd_char) {
    int mapped_lcd_char = ((first_special_char + lcd_char) % 4) | (lcd_char & 0x04); // this mapping rotates the first special char for a smooth animation
    for(lcd_y=0; lcd_y<LCD_CHAR_Y; ++lcd_y) {
      buffer[LCD_CHAR_Y*mapped_lcd_char + lcd_y] = 0;
      logo_x = LCD_CHAR_X*(lcd_char % 4) + logo_x_offset;
      logo_line_offset = LOGO_X * (lcd_char / 4);
      for(lcd_x=0; lcd_x<LCD_CHAR_X; ++lcd_x, ++logo_x) {
	if( logo[logo_line_offset + logo_x] & (1 << lcd_y) )
	  buffer[LCD_CHAR_Y*mapped_lcd_char + lcd_y] |= (1 << (LCD_CHAR_X-1-lcd_x));
      }
    }
  }

  // transfer to LCD
  MUTEX_LCD_TAKE;

#if LCD_NUM_DEVICES < 2
  MIOS32_LCD_DeviceSet(1);
  MIOS32_LCD_SpecialCharsInit(buffer);
#elif LCD_NUM_DEVICES == 4
  // print logo on two leftmost LCDs
  int lcd;
  for(lcd=2; lcd<LCD_NUM_DEVICES; ++lcd) {
    MIOS32_LCD_DeviceSet(lcd);
    MIOS32_LCD_SpecialCharsInit(buffer);
  }
#else
# error "SEQ_LCD_LOGO_Print only prepared for 2 or 4 LCDs"
#endif
  MUTEX_LCD_GIVE;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Prints part of the logo with special characters
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_LOGO_Print(u8 lcd_pos)
{
  u8 logo_offset = 40;
  int i;

  // clear leftmost characters
  if( lcd_pos > 0 ) {
    SEQ_LCD_CursorSet(logo_offset + lcd_pos-1, 0);
    SEQ_LCD_PrintChar(' ');

    SEQ_LCD_CursorSet(logo_offset + lcd_pos-1, 1);
    SEQ_LCD_PrintChar(' ');
  }

  // update immediately    
  SEQ_UI_LCD_Update();

  // rotate first special character
  int first_special_char = lcd_pos % 4;

  // generate new characters
  SEQ_LCD_LOGO_Copy(lcd_pos * LCD_CHAR_X, first_special_char);

  // print special characters
  SEQ_LCD_CursorSet(logo_offset + lcd_pos, 0);

  for(i=0; i<4; ++i)
    SEQ_LCD_PrintChar((first_special_char + i) % 4);

  SEQ_LCD_CursorSet(logo_offset + lcd_pos, 1);

  for(i=0; i<4; ++i)
    SEQ_LCD_PrintChar(4 + ((first_special_char + i) % 4));

  // update immediately
  SEQ_UI_LCD_Update();

  return 0; // no error
}
