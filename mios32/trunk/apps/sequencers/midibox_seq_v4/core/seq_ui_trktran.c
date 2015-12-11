// $Id: seq_ui_trktran.c 1263 2011-07-19 18:37:25Z tk $
/*
 * Track length page
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
#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_cc.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       3
#define ITEM_OCTAVE        0
#define ITEM_SEMITONE      1
#define ITEM_GXTY          2


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_OCTAVE: *gp_leds = (1 << ((SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_OCT)-8)&0xf)); break;
    case ITEM_SEMITONE: *gp_leds = (1 << ((SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_SEMI)-8)&0xf)); break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local encoder callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported encoder
/////////////////////////////////////////////////////////////////////////////
static s32 Encoder_Handler(seq_ui_encoder_t encoder, s32 incrementer)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  if( encoder == SEQ_UI_ENCODER_GP1 )
    ui_selected_item = ITEM_GXTY;
  else if( encoder >= SEQ_UI_ENCODER_GP2 && encoder <= SEQ_UI_ENCODER_GP16 ) {
    if( ui_selected_item == ITEM_GXTY )
      ui_selected_item = ITEM_OCTAVE;

    switch( ui_selected_item ) {
      case ITEM_OCTAVE:   return SEQ_UI_CC_Set(SEQ_CC_TRANSPOSE_OCT, (encoder-8) & 0xf);
      case ITEM_SEMITONE: return SEQ_UI_CC_Set(SEQ_CC_TRANSPOSE_SEMI, (encoder-8) & 0xf);
    }
  }

  // for GXTY via GP encoder + Datawheel
  u8 change_cc = 0;
  switch( ui_selected_item ) {
  case ITEM_GXTY:     return SEQ_UI_GxTyInc(incrementer);
  case ITEM_OCTAVE:   change_cc = SEQ_CC_TRANSPOSE_OCT; break;
  case ITEM_SEMITONE: change_cc = SEQ_CC_TRANSPOSE_SEMI; break;
  }

  if( change_cc ) {
    u8 value = (SEQ_CC_Get(visible_track, change_cc) + 7) & 0xf;
    if( SEQ_UI_Var8_Inc(&value, 0, 14, incrementer) ) {
      SEQ_CC_Set(visible_track, change_cc, (value - 7) & 0xf);
      return 1;
    }
  }

  return -1; // invalid or unsupported encoder
}


/////////////////////////////////////////////////////////////////////////////
// Local button callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported button
/////////////////////////////////////////////////////////////////////////////
static s32 Button_Handler(seq_ui_button_t button, s32 depressed)
{
  if( depressed ) return 0; // ignore when button depressed

  if( button >= SEQ_UI_BUTTON_GP2 && button <= SEQ_UI_BUTTON_GP16 ) {
    if( ui_selected_item == ITEM_GXTY )
      ui_selected_item = ITEM_OCTAVE;

    switch( ui_selected_item ) {
      case ITEM_OCTAVE:   return SEQ_UI_CC_Set(SEQ_CC_TRANSPOSE_OCT, (button-8) & 0xf);
      case ITEM_SEMITONE: return SEQ_UI_CC_Set(SEQ_CC_TRANSPOSE_SEMI, (button-8) & 0xf);
    }
  }

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
      ui_selected_item = ITEM_GXTY;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	ui_selected_item = 0;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      else
	--ui_selected_item;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Down:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);
  }

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Transpose: >Octave:+0<>Semitone:+0<                                           
  // GxTy  -7   -6   -5   -4   -3   -2   -1  >+0<  +1   +2   +3   +4   +5   +6   +7

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk. Transpose: ");

  SEQ_LCD_PrintChar(ui_selected_item == ITEM_OCTAVE ? '>' : ' ');
  if( ui_selected_item == ITEM_OCTAVE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(9);
  } else {
    u8 value = SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_OCT);
    SEQ_LCD_PrintFormattedString("Octave:%c%d", (value < 8) ? '+' : '-', (value < 8) ? value : (16-value));
  }
  SEQ_LCD_PrintChar(ui_selected_item == ITEM_OCTAVE ? '<' : ' ');

  SEQ_LCD_PrintChar(ui_selected_item == ITEM_SEMITONE ? '>' : ' ');
  if( ui_selected_item == ITEM_SEMITONE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(11);
  } else {
    u8 value = SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_SEMI);
    SEQ_LCD_PrintFormattedString("Semitone:%c%d", (value < 8) ? '+' : '-', (value < 8) ? value : (16-value));
  }
  SEQ_LCD_PrintChar(ui_selected_item == ITEM_SEMITONE ? '<' : ' ');

  SEQ_LCD_PrintSpaces(40); // second half


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(1);
  }

  ///////////////////////////////////////////////////////////////////////////

  u8 value = SEQ_CC_Get(visible_track, ui_selected_item == ITEM_SEMITONE ? SEQ_CC_TRANSPOSE_SEMI : SEQ_CC_TRANSPOSE_OCT);

  int i;
  for(i=1; i<8; ++i) {
    SEQ_LCD_PrintChar(value == (8+i) ? '>' : ' ');

    if( ui_selected_item != ITEM_GXTY && value == (8+i) && ui_cursor_flash )
      SEQ_LCD_PrintSpaces(2);
    else
      SEQ_LCD_PrintFormattedString("-%d", 8-i);

    SEQ_LCD_PrintChar(value == (8+i) ? '<' : ' ');
    SEQ_LCD_PrintChar(' ');
  }

  for(i=8; i<16; ++i) {
    SEQ_LCD_PrintChar(value == (i-8) ? '>' : ' ');

    if( ui_selected_item != ITEM_GXTY && value == (i-8)  && ui_cursor_flash )
      SEQ_LCD_PrintSpaces(2);
    else
      SEQ_LCD_PrintFormattedString("+%d", i-8);

    SEQ_LCD_PrintChar(value == (i-8) ? '<' : ' ');
    SEQ_LCD_PrintChar(' ');
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKTRAN_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
