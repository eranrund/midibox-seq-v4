// $Id: mbng_enc.c 2174 2015-05-16 22:15:41Z tk $
//! \defgroup MBNG_ENC
//! Encoder access functions for MIDIbox NG
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "app.h"
#include "mbng_enc.h"
#include "mbng_lcd.h"
#include "mbng_patch.h"
#include "mbng_event.h"


/////////////////////////////////////////////////////////////////////////////
//! local variables
/////////////////////////////////////////////////////////////////////////////

static u8 enc_speed_multiplier;


/////////////////////////////////////////////////////////////////////////////
//! This function initializes the ENC handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  enc_speed_multiplier = 0;

  int i;
  for(i=1; i<MIOS32_ENC_NUM_MAX; ++i) { // start at 1 since the first encoder is allocated by SCS
    mios32_enc_config_t enc_config;
    enc_config = MIOS32_ENC_ConfigGet(i);
    enc_config.cfg.type = NON_DETENTED;
    enc_config.cfg.sr = 0;
    enc_config.cfg.pos = 0;
    enc_config.cfg.speed = NORMAL;
    enc_config.cfg.speed_par = 0;
    MIOS32_ENC_ConfigSet(i, enc_config);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Enables/Disables fast mode with given multiplier
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_FastModeSet(u8 multiplier)
{
  enc_speed_multiplier = multiplier;
  return 0;
}

s32 MBNG_ENC_FastModeGet(void)
{
  return enc_speed_multiplier;
}


/////////////////////////////////////////////////////////////////////////////
//! Sets the encoder speed depending on value range
//! should this be optional?
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_AutoSpeed(u32 enc, mbng_event_item_t *item, u32 range)
{
  int enc_ix = enc + 1;
  if( enc == (mbng_patch_scs.enc_emu_id-1) )
    enc_ix = 0;

  mios32_enc_config_t enc_config;
  enc_config = MIOS32_ENC_ConfigGet(enc_ix); // add +1 since the first encoder is allocated by SCS

  mios32_enc_speed_t cfg_speed = NORMAL;
  int                cfg_speed_par = 0;

  if(        item->custom_flags.ENC.enc_speed_mode == MBNG_EVENT_ENC_SPEED_MODE_SLOW ) {
    cfg_speed = SLOW;
    cfg_speed_par = item->custom_flags.ENC.enc_speed_mode_par;
  } else if( item->custom_flags.ENC.enc_speed_mode <= MBNG_EVENT_ENC_SPEED_MODE_NORMAL ) {
    cfg_speed = NORMAL;
    cfg_speed_par = item->custom_flags.ENC.enc_speed_mode_par;
  } else if( item->custom_flags.ENC.enc_speed_mode <= MBNG_EVENT_ENC_SPEED_MODE_FAST ) {
    cfg_speed = FAST;
    cfg_speed_par = item->custom_flags.ENC.enc_speed_mode_par;
  } else { // MBNG_EVENT_ENC_SPEED_MODE_AUTO
    if( enc_config.cfg.type == NON_DETENTED ) {
      if( range < 32  ) {
	cfg_speed = SLOW;
	cfg_speed_par = 3;
      } else if( range <= 256 ) {
	cfg_speed = NORMAL;
      } else {
	cfg_speed = FAST;
	cfg_speed_par = 2 + (range / 2048);
	if( cfg_speed_par > 7 )
	  cfg_speed_par = 7;
      }
    } else {
      if( range < 32  ) {
	cfg_speed = SLOW;
	cfg_speed_par = 3;
      } else if( range <= 64 ) {
	cfg_speed = NORMAL;
      } else {
	cfg_speed = FAST;
	cfg_speed_par = 2 + (range / 2048);
	if( cfg_speed_par > 7 )
	  cfg_speed_par = 7;
      }
    }
  }

  if( enc_config.cfg.speed != cfg_speed || enc_config.cfg.speed_par != cfg_speed_par ) {
    enc_config.cfg.speed = cfg_speed;
    enc_config.cfg.speed_par = cfg_speed_par;
    MIOS32_ENC_ConfigSet(enc_ix, enc_config); // add +1 since the first encoder is allocated by SCS
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function should be called from APP_ENC_NotifyChange when an encoder
//! has been moved
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  u16 hw_id = MBNG_EVENT_CONTROLLER_ENC + encoder + 1;
  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_ENC_NotifyChange(%d, %d)\n", hw_id & 0xfff, incrementer);
  }

  // MIDI Learn
  MBNG_EVENT_MidiLearnIt(hw_id);

  // get ID
  mbng_event_item_t item;
  u32 continue_ix = 0;
  do {
    if( MBNG_EVENT_ItemSearchByHwId(hw_id, &item, &continue_ix) < 0 ) {
      if( continue_ix )
	return 0; // ok: at least one event was assigned
      if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("No event assigned to ENC hw_id=%d\n", hw_id & 0xfff);
      }
      return -2; // no event assigned
    }

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      MBNG_EVENT_ItemPrint(&item, 0);
    }

    s32 event_incrementer = incrementer;
    if( enc_speed_multiplier > 1 ) {
      event_incrementer *= (s32)enc_speed_multiplier;
    }

    // set speed mode
    u8 *map_values;
    int map_len = MBNG_EVENT_MapGet(item.map, &map_values);
    int range = (map_len > 0) ? map_len : ((item.min <= item.max) ? (item.max - item.min + 1) : (item.min - item.max + 1));
    MBNG_ENC_AutoSpeed(encoder, &item, range);

    // change value
    s32 value = 0;
    switch( item.custom_flags.ENC.enc_mode ) {
    case MBNG_EVENT_ENC_MODE_40SPEED:
      value = 0x40 + event_incrementer;
      if( value < 0 )
	value = 0;
      else if( value >= 0x7f )
	value = 0x7f;
      break;

    case MBNG_EVENT_ENC_MODE_00SPEED:
      value = event_incrementer & 0x7f;
      break;

    case MBNG_EVENT_ENC_MODE_INC00SPEED_DEC40SPEED:
      if( event_incrementer < 0 ) {
	if( event_incrementer < -63 )
	  event_incrementer = -63;
	value = -event_incrementer | 0x40;
      } else {
	if( event_incrementer > 63 )
	  event_incrementer = 63;
	value = event_incrementer;
      }
      break;

    case MBNG_EVENT_ENC_MODE_INC41_DEC3F:
      value = event_incrementer > 0 ? 0x41 : 0x3f;
      break;

    case MBNG_EVENT_ENC_MODE_INC01_DEC7F:
      value = event_incrementer > 0 ? 0x01 : 0x7f;
      break;

    case MBNG_EVENT_ENC_MODE_INC01_DEC41:
      value = event_incrementer > 0 ? 0x01 : 0x41;
      break;

    default: { // MBNG_EVENT_ENC_MODE_ABSOLUTE
      u8 force_send = 0;
      if( map_len > 0 ) {
	int map_ix = item.map_ix; // MBNG_EVENT_MapIxFromValue(map_values, map_len, item.value);
	int prev_map_ix = map_ix;
	map_ix += event_incrementer;
	if( map_ix >= map_len )
	  map_ix = map_len - 1;
	else if( map_ix < 0 )
	  map_ix = 0;
	MBNG_EVENT_ItemSetMapIx(&item, map_ix);
	value = map_values[map_ix];

	if( prev_map_ix != map_ix )
	  force_send = 1;
      } else {
	if( item.min <= item.max ) {
	  value = item.value + event_incrementer;
	  if( value < item.min )
	    value = item.min;
	  else if( value > item.max )
	    value = item.max;
	} else {
	  // reversed range
	  value = item.value - event_incrementer;
	  if( value < item.max )
	    value = item.max;
	  else if( value > item.min )
	    value = item.min;
	}
      }

      if( value == item.value && !force_send )
	return 0; // no change
    }
    }

    // take over new value
    item.value = value;

    if( MBNG_EVENT_NotifySendValue(&item) == 2 )
      break; // stop has been requested
  } while( continue_ix );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by MBNG_EVENT_ItemReceive when a matching value
//! has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_NotifyReceivedValue(mbng_event_item_t *item)
{
  u16 hw_id = item->hw_id;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_ENC_NotifyReceivedValue(%d, %d)\n", hw_id & 0xfff, item->value);
  }

  // nothing else to do...

  return 0; // no error
}


//! \}
