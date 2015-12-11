// $Id: mbng_event.c 2019 2014-06-04 19:09:46Z tk $
//! \defgroup MBNG_EVENT
//! Event Handler for MIDIbox NG
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
#include <string.h>

//#include <scs.h>
#include <ainser.h>
#include <midimon.h>
#include <seq_bpm.h>

#include "app.h"
#include "tasks.h"
#include "mbng_event.h"
#include "mbng_lcd.h"
#include "mbng_din.h"
#include "mbng_dout.h"
#include "mbng_matrix.h"
#include "mbng_enc.h"
#include "mbng_ain.h"
#include "mbng_ainser.h"
#include "mbng_mf.h"
#include "mbng_cv.h"
#include "mbng_kb.h"
#include "mbng_seq.h"
#include "mbng_patch.h"
#include "mbng_file_s.h"
#include "mbng_file_r.h"
#include "mbfm.h"
#include "mbfm_controlhandler.h"
#include "mbfm_sequencer.h"


/////////////////////////////////////////////////////////////////////////////
//! Local types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u16 ALL;

  struct {
    u16 match_ctr:13;
    u16 dump:1;
    u16 txt:1;
    u16 cursor_set:1;
  };
} sysex_runtime_var_t;

typedef union {
  u16 ALL;

  struct {
    u16 has_fwd_id:1;
    u16 has_fwd_value:1;
    u16 has_cond:1;
    u16 has_min:1;
    u16 has_max:1;
    u16 has_offset:1;
    u16 has_rgb:1;
    u16 has_map:1;
    u16 has_lcd:1;
    u16 has_lcd_x:1;
    u16 has_lcd_y:1;
  };
} extra_par_available_t;

typedef struct { // should be dividable by u16
  u16 id;
  u16 hw_id;
  mbng_event_flags_t flags; // 32bit
  mbng_event_syxdump_pos_t syxdump_pos; // 32bit
  mbng_event_custom_flags_t custom_flags; // 16bit
  extra_par_available_t extra_par_available; // 16bit
  u16 enabled_ports;
  u16 value;
  sysex_runtime_var_t sysex_runtime_var;    // several runtime flags
  u8 len; // for the whole item. positioned here, so that u16 entries are halfword aligned
  u8 len_stream;
  u8 len_label;
  u8 bank;
  u8 secondary_value;
  u8 data_begin; // data section for streams, label and extra parameters starts here, it can have multiple bytes
} mbng_event_pool_item_t;

typedef struct {
  u8 len;
  u8 num;
  u8 data_begin; // data section for value map starts here, it can have multiple bytes
} mbng_event_pool_map_t;


/////////////////////////////////////////////////////////////////////////////
//! Local variables
/////////////////////////////////////////////////////////////////////////////


#if defined(MIOS32_FAMILY_STM32F4xx)
# define MBNG_EVENT_POOL_MAX_SIZE (64*1024)
#else
# define MBNG_EVENT_POOL_MAX_SIZE (24*1024)
#endif

#ifndef AHB_SECTION
#define AHB_SECTION
#endif

#if MBNG_EVENT_POOL_MAX_SIZE > (64*1024)
# error "More than 64k Event Pool is not supported yet!"
#endif
static u8 AHB_SECTION event_pool[MBNG_EVENT_POOL_MAX_SIZE];
static u16 event_pool_size;
static u16 event_pool_maps_begin;
static u16 event_pool_num_items;
static u16 event_pool_num_maps;

// last active event
mbng_event_item_id_t last_event_item_id;

// banks
u8 selected_bank;
u8 num_banks;

// listen to NRPN for up to 8 ports at up to 16 channels
// in order to save RAM, we only listen to USB and UART based ports! (this already costs us 512 byte!)
#define MBNG_EVENT_NRPN_RECEIVE_PORTS_MASK    0x00ff
#define MBNG_EVENT_NRPN_RECEIVE_PORTS_OFFSET  0
#define MBNG_EVENT_NRPN_RECEIVE_PORTS         8
#define MBNG_EVENT_NRPN_RECEIVE_CHANNELS     16
static u16 nrpn_received_address[MBNG_EVENT_NRPN_RECEIVE_PORTS][MBNG_EVENT_NRPN_RECEIVE_CHANNELS];
static u16 nrpn_received_value[MBNG_EVENT_NRPN_RECEIVE_PORTS][MBNG_EVENT_NRPN_RECEIVE_CHANNELS];

// for UART based transfers we also optimize the output
#define MBNG_EVENT_NRPN_SEND_PORTS_MASK    0x00f0
#define MBNG_EVENT_NRPN_SEND_PORTS_OFFSET  4
#define MBNG_EVENT_NRPN_SEND_PORTS         4
#define MBNG_EVENT_NRPN_SEND_CHANNELS     16
static u16 nrpn_sent_address[MBNG_EVENT_NRPN_SEND_PORTS][MBNG_EVENT_NRPN_SEND_CHANNELS];
static u16 nrpn_sent_value[MBNG_EVENT_NRPN_SEND_PORTS][MBNG_EVENT_NRPN_SEND_CHANNELS];


static u8                    midi_learn_mode;
static u8                    midi_learn_nrpn_chn;
static u8                    midi_learn_nrpn_valid;
static u16                   midi_learn_min;
static u16                   midi_learn_max;
static mios32_midi_package_t midi_learn_event;
static mios32_midi_port_t    midi_learn_nrpn_port;
static u16                   midi_learn_nrpn_address;
static u16                   midi_learn_nrpn_value;

// hardcoded "Logic Control Meters"
// receive on USB1..USB4 and OUT1..OUT4
#define LC_METERS_NUM_PORTS 8
#define LC_METERS_NUM_ITEMS 8
static u8 lc_meters[LC_METERS_NUM_PORTS][LC_METERS_NUM_ITEMS];


/////////////////////////////////////////////////////////////////////////////
//! Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_EVENT_ItemCopy2User(mbng_event_pool_item_t* pool_item, mbng_event_item_t *item);
static s32 MBNG_EVENT_ItemCopy2Pool(mbng_event_item_t *item, mbng_event_pool_item_t* pool_item);

static s32 MBNG_EVENT_LCMeters_Update(void);
static s32 MBNG_EVENT_LCMeters_Set(u8 port_ix, u8 lc_meter_value);
static s32 MBNG_EVENT_LCMeters_Tick(void);


/////////////////////////////////////////////////////////////////////////////
//! This function initializes the event pool structure
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  MBNG_EVENT_PoolClear();

  {
    int i, j;

    u16 *nrpn_received_address_ptr = (u16 *)&nrpn_received_address[0][0];
    u16 *nrpn_received_value_ptr = (u16 *)&nrpn_received_value[0][0];
    for(i=0; i<MBNG_EVENT_NRPN_RECEIVE_PORTS; ++i) {
      for(j=0; j<MBNG_EVENT_NRPN_RECEIVE_CHANNELS; ++j) {
	*nrpn_received_address_ptr = 0;
	*nrpn_received_value_ptr = 0;
      }
    }

    u16 *nrpn_sent_address_ptr = (u16 *)&nrpn_sent_address[0][0];
    u16 *nrpn_sent_value_ptr = (u16 *)&nrpn_sent_value[0][0];
    for(i=0; i<MBNG_EVENT_NRPN_SEND_PORTS; ++i) {
      for(j=0; j<MBNG_EVENT_NRPN_SEND_CHANNELS; ++j) {
	*nrpn_sent_address_ptr = 0xffff; // invalidate
	*nrpn_sent_value_ptr = 0xffff; // invalidate
      }
    }
  }

  MBNG_EVENT_LCMeters_Init();

  MBNG_EVENT_MidiLearnModeSet(0);

  mbng_event_item_t item;

  // Default Items:
  int i;

  // Buttons
  for(i=1; i<=64; ++i) {
    char str[21];
    u8 stream[20];

    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_BUTTON + i);
    item.fwd_id = MBNG_EVENT_CONTROLLER_LED | i;
    item.flags.type = MBNG_EVENT_TYPE_NOTE_ON;
    stream[0] = 0x90;
    stream[1] = 0x24 + i - 1;
    item.stream = stream;
    item.stream_size = 2;
    item.secondary_value = stream[1];

    strcpy(str, "^std_btn"); // Button #%3i %3d%b
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  // Encoders
  for(i=1; i<=64; ++i) {
    char str[21];
    u8 stream[20];

    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_ENC + i);
    item.fwd_id = MBNG_EVENT_CONTROLLER_LED_MATRIX | i;
    item.flags.type = MBNG_EVENT_TYPE_CC;
    stream[0] = 0xb0;
    stream[1] = 0x10 + i - 1;
    item.stream = stream;
    item.stream_size = 2;
    item.secondary_value = stream[1];

    strcpy(str, "^std_enc"); // ENC #%3i    %3d%B
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  // AINSER
  AINSER_EnabledSet(0, 1);
  for(i=1; i<=64; ++i) {
    char str[21];
    u8 stream[20];

    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_AINSER + i);
    item.flags.type = MBNG_EVENT_TYPE_CC;
    stream[0] = 0xb1;
    stream[1] = 0x10 + i - 1;
    item.stream = stream;
    item.stream_size = 2;
    item.secondary_value = stream[1];

    strcpy(str, "^std_aser"); // AINSER #%3i %3d%B
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  // AINs
  for(i=1; i<=6; ++i) {
    char str[21];
    u8 stream[20];

    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_AIN + i);
    item.flags.type = MBNG_EVENT_TYPE_CC;
    stream[0] = 0xb2;
    stream[1] = 0x10 + i - 1;
    item.stream = stream;
    item.stream_size = 2;
    item.secondary_value = stream[1];

    strcpy(str, "^std_ain"); // AIN #%3i    %3d%B
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  MBNG_EVENT_PoolUpdate();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called each mS (with low priority)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_Tick(void)
{
  {
    static u16 ms_counter = 0;

    // each second:
    if( ++ms_counter >= 1000 ) {
      ms_counter = 0;

      // invalidate the NRPN optimizer
      portENTER_CRITICAL(); // should be atomic!
      {
	int i, j;

	u16 *nrpn_sent_address_ptr = (u16 *)&nrpn_sent_address[0][0];
	u16 *nrpn_sent_value_ptr = (u16 *)&nrpn_sent_value[0][0];
	for(i=0; i<MBNG_EVENT_NRPN_SEND_PORTS; ++i) {
	  for(j=0; j<MBNG_EVENT_NRPN_SEND_CHANNELS; ++j) {
	    *nrpn_sent_address_ptr = 0xffff; // invalidate
	    *nrpn_sent_value_ptr = 0xffff; // invalidate
	  }
	}
      }
      portEXIT_CRITICAL();
    }
  }

  {
    static u16 ms_counter = 0;

    // each 300 mS
    if( ++ms_counter >= 300 ) {
      ms_counter = 0;

      MBNG_EVENT_LCMeters_Tick();
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Clear the event pool
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolClear(void)
{
  event_pool_size = 0;
  event_pool_maps_begin = 0;
  event_pool_num_items = 0;
  event_pool_num_maps = 0;

  last_event_item_id = 0;

  selected_bank = 1;
  num_banks = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Called after a new file has been loaded (post-processing step).
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolUpdate(void)
{
  num_banks = 0;

  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

    // banks
    {
      if( pool_item->bank ) {
	pool_item->flags.active = (pool_item->bank == selected_bank);

	if( pool_item->bank > num_banks )
	  num_banks = pool_item->bank;
      } else {
	pool_item->flags.active = 1;
      }
    }

    pool_ptr += pool_item->len;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Sends the event pool to debug terminal
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolPrint(void)
{
  return MIOS32_MIDI_SendDebugHexDump(event_pool, event_pool_size);
}

/////////////////////////////////////////////////////////////////////////////
//! Sends short item informations to debug terminal
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolItemsPrint(void)
{
  if( !event_pool_num_items ) {
    DEBUG_MSG("No Events in pool\n");
  } else {
    DEBUG_MSG("%d Events in pool:\n", event_pool_num_items);
    u8 *pool_ptr = (u8 *)&event_pool[0];
    u32 i;
    for(i=0; i<event_pool_num_items; ++i) {
      mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

      mbng_event_item_t item;
      MBNG_EVENT_ItemCopy2User(pool_item, &item);
      MBNG_EVENT_ItemPrint(&item, 0);

      pool_ptr += pool_item->len;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Sends short map informations to debug terminal
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolMapsPrint(void)
{
  if( !event_pool_num_maps ) {
    DEBUG_MSG("No Maps in pool\n");
  } else {
    DEBUG_MSG("%d Maps in pool:\n", event_pool_num_maps);
    u8 *pool_ptr = (u8 *)&event_pool[event_pool_maps_begin];
    u32 i;
    for(i=0; i<event_pool_num_maps; ++i) {
      mbng_event_pool_map_t *pool_map = (mbng_event_pool_map_t *)pool_ptr;

      char value_str[128];
      sprintf(value_str, "MAP%d", pool_map->num);

      u8 *map_values = (u8 *)&pool_map->data_begin;
      int j;
      for(j=2; j<pool_map->len && j < 16; ++j ) {
	sprintf(value_str, "%s %d", value_str, *map_values++);
      }

      if( j == 16 ) {
	sprintf(value_str, "%s ...", value_str);
      }
      DEBUG_MSG(value_str);

      pool_ptr += pool_map->len;
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! \returns the number of EVENT items
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolNumItemsGet(void)
{
  return event_pool_num_items;
}

/////////////////////////////////////////////////////////////////////////////
//! \returns the number of MAPs
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolNumMapsGet(void)
{
  return event_pool_num_maps;
}

/////////////////////////////////////////////////////////////////////////////
//! \returns the current pool size
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolSizeGet(void)
{
  return event_pool_size;
}

/////////////////////////////////////////////////////////////////////////////
//! \returns the maximum pool size
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolMaxSizeGet(void)
{
  return MBNG_EVENT_POOL_MAX_SIZE;
}


/////////////////////////////////////////////////////////////////////////////
//! Adds a map to event pool
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_MapAdd(u8 map, u8 *map_values, u8 len)
{
  if( (event_pool_size+len+2) > MBNG_EVENT_POOL_MAX_SIZE )
    return -2; // out of storage 

  u32 event_pool_map_start = event_pool_size;

  ++event_pool_num_maps;
  u8 *pool_ptr = (u8 *)&event_pool[event_pool_map_start];
  *pool_ptr++ = len+2;
  *pool_ptr++ = map;
  memcpy(pool_ptr, (u8 *)map_values, len);

  event_pool_size += len + 2;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Returns a map from the event pool
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_MapGet(u8 map, u8 **map_values)
{
  u8 *pool_ptr = (u8 *)&event_pool[event_pool_maps_begin];
  u32 i;
  for(i=0; i<event_pool_num_maps; ++i) {
      mbng_event_pool_map_t *pool_map = (mbng_event_pool_map_t *)pool_ptr;

      if( pool_map->num == map ) {
	*map_values = (u8 *)&pool_map->data_begin;
	return pool_map->len - 2;
      }

      pool_ptr += pool_map->len;
  }

  return -1; // map not available
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the index of a given value
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_MapIxGet(u8 *map_values, u8 map_len, u8 value)
{
  // first search for exact match
  {
    int i;
    u8 *map_values_ptr = map_values;
    for(i=0; i<map_len; ++i)
      if( *map_values_ptr++ == value )
	return i;
  }

  // otherwise search for match which is close to the given value
  {
    int i;
    u8 *map_values_ptr = map_values;
    for(i=0; i<map_len; ++i)
      if( *map_values_ptr++ > value ) {
	return (i == 0) ? 0 : (i-1);
      }
  }

  return map_len-1; // no match -> take last index
}


/////////////////////////////////////////////////////////////////////////////
//! \Returns the number of defined banks
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_NumBanksGet(void)
{
  return num_banks;
}

/////////////////////////////////////////////////////////////////////////////
//! \Returns the selected bank
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_SelectedBankGet(void)
{
  return selected_bank;
}


/////////////////////////////////////////////////////////////////////////////
//! Called to set a new bank
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_SelectedBankSet(u8 new_bank)
{
  if( new_bank < 1 || new_bank > num_banks )
    return -1; // invalid bank

  selected_bank = new_bank;

  // update active flag of all elements depending on the bank
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

    if( pool_item->bank ) {
      pool_item->flags.active = (pool_item->bank == new_bank);
    }

    pool_ptr += pool_item->len;
  }

  // refresh the new selected elements
  MBNG_EVENT_Refresh();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! \Returns the selected bank of a given hw_id
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_HwIdBankGet(u16 hw_id)
{
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

    if( (pool_item->hw_id & 0xfff) == hw_id && pool_item->bank && pool_item->flags.active ) {
      return pool_item->bank; // bank found
    }

    pool_ptr += pool_item->len;
  }

  return 0; // either hw_id or no active item found
}

/////////////////////////////////////////////////////////////////////////////
//! Sets the new bank for the given hw_id only
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_HwIdBankSet(u16 hw_id, u8 new_bank)
{
  // update all items which are banked and which belong to the given hw_id
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

    if( (pool_item->hw_id & 0xfff) == hw_id && pool_item->bank ) {
      pool_item->flags.active = (pool_item->bank == new_bank);

      // refresh active item
      if( pool_item->flags.active ) {
	mbng_event_item_t item;
	MBNG_EVENT_ItemCopy2User(pool_item, &item);
	item.flags.update_lcd = 1; // force LCD update
	MBNG_EVENT_ItemReceive(&item, pool_item->value, 1, 1);
      }
    }

    pool_ptr += pool_item->len;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns default no_dump parameter for item depending in id and type
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemNoDumpDefault(mbng_event_item_t *item)
{
  switch( item->id & 0xf000 ) {
  //case MBNG_EVENT_CONTROLLER_DISABLED:
  case MBNG_EVENT_CONTROLLER_SENDER:
  //case MBNG_EVENT_CONTROLLER_RECEIVER:
  case MBNG_EVENT_CONTROLLER_BUTTON:
  case MBNG_EVENT_CONTROLLER_LED:
  case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX:
  case MBNG_EVENT_CONTROLLER_LED_MATRIX:
  case MBNG_EVENT_CONTROLLER_ENC:
  case MBNG_EVENT_CONTROLLER_AIN:
  case MBNG_EVENT_CONTROLLER_AINSER:
  case MBNG_EVENT_CONTROLLER_MF:
  case MBNG_EVENT_CONTROLLER_CV:
  //case MBNG_EVENT_CONTROLLER_KB            = 0xc000,
    switch( item->flags.type ) {
    //case MBNG_EVENT_TYPE_UNDEFINED:
    case MBNG_EVENT_TYPE_NOTE_OFF:
    case MBNG_EVENT_TYPE_NOTE_ON:
    case MBNG_EVENT_TYPE_POLY_PRESSURE:
    case MBNG_EVENT_TYPE_CC:
    case MBNG_EVENT_TYPE_PROGRAM_CHANGE:
    case MBNG_EVENT_TYPE_AFTERTOUCH:
    case MBNG_EVENT_TYPE_PITCHBEND:
    case MBNG_EVENT_TYPE_SYSEX:
    case MBNG_EVENT_TYPE_NRPN:
    //case MBNG_EVENT_TYPE_META:
    //case MBNG_EVENT_TYPE_MBFM:
      return 0; // allow dump by default
    }
  }

  return 1; // no dump
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes an item with default settings
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemInit(mbng_event_item_t *item, mbng_event_item_id_t id)
{
  item->id = id;
  item->pool_address = 0xffff; // invalid address
  item->flags.ALL = 0;
  item->custom_flags.ALL = 0;
  item->enabled_ports = 0x1011; // OSC1, UART1 and USB1
  item->fwd_id = 0;
  item->fwd_value = 0xffff;
  item->hw_id  = id;
  item->cond.ALL = 0;
  item->value  = 0;
  item->min    = 0;
  item->max    = 127;
  item->offset = 0;
  item->syxdump_pos.ALL = 0;
  item->rgb.ALL = 0;
  item->stream_size = 0;
  item->map = 0;
  item->bank = 0;
  item->secondary_value = 0;
  item->lcd = 0;
  item->lcd_x = 0;
  item->lcd_y = 0;
  item->stream = NULL;
  item->label = NULL;

  // differ between type
  switch( id & 0xf000 ) {
  case MBNG_EVENT_CONTROLLER_SENDER: {
  }; break;

  case MBNG_EVENT_CONTROLLER_RECEIVER: {
  }; break;

  case MBNG_EVENT_CONTROLLER_BUTTON: {
    item->custom_flags.DIN.button_mode = MBNG_EVENT_BUTTON_MODE_ON_OFF;
  }; break;

  case MBNG_EVENT_CONTROLLER_LED: {
  }; break;

  case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX: {
  }; break;

  case MBNG_EVENT_CONTROLLER_LED_MATRIX: {
  }; break;

  case MBNG_EVENT_CONTROLLER_ENC: {
    item->flags.led_matrix_pattern = MBNG_EVENT_LED_MATRIX_PATTERN_1;
    item->custom_flags.ENC.enc_mode = MBNG_EVENT_ENC_MODE_ABSOLUTE;
    item->custom_flags.ENC.enc_speed_mode = MBNG_EVENT_ENC_SPEED_MODE_AUTO;
  }; break;

  case MBNG_EVENT_CONTROLLER_AIN: {
    item->custom_flags.AIN.ain_mode = MBNG_EVENT_AIN_MODE_DIRECT;
  }; break;

  case MBNG_EVENT_CONTROLLER_AINSER: {
    item->custom_flags.AINSER.ain_mode = MBNG_EVENT_AIN_MODE_DIRECT;
  }; break;

  case MBNG_EVENT_CONTROLLER_MF: {
  }; break;

  case MBNG_EVENT_CONTROLLER_CV: {
  }; break;

  case MBNG_EVENT_CONTROLLER_KB: {
  }; break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Local function to copy a pool item into a "user" item
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_EVENT_ItemCopy2User(mbng_event_pool_item_t* pool_item, mbng_event_item_t *item)
{
  // note: sooner or later I will clean-up this copy routine!
  // It copies many parts of the mbng_event_pool_item_t structure into the mbng_event_item_t
  // with accesses to all structure members. 
  // Each copy operation consists of two instructions (see project_build/project.lss)
  // regardless if 32, 16 or 8 bit variables are copied.
  // Optimisation could be done by arranging struct members in a way which allows direct 32bit
  // copy operations.
  // Maintaining two structs this way is "dirty", but really helps to improve the performance (a bit)
  // on this timing critical part.
  // This should be done once the struct definitions are settled - currently it isn't urgent
  // since I haven't noticed significant performnance issues yet (on a LPC17 core...)
  item->id = pool_item->id;
  u32 pool_address = (u32)pool_item - (u32)event_pool;
  item->pool_address = (pool_address < MBNG_EVENT_POOL_MAX_SIZE) ? pool_address : 0xffff;
  item->flags.ALL = pool_item->flags.ALL;
  item->custom_flags.ALL = pool_item->custom_flags.ALL;
  item->hw_id = pool_item->hw_id;
  item->value = pool_item->value;
  item->bank = pool_item->bank;
  item->enabled_ports = pool_item->enabled_ports;
  item->syxdump_pos.ALL = pool_item->syxdump_pos.ALL;

  item->matrix_pin = 0; // has to be set after creation by the MATRIX handler
  item->secondary_value = pool_item->secondary_value;
  item->stream_size = pool_item->len_stream;
  item->stream = pool_item->len_stream ? (u8 *)&pool_item->data_begin : NULL;
  item->label = pool_item->len_label ? ((char *)&pool_item->data_begin + pool_item->len_stream) : NULL;

  u8 *extra_par = (u8 *)(&pool_item->data_begin + pool_item->len_stream + pool_item->len_label);
  extra_par_available_t extra_par_available; extra_par_available.ALL = pool_item->extra_par_available.ALL;

  if( extra_par_available.has_cond ) {
    item->cond.ALL = extra_par[0] | (extra_par[1] << 8) | (extra_par[2] << 16) | (extra_par[3] << 24);
    extra_par += 4;
  } else {
    item->cond.ALL = 0;
  }

  if( extra_par_available.has_fwd_id ) {
    item->fwd_id = extra_par[0] | (extra_par[1] << 8);
    extra_par += 2;
  } else {
    item->fwd_id = 0;
  }

  if( extra_par_available.has_fwd_value ) {
    item->fwd_value = extra_par[0] | (extra_par[1] << 8);
    extra_par += 2;
  } else {
    item->fwd_value = 0xffff;
  }

  if( extra_par_available.has_min ) {
    item->min = (s16)(extra_par[0] | (extra_par[1] << 8));
    extra_par += 2;
  } else {
    item->min = 0;
  }

  if( extra_par_available.has_max ) {
    item->max = (s16)(extra_par[0] | (extra_par[1] << 8));
    extra_par += 2;
  } else {
    item->max = 127;
  }

  if( extra_par_available.has_offset ) {
    item->offset = (s16)(extra_par[0] | (extra_par[1] << 8));
    extra_par += 2;
  } else {
    item->offset = 0;
  }

  if( extra_par_available.has_rgb ) {
    item->rgb.ALL = (s16)(extra_par[0] | (extra_par[1] << 8));
    extra_par += 2;
  } else {
    item->rgb.ALL = 0;
  }

  if( extra_par_available.has_map ) {
    item->map = extra_par[0];
    extra_par += 1;
  } else {
    item->map = 0;
  }

  if( extra_par_available.has_lcd ) {
    item->lcd = extra_par[0];
    extra_par += 1;
  } else {
    item->lcd = 0;
  }

  if( extra_par_available.has_lcd_x ) {
    item->lcd_x = extra_par[0];
    extra_par += 1;
  } else {
    item->lcd_x = 0;
  }

  if( extra_par_available.has_lcd_y ) {
    item->lcd_y = extra_par[0];
    extra_par += 1;
  } else {
    item->lcd_y = 0;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Local function to copy a "user" item into a "pool" item
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_EVENT_ItemCopy2Pool(mbng_event_item_t *item, mbng_event_pool_item_t* pool_item)
{
  pool_item->id = item->id;
  pool_item->flags.ALL = item->flags.ALL;
  pool_item->custom_flags.ALL = item->custom_flags.ALL;
  pool_item->hw_id = item->hw_id;
  pool_item->value = item->value;
  pool_item->bank = item->bank;
  pool_item->enabled_ports = item->enabled_ports;
  pool_item->syxdump_pos.ALL = item->syxdump_pos.ALL;
  pool_item->secondary_value = item->secondary_value;

  u32 label_len = item->label ? (strlen(item->label)+1) : 0;
  u32 pool_item_len = sizeof(mbng_event_pool_item_t) - 1 + item->stream_size + label_len;

  pool_item->len_stream = item->stream ? item->stream_size : 0;
  pool_item->len_label = label_len;

  if( pool_item->len_stream )
    memcpy((u8 *)&pool_item->data_begin, item->stream, pool_item->len_stream);

  if( pool_item->len_label )
    memcpy((u8 *)&pool_item->data_begin + pool_item->len_stream, item->label, pool_item->len_label);

  u8 *extra_par = (u8 *)(&pool_item->data_begin + pool_item->len_stream + pool_item->len_label);
  pool_item->extra_par_available.ALL = 0;

  if( item->cond.ALL ) {
    pool_item->extra_par_available.has_cond = 1;
    extra_par[0] = item->cond.ALL;
    extra_par[1] = item->cond.ALL >> 8;
    extra_par[2] = item->cond.ALL >> 16;
    extra_par[3] = item->cond.ALL >> 24;
    extra_par += 4;
    pool_item_len += 4;
  }

  if( item->fwd_id ) {
    pool_item->extra_par_available.has_fwd_id = 1;
    extra_par[0] = item->fwd_id;
    extra_par[1] = item->fwd_id >> 8;
    extra_par += 2;
    pool_item_len += 2;
  }

  if( item->fwd_value != 0xffff ) {
    pool_item->extra_par_available.has_fwd_value = 1;
    extra_par[0] = item->fwd_value;
    extra_par[1] = item->fwd_value >> 8;
    extra_par += 2;
    pool_item_len += 2;
  }

  if( item->min ) {
    pool_item->extra_par_available.has_min = 1;
    extra_par[0] = (u16)item->min;
    extra_par[1] = (u16)item->min >> 8;
    extra_par += 2;
    pool_item_len += 2;
  }

  if( item->max != 127 ) {
    pool_item->extra_par_available.has_max = 1;
    extra_par[0] = (u16)item->max;
    extra_par[1] = (u16)item->max >> 8;
    extra_par += 2;
    pool_item_len += 2;
  }

  if( item->offset ) {
    pool_item->extra_par_available.has_offset = 1;
    extra_par[0] = (u16)item->offset;
    extra_par[1] = (u16)item->offset >> 8;
    extra_par += 2;
    pool_item_len += 2;
  }

  if( item->rgb.ALL ) {
    pool_item->extra_par_available.has_rgb = 1;
    extra_par[0] = item->rgb.ALL;
    extra_par[1] = item->rgb.ALL >> 8;
    extra_par += 2;
    pool_item_len += 2;
  }

  if( item->map ) {
    pool_item->extra_par_available.has_map = 1;
    extra_par[0] = item->map;
    extra_par += 1;
    pool_item_len += 1;
  }

  if( item->lcd ) {
    pool_item->extra_par_available.has_lcd = 1;
    extra_par[0] = item->lcd;
    extra_par += 1;
    pool_item_len += 1;
  }

  if( item->lcd_x ) {
    pool_item->extra_par_available.has_lcd_x = 1;
    extra_par[0] = item->lcd_x;
    extra_par += 1;
    pool_item_len += 1;
  }

  if( item->lcd_y ) {
    pool_item->extra_par_available.has_lcd_y = 1;
    extra_par[0] = item->lcd_y;
    extra_par += 1;
    pool_item_len += 1;
  }

  // temporary variables:
  pool_item->sysex_runtime_var.ALL = 0;

  // store length
  pool_item->len = pool_item_len;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Local function to calculate the expected pool item length
/////////////////////////////////////////////////////////////////////////////
static u32 MBNG_EVENT_ItemCalcPoolItemLen(mbng_event_item_t *item)
{
  u32 label_len = item->label ? (strlen(item->label)+1) : 0;
  u32 pool_item_len = sizeof(mbng_event_pool_item_t) - 1 + item->stream_size + label_len;

  if( item->cond.ALL ) {
    pool_item_len += 4;
  }

  if( item->fwd_id ) {
    pool_item_len += 2;
  }

  if( item->fwd_value != 0xffff ) {
    pool_item_len += 2;
  }

  if( item->min ) {
    pool_item_len += 2;
  }

  if( item->max != 127 ) {
    pool_item_len += 2;
  }

  if( item->offset ) {
    pool_item_len += 2;
  }

  if( item->rgb.ALL ) {
    pool_item_len += 2;
  }

  if( item->map ) {
    pool_item_len += 1;
  }

  if( item->lcd ) {
    pool_item_len += 1;
  }

  if( item->lcd_x ) {
    pool_item_len += 1;
  }

  if( item->lcd_y ) {
    pool_item_len += 1;
  }

  return pool_item_len;
}


/////////////////////////////////////////////////////////////////////////////
//! \Returns an item of the event pool with the given index number
//! (only used for dumping out all items in mbng_file_p.c)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemGet(u32 item_ix, mbng_event_item_t *item)
{
  if( item_ix >= event_pool_num_items )
    return -1; // not found

  u8 *pool_ptr = (u8 *)&event_pool[0];
  mbng_event_pool_item_t *pool_item;

  u32 i;
  for(i=0; i<=item_ix; ++i) {
    pool_item = (mbng_event_pool_item_t *)pool_ptr;
    pool_ptr += pool_item->len;
  }

  MBNG_EVENT_ItemCopy2User(pool_item, item);
  return 0; // item found
}

/////////////////////////////////////////////////////////////////////////////
//! Adds an item to event pool
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemAdd(mbng_event_item_t *item)
{
  u32 pool_item_len = MBNG_EVENT_ItemCalcPoolItemLen(item);

  if( pool_item_len > 255 )
    return -1; // too much data

  if( (event_pool_size+pool_item_len) > MBNG_EVENT_POOL_MAX_SIZE )
    return -2; // out of storage 

  // shift map items
  if( event_pool_maps_begin < event_pool_size ) {
    u8 *first_map = (u8 *)&event_pool[event_pool_maps_begin];
    u8 *new_map_begin = (u8 *)&event_pool[event_pool_maps_begin + pool_item_len];
    u32 map_items_size = event_pool_size - event_pool_maps_begin;
    memmove(new_map_begin, first_map, map_items_size);
  }

  mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)&event_pool[event_pool_maps_begin];
  MBNG_EVENT_ItemCopy2Pool(item, pool_item);
  event_pool_size += pool_item->len;
  ++event_pool_num_items;
  event_pool_maps_begin += pool_item_len;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Modifies an existing item in the pool
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemModify(mbng_event_item_t *item)
{
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    if( pool_item->id == item->id ) {
      u32 label_len = item->label ? (strlen(item->label)+1) : 0;
      u32 pool_item_len = MBNG_EVENT_ItemCalcPoolItemLen(item);

      if( pool_item_len > 255 )
	return -2; // too much data

      int len_diff = pool_item_len - pool_item->len;
      if( len_diff >= 0 && (event_pool_size+len_diff) > MBNG_EVENT_POOL_MAX_SIZE )
	return -2; // out of storage 

      if( len_diff != 0 ) {
	// make room
	u8 *old_next_pool_item = (u8 *)((u32)pool_item + pool_item->len);
	u8 *new_next_pool_item = (u8 *)((u32)pool_item + pool_item_len);
	u32 move_size = MBNG_EVENT_POOL_MAX_SIZE - ((u32)new_next_pool_item - (u32)&event_pool);

	//DEBUG_MSG("New Item changed size by %d bytes. Next Old Addr: 0x%08x, New: 0x%08x, move_size %d\n", len_diff, old_next_pool_item, new_next_pool_item, move_size);

	// make copy of label (because it's normaly located in pool!
	if( label_len >= 128 ) {
	  DEBUG_MSG("[MBNG_EVENT_ItemModify] Impossible Mission!\n"); // should never happen...
	}
	char label[129];
	if( item->label ) {
	  strcpy(label, item->label);
	  item->label = label;
	} else {
	  label[0] = 0;
	}

	// move memory
	memmove(new_next_pool_item, old_next_pool_item, move_size);

	// copy the modified item into pool
	MBNG_EVENT_ItemCopy2Pool(item, pool_item);

	// change event pool size and move map pointer
	event_pool_size += len_diff;
	event_pool_maps_begin += len_diff;
      } else {
	// no size change - copy new item directly into pool
	MBNG_EVENT_ItemCopy2Pool(item, pool_item);
      }

      return 0; // operation was successfull
    }
    pool_ptr += pool_item->len;
  }

  return -1; // not found
}

/////////////////////////////////////////////////////////////////////////////
//! Search an item in event pool based on ID
//! \returns 0 and copies item into *item if found
//! \returns -1 if item not found
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemSearchById(mbng_event_item_id_t id, mbng_event_item_t *item, u32 *continue_ix)
{
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i = 0;

  if( *continue_ix ) {
    // lower half: pointer offset to pool item
    // upper half: index of pool item
    pool_ptr += (*continue_ix & 0xffff);
    i = *continue_ix >> 16;
  }

  for(; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    if( pool_item->id == id ) {
      MBNG_EVENT_ItemCopy2User(pool_item, item);

      // pass pointer offset to pool item + index of pool item in continue_ix for continued search
      // skip this if the new values exceeding the 16bit boundary, or if this is the last pool item
      u32 next_pool_offset = (u32)pool_ptr - (u32)event_pool + pool_item->len;
      u32 next_pool_i = i + 1;
      if( next_pool_i > 65535 || next_pool_i >= event_pool_num_items || next_pool_offset > 65535 )
	*continue_ix = 0;
      else
	*continue_ix = (next_pool_i << 16) | next_pool_offset;

      return 0; // item found
    }
    pool_ptr += pool_item->len;
  }

  return -1; // not found
}


/////////////////////////////////////////////////////////////////////////////
//! Search an item in event pool based on the HW ID
//! Takes the selected bank into account (means: only an active item will be returned)
//! \returns 0 and copies item into *item if found
//! \returns -1 if item not found
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemSearchByHwId(mbng_event_item_id_t hw_id, mbng_event_item_t *item, u32 *continue_ix)
{
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i = 0;

  if( *continue_ix ) {
    // lower half: pointer offset to pool item
    // upper half: index of pool item
    pool_ptr += (*continue_ix & 0xffff);
    i = *continue_ix >> 16;
  }

  for(; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

    if( pool_item->flags.active && pool_item->hw_id == hw_id ) {
      MBNG_EVENT_ItemCopy2User(pool_item, item);

      // pass pointer offset to pool item + index of pool item in continue_ix for continued search
      // skip this if the new values exceeding the 16bit boundary, or if this is the last pool item
      u32 next_pool_offset = (u32)pool_ptr - (u32)event_pool + pool_item->len;
      u32 next_pool_i = i + 1;
      if( next_pool_i > 65535 || next_pool_i >= event_pool_num_items || next_pool_offset > 65535 )
	*continue_ix = 0;
      else
	*continue_ix = (next_pool_i << 16) | next_pool_offset;

      return 0; // item found
    }
    pool_ptr += pool_item->len;
  }

  return -1; // not found
}


/////////////////////////////////////////////////////////////////////////////
//! Iterates through the event pool to retrieve values/secondary values\n
//! Used to store a snapshot in \ref MBNG_FILE_S_Write
//! \returns 0 if a new value set has been found
//! \returns -1 if no new item
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemRetrieveValues(mbng_event_item_id_t *id, s16 *value, u8 *secondary_value, u32 *continue_ix)
{
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i = 0;

  if( *continue_ix ) {
    // lower half: pointer offset to pool item
    // upper half: index of pool item
    pool_ptr += (*continue_ix & 0xffff);
    i = *continue_ix >> 16;
  }

  for(; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

    *id = pool_item->id;
    *value = pool_item->value;
    *secondary_value = pool_item->secondary_value;

    // pass pointer offset to pool item + index of pool item in continue_ix for continued search
    // skip this if the new values exceeding the 16bit boundary, or if this is the last pool item
    u32 next_pool_offset = (u32)pool_ptr - (u32)event_pool + pool_item->len;
    u32 next_pool_i = i + 1;
    if( next_pool_i > 65535 || next_pool_i >= event_pool_num_items || next_pool_offset > 65535 )
      *continue_ix = 0;
    else
      *continue_ix = (next_pool_i << 16) | next_pool_offset;

    return 0; // item found
  }

  return -1; // no new item
}


/////////////////////////////////////////////////////////////////////////////
//! set a value directly
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemCopyValueToPool(mbng_event_item_t *item)
{
  // take over in pool item
  if( item->pool_address < MBNG_EVENT_POOL_MAX_SIZE ) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + item->pool_address);
    pool_item->value = item->value;
    if( item->flags.use_key_or_cc ) // only change secondary value if key_or_cc option selected
      pool_item->secondary_value = item->secondary_value;
    pool_item->flags.value_from_midi = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! activates/deactivates an event (like bank mechanism)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemSetActive(mbng_event_item_t *item, u8 active)
{
  // take over in pool item
  if( item->pool_address < MBNG_EVENT_POOL_MAX_SIZE ) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + item->pool_address);
    pool_item->flags.active = active;
    item->flags.active = active;

    if( active ) {
      u8 allow_refresh = 1;
      switch( item->hw_id & 0xf000 ) {
      case MBNG_EVENT_CONTROLLER_SENDER:   allow_refresh = 0; break;
      case MBNG_EVENT_CONTROLLER_RECEIVER: allow_refresh = 0; break;
      }

      if( allow_refresh ) {
	pool_item->flags.update_lcd = 1; // force LCD update
	MBNG_EVENT_ItemReceive(item, pool_item->value, 1, 1);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! locks/unlocks an event for write operations (when items have received a new value)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemSetLock(mbng_event_item_t *item, u8 lock)
{
  // take over in pool item
  if( item->pool_address < MBNG_EVENT_POOL_MAX_SIZE ) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + item->pool_address);
    pool_item->flags.write_locked = lock;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! \retval 0 if no matching condition
//! \retval 1 if matching condition (or no condition)
//! \retval 2 if matching condition and stop requested
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemCheckMatchingCondition(mbng_event_item_t *item)
{
  if( !item->cond.condition )
    return 1; // shortcut: no condition selected -> match

  // take value from another event?
  u16 cmp_value;
  if( item->cond.hw_id ) {
    mbng_event_item_t tmp_item;
    u32 continue_ix = 0;
    if( MBNG_EVENT_ItemSearchById(item->cond.hw_id, &tmp_item, &continue_ix) < 0 ) {
      return 0; // id doesn't exist -> no match
    }
    cmp_value = tmp_item.value;
  } else {
    // take my own value
    cmp_value = item->value;
  }

  // check condition:
  switch( item->cond.condition ) {
  case MBNG_EVENT_IF_COND_EQ:                  return (cmp_value == item->cond.value) ? 1 : 0;
  case MBNG_EVENT_IF_COND_EQ_STOP_ON_MATCH:    return (cmp_value == item->cond.value) ? 2 : 0;
  case MBNG_EVENT_IF_COND_UNEQ:                return (cmp_value != item->cond.value) ? 1 : 0;
  case MBNG_EVENT_IF_COND_UNEQ_STOP_ON_MATCH:  return (cmp_value != item->cond.value) ? 2 : 0;
  case MBNG_EVENT_IF_COND_LT:                  return (cmp_value <  item->cond.value) ? 1 : 0;
  case MBNG_EVENT_IF_COND_LT_STOP_ON_MATCH:    return (cmp_value <  item->cond.value) ? 2 : 0;
  case MBNG_EVENT_IF_COND_LEQ:                 return (cmp_value <= item->cond.value) ? 1 : 0;
  case MBNG_EVENT_IF_COND_LEQ_STOP_ON_MATCH:   return (cmp_value <= item->cond.value) ? 2 : 0;
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Sends the an item description to debug terminal
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemPrint(mbng_event_item_t *item, u8 all)
{
  if( all ) {
    DEBUG_MSG("id=%s:%d (hw_id=%s:%d)",
	      MBNG_EVENT_ItemControllerStrGet(item->id), item->id & 0xfff,
	      MBNG_EVENT_ItemControllerStrGet(item->hw_id), item->hw_id & 0xfff);

    DEBUG_MSG("  - bank=%d", item->bank);

    if( item->cond.condition ) {
      if( item->cond.hw_id ) {
	DEBUG_MSG("  - condition: if_%s=%s:%d:%d",
		  MBNG_EVENT_ItemConditionStrGet(item),
		  MBNG_EVENT_ItemControllerStrGet(item->cond.hw_id),
		  item->cond.hw_id & 0xfff, item->cond.value);
      } else {
	DEBUG_MSG("  - condition: if_%s=%d",
		  MBNG_EVENT_ItemConditionStrGet(item),
		  item->cond.value);
      }
    } else {
      DEBUG_MSG("  - condition: none");
    }

    DEBUG_MSG("  - fwd_id=%s:%d", MBNG_EVENT_ItemControllerStrGet(item->fwd_id), item->fwd_id & 0xfff);
    if( item->fwd_value != 0xffff ) {
      DEBUG_MSG("  - fwd_value=%d", item->fwd_value);
    } else {
      DEBUG_MSG("  - fwd_value=off");
    }
    DEBUG_MSG("  - fwd_to_lcd=%d", item->flags.fwd_to_lcd);
    DEBUG_MSG("  - type=%s", MBNG_EVENT_ItemTypeStrGet(item));

    switch( item->flags.type ) {
    case MBNG_EVENT_TYPE_NOTE_OFF:
    case MBNG_EVENT_TYPE_NOTE_ON:
    case MBNG_EVENT_TYPE_POLY_PRESSURE: {
      if( item->stream_size >= 2 ) {
	DEBUG_MSG("  - chn=%d", (item->stream[0] & 0xf)+1);
	if( item->flags.use_any_key_or_cc ) {
	  DEBUG_MSG("  - key=%d", item->stream[1]);
	} else {
	  DEBUG_MSG("  - key=any");
	}
	DEBUG_MSG("  - use_key_number=%d", item->flags.use_key_or_cc);
      }
    } break;

    case MBNG_EVENT_TYPE_CC: {
      if( item->stream_size >= 2 ) {
	DEBUG_MSG("  - chn=%d", (item->stream[0] & 0xf)+1);
	if( item->flags.use_any_key_or_cc ) {
	  DEBUG_MSG("  - cc=%d", item->stream[1]);
	} else {
	  DEBUG_MSG("  - cc=any");
	}

	DEBUG_MSG("  - use_cc_number=%d", item->flags.use_key_or_cc);
      }
    } break;

    case MBNG_EVENT_TYPE_PROGRAM_CHANGE:
    case MBNG_EVENT_TYPE_AFTERTOUCH:
    case MBNG_EVENT_TYPE_PITCHBEND: {
      if( item->stream_size >= 1 ) {
	DEBUG_MSG("  - chn=%d", (item->stream[0] & 0xf)+1);
      }
    } break;

    case MBNG_EVENT_TYPE_SYSEX: {
      if( item->stream_size ) {
	DEBUG_MSG("  - stream=");

	int pos;
	for(pos=0; pos<item->stream_size; ++pos) {
	  if( item->stream[pos] == 0xff ) { // meta indicator
	    char *var_str = (char *)MBNG_EVENT_ItemSysExVarStrGet(item, pos+1);
	    if( strcasecmp(var_str, "undef") == 0 ) {
	      DEBUG_MSG("      0xff 0x%02x", item->stream[pos+1]);
	    } else {
	      DEBUG_MSG("      ^%s", var_str);
	    }
	    ++pos;
	  } else {
	    DEBUG_MSG("      0x%02x", item->stream[pos]);
	  }
	}
      }
    } break;

    case MBNG_EVENT_TYPE_NRPN: {
      if( item->stream_size >= 3 ) {
	DEBUG_MSG("  - chn=%d", (item->stream[0] & 0xf)+1);
	DEBUG_MSG("  - nrpn=%d", item->stream[1] | (int)(item->stream[2] << 7));
	DEBUG_MSG("  - nrpn_format=%s", MBNG_EVENT_ItemNrpnFormatStrGet(item));
      }
    } break;

    case MBNG_EVENT_TYPE_META: {
      int i;
      for(i=0; i<item->stream_size; ++i) {
	mbng_event_meta_type_t meta_type = item->stream[i];

	char str[100]; str[0] = 0;
	u8 num_bytes = MBNG_EVENT_ItemMetaNumBytesGet(meta_type);
	int j;
	for(j=0; j<num_bytes; ++j) {
	  u8 meta_value = item->stream[++i];
	  sprintf(str, "%s:%d", str, (int)meta_value);
	}

	DEBUG_MSG("  - meta=%s%s", MBNG_EVENT_ItemMetaTypeStrGet(meta_type), str);
	DEBUG_MSG("  - use_cc_number=%d", item->flags.use_key_or_cc);
      }
    } break;
    
    case MBNG_EVENT_TYPE_MBFM: {
      int i;
      for(i=0; i<item->stream_size; ++i) {
	      mbng_event_mbfm_type_t mbfm_type = item->stream[i];

	      char str[100]; str[0] = 0;
	      u8 num_bytes = MBNG_EVENT_ItemMBFMNumBytesGet(mbfm_type);
	      int j;
	      for(j=0; j<num_bytes; ++j) {
	        u8 mbfm_value = item->stream[++i];
	        sprintf(str, "%s:%d", str, (int)mbfm_value);
	      }

	      DEBUG_MSG("  - mbfm=%s%s", MBNG_EVENT_ItemMBFMTypeStrGet(mbfm_type), str);
	      DEBUG_MSG("  - use_cc_number=%d", item->flags.use_key_or_cc);
      }
    } break;
    }


    {
      char ports_bin[17];
      int bit;
      for(bit=0; bit<16; ++bit) {
	ports_bin[bit] = (item->enabled_ports & (1 << bit)) ? '1' : '0';
      }
      ports_bin[16] = 0;

      DEBUG_MSG("  - ports=%s", ports_bin);
    }

    DEBUG_MSG("  - value=%d", item->value);
    DEBUG_MSG("  - secondary_value=%d", item->secondary_value);
    DEBUG_MSG("  - map=%d", item->map);
    DEBUG_MSG("  - min=%d", item->min);
    DEBUG_MSG("  - max=%d", item->max);
    DEBUG_MSG("  - offset=%d", item->offset);
    DEBUG_MSG("  - dimmed=%d", item->flags.dimmed);
    DEBUG_MSG("  - rgb=%d:%d:%d", item->rgb.r, item->rgb.g, item->rgb.b);
    DEBUG_MSG("  - syxdump_pos=%d:%d", item->syxdump_pos.receiver, item->syxdump_pos.pos);
    DEBUG_MSG("  - radio_group=%d", item->flags.radio_group);

    switch( item->id & 0xf000 ) {
    case MBNG_EVENT_CONTROLLER_SENDER: {
    } break;

    case MBNG_EVENT_CONTROLLER_RECEIVER: {
      DEBUG_MSG("  - emu_enc_mode=%s", MBNG_EVENT_ItemEncModeStrGet(item->custom_flags.RECEIVER.emu_enc_mode));
      DEBUG_MSG("  - emu_enc_hw_id=%d", item->custom_flags.RECEIVER.emu_enc_hw_id);
    } break;

    case MBNG_EVENT_CONTROLLER_BUTTON: {
      DEBUG_MSG("  - button_mode=%s", MBNG_EVENT_ItemButtonModeStrGet(item));
    } break;

    case MBNG_EVENT_CONTROLLER_LED: {
    } break;

    case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX: {
    } break;

    case MBNG_EVENT_CONTROLLER_LED_MATRIX: {
    } break;

    case MBNG_EVENT_CONTROLLER_ENC: {
      DEBUG_MSG("  - enc_mode=%s", MBNG_EVENT_ItemEncModeStrGet(item->custom_flags.ENC.enc_mode));
      DEBUG_MSG("  - enc_speed_mode=%s:%d", MBNG_EVENT_ItemEncSpeedModeStrGet(item), item->custom_flags.ENC.enc_speed_mode_par);
    } break;

    case MBNG_EVENT_CONTROLLER_AIN: {
      DEBUG_MSG("  - ain_mode=%s", MBNG_EVENT_ItemAinModeStrGet(item));
      DEBUG_MSG("  - ain_sensor_mode=%s", MBNG_EVENT_ItemAinSensorModeStrGet(item));
      DEBUG_MSG("  - ain_filter_delay_ms=%d", item->custom_flags.AIN.ain_filter_delay_ms);
    } break;

    case MBNG_EVENT_CONTROLLER_AINSER: {
      DEBUG_MSG("  - ain_mode=%s", MBNG_EVENT_ItemAinModeStrGet(item));
      DEBUG_MSG("  - ain_sensor_mode=%s", MBNG_EVENT_ItemAinSensorModeStrGet(item));
    } break;

    case MBNG_EVENT_CONTROLLER_MF: {
    } break;

    case MBNG_EVENT_CONTROLLER_CV: {
      if( item->custom_flags.CV.fwd_gate_to_dout_pin ) {
	DEBUG_MSG("  - fwd_gate_to_dout_pin=%d.D%d",
		  ((item->custom_flags.CV.fwd_gate_to_dout_pin-1) / 8) + 1,
		  7 - ((item->custom_flags.CV.fwd_gate_to_dout_pin-1) % 8));
      } else {
	DEBUG_MSG("  - fwd_gate_to_dout_pin=off");
      }

      DEBUG_MSG("  - cv_inverted=%d", item->custom_flags.CV.cv_inverted);
      DEBUG_MSG("  - cv_gate_inverted=%d", item->custom_flags.CV.cv_gate_inverted);
      DEBUG_MSG("  - cv_hz_v=%d", item->custom_flags.CV.cv_hz_v);
    } break;

    case MBNG_EVENT_CONTROLLER_KB: {
      DEBUG_MSG("  - kb_transpose=%d", (s8)item->custom_flags.KB.kb_transpose);
      DEBUG_MSG("  - kb_velocity_map=map%d", (s8)item->custom_flags.KB.kb_velocity_map);
    } break;
    }

    DEBUG_MSG("  - led_matrix_pattern=%s", MBNG_EVENT_ItemLedMatrixPatternStrGet(item));
    DEBUG_MSG("  - colour=%d", item->flags.colour);
    DEBUG_MSG("  - lcd_pos=%d:%d:%d", item->lcd+1, item->lcd_x+1, item->lcd_y+1);

    if( item->label && strlen(item->label) ) {
      DEBUG_MSG("  - label=\"%s\"", item->label);
    } else {
      DEBUG_MSG("  - label=none");
    }

  } else {
#if 0
    DEBUG_MSG("[EVENT] id=%s:%d %s stream:",
	      MBNG_EVENT_ItemControllerStrGet(item->id),
	      item->id & 0xfff,
	      MBNG_EVENT_ItemTypeStrGet(item));
    if( item->stream_size ) {
      MIOS32_MIDI_SendDebugHexDump(item->stream, item->stream_size);
    }
#else
    return DEBUG_MSG("[EVENT] id=%s:%d hw_id=%s:%d bank=%d fwd_id=%s:%d type=%s value=%d label=%s\n",
		     MBNG_EVENT_ItemControllerStrGet(item->id),
		     item->id & 0xfff,
		     MBNG_EVENT_ItemControllerStrGet(item->hw_id),
		     item->hw_id & 0xfff,
		     item->bank,
		     MBNG_EVENT_ItemControllerStrGet(item->fwd_id),
		     item->fwd_id & 0xfff,
		     MBNG_EVENT_ItemTypeStrGet(item),
		     item->value,
		     item->label ? item->label : "");
#endif
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Sends detailed item informations to debug terminal
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemSearchByIdAndPrint(mbng_event_item_id_t id)
{
  int num_found = 0;
  u8 *pool_ptr = (u8 *)&event_pool[0];

  int i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    if( pool_item->id == id ) {
      ++num_found;

      mbng_event_item_t item;
      MBNG_EVENT_ItemCopy2User(pool_item, &item);
      MBNG_EVENT_ItemPrint(&item, 1);
    }
    pool_ptr += pool_item->len;
  }

  return num_found;
}


/////////////////////////////////////////////////////////////////////////////
//! Sends detailed item informations to debug terminal
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemSearchByHwIdAndPrint(mbng_event_item_id_t hw_id)
{
  int num_found = 0;
  u8 *pool_ptr = (u8 *)&event_pool[0];

  int i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    if( pool_item->hw_id == hw_id ) {
      ++num_found;

      mbng_event_item_t item;
      MBNG_EVENT_ItemCopy2User(pool_item, &item);
      MBNG_EVENT_ItemPrint(&item, 1);
    }
    pool_ptr += pool_item->len;
  }

  return num_found;
}



/////////////////////////////////////////////////////////////////////////////
//! Logic Control Meters
/////////////////////////////////////////////////////////////////////////////

//! updates all events which are assigned to a meter value
static s32 MBNG_EVENT_LCMeters_Update(void)
{
  int matrix;
  mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[0];
  for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DOUT; ++matrix, ++m) {
    if( m->lc_meter_port ) {
      int port_ix = -1;
      if( m->lc_meter_port >= USB0 && m->lc_meter_port <= USB3 ) {
	port_ix = (m->lc_meter_port & 3) + 0;
      } else if( m->lc_meter_port >= UART0 && m->lc_meter_port <= UART3 ) {
	port_ix = (m->lc_meter_port & 3) + 4;
      }

      if( port_ix >= 0 ) {
	u8 color = 0;
	u8 level = 127;

	int row;
	u8 *meter_value = (u8 *)&lc_meters[port_ix][0];
	for(row=0; row<8; ++row, ++meter_value) {
	  MBNG_MATRIX_DOUT_PatternSet_LCMeter(matrix, color, row, *meter_value, level);
	}
      }
    }
  }

  return 0; // no error
}

//! reset meters
//! can also be called from external, e.g. used by mbng_file_c
s32 MBNG_EVENT_LCMeters_Init(void)
{
  int port_ix;
  for(port_ix=0; port_ix<LC_METERS_NUM_PORTS; ++port_ix) {
    int i;
    u8 *ptr = (u8 *)&lc_meters[port_ix];
    for(i=0; i<LC_METERS_NUM_ITEMS; ++i)
      *ptr++ = 0;
  }

  MBNG_EVENT_LCMeters_Update();

  return 0; // no error
}

//! set a meter of a given port
static s32 MBNG_EVENT_LCMeters_Set(u8 port_ix, u8 lc_meter_value)
{
  if( port_ix >= LC_METERS_NUM_PORTS )
    return -1;

  // lc_meter_value layout:
  // [7:4] channel
  // [3:0] 0x0..0xc: lever meter 0..100% (overload not cleared!)
  //       0xe: set overload
  //       0xf: clear overload
  u8 meter_ix = (lc_meter_value >> 4) & 0x7;
  u8 meter_value = lc_meter_value & 0xf;

  u8 *meter = (u8 *)&lc_meters[port_ix][meter_ix];
  u8 prev_value = *meter;

  if( meter_value == 0xe ) {
    //*meter |= 0x80;
    *meter = 0xff; // also set value to max
  } else if( meter_value == 0xf ) {
    *meter &= 0x7f;
  } else if( meter_value == 0xd ) {
    *meter |= 0x7f; // unspecified, set value to max
  } else if( meter_value <= 0xc ) {
    u8 scaled = ((u32)meter_value * 127) / 12;
    if( scaled > 127 ) // just to ensure...
      scaled = 127;
    *meter = (*meter & 0x80) | scaled;
  }

  if( *meter != prev_value )
    MBNG_EVENT_LCMeters_Update();

  return 0; // no error
}

//! should be called each 300 mS to handle the meter animation
static s32 MBNG_EVENT_LCMeters_Tick(void)
{
  u8 port_ix;
  u8 meter;
  u8 any_update = 0;

  u8 *ptr = (u8 *)&lc_meters[0][0];
  for(port_ix=0; port_ix<LC_METERS_NUM_PORTS; ++port_ix) {
    for(meter=0; meter<LC_METERS_NUM_ITEMS; ++meter, ++ptr) {
      u8 prev_value = *ptr;
      s32 value = *ptr & 0x7f;
      if( value != 0 ) {
	value -= 10; // 128/13 segments
	if( value < 0 )
	  value = 0;
	*ptr = (*ptr & 0x80) | value;
      }
      if( *ptr != prev_value )
	any_update |= 1;
    }
  }

  if( any_update )
    MBNG_EVENT_LCMeters_Update();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Enable/Get Learn Mode Status
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_MidiLearnModeSet(u8 mode)
{
  midi_learn_mode = mode;
  midi_learn_event.ALL = 0;
  midi_learn_min = 0xffff;
  midi_learn_max = 0xffff;
  midi_learn_nrpn_port = DEFAULT;
  midi_learn_nrpn_chn = 0;
  midi_learn_nrpn_valid = 0;
  midi_learn_nrpn_address = 0xffff;
  midi_learn_nrpn_value = 0xffff;

  return 0; // no error
}

s32 MBNG_EVENT_MidiLearnModeGet(void)
{
  return midi_learn_mode;
}

s32 MBNG_EVENT_MidiLearnStatusMsg(char *line1, char *line2)
{
  if( !midi_learn_event.ALL ) {
    if( !line1[0] ) {
      sprintf(line1, "Waiting for         ");
    }
    sprintf(line2, "MIDI Event...       ");
  } else {
    if( !line1[0] ) {
      if( midi_learn_mode >= 2 && midi_learn_nrpn_valid == 0x7 ) {
	sprintf(line1, "NRPN %5d = %5d  ", midi_learn_nrpn_address, midi_learn_nrpn_value);
      } else {
	sprintf(line1, "Received: %02X%02X%02X    ",
		midi_learn_event.evnt0,
		midi_learn_event.evnt1,
		midi_learn_event.evnt2);
      }
    }

    sprintf(line2, "Min:%5d Max:%5d ",
	    (midi_learn_min == 0xffff) ? 0 : midi_learn_min,
	    (midi_learn_max == 0xffff) ? 127 : midi_learn_max);
  }

  return 0; // no error
}

s32 MBNG_EVENT_MidiLearnIt(mbng_event_item_id_t hw_id)
{
  if( !midi_learn_mode )
    return 0; // nothing to learn...

  if( !midi_learn_event.ALL )
    return 0; // nothing to learn...

  mbng_event_item_id_t id = hw_id;

  u8 new_item = 0;
  mbng_event_item_t item;
  // note: currently only assigned to first found item
  u32 continue_ix = 0;
  if( MBNG_EVENT_ItemSearchByHwId(hw_id, &item, &continue_ix) < 0 ) {
    new_item = 1;

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("[MIDI_LEARN] creating a new item for hw_id=%s:%d\n", MBNG_EVENT_ItemControllerStrGet(hw_id), hw_id & 0xfff);
    }

    mbng_event_item_t tmp_item;
    u32 continue_id_ix = 0;
    while( MBNG_EVENT_ItemSearchById(id, &tmp_item, &continue_id_ix) >= 0 ) {
      if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("[MIDI_LEARN] id=%s:%d already allocated, trying next one...\n", MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff);
      }

      ++id;

      if( (id & 0xfff) == 0 ) {
	DEBUG_MSG("[MIDI_LEARN] ERROR: no free id found!\n");
	return -1; // unable to add new item
      }
    }
    
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("[MIDI_LEARN] the new item will get id=%s:%d\n", MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff);
    }

    MBNG_EVENT_ItemInit(&item, id);
  } else {
    if( item.flags.type == MBNG_EVENT_TYPE_META ) {
      if( item.stream_size && item.stream[0] == MBNG_EVENT_META_TYPE_MIDI_LEARN )
	return 0; // silently ignore

      if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("[MIDI_LEARN] id=%s:%d is assigned to a Meta Event which can't be overwritten!\n", 
		  MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff);
      }
      return -2; // meta items can't be changed
    }else if(item.flags.type == MBNG_EVENT_TYPE_MBFM){
      if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("[MIDI_LEARN] id=%s:%d is assigned to a MBFM Event which can't be overwritten!\n", 
		  MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff);
      }
      return -2; // mbfm items can't be changed
    }

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("[MIDI_LEARN] modify an existing item id=%s:%d for hw_id=%s:%d\n", 
		  MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff,
		  MBNG_EVENT_ItemControllerStrGet(hw_id), hw_id & 0xfff);
      }
    }
  }

  // modify item depending on the learnt event
  u8 item_modified = 0;
  u8 stream[4];
  if( midi_learn_mode == 1 || midi_learn_nrpn_valid != 0x7 ) {
    switch( midi_learn_event.type ) {
    case NoteOff:
    case NoteOn:
    case PolyPressure:
    case CC:
      item_modified = 1;
      item.flags.type = MBNG_EVENT_TYPE_NOTE_OFF + (midi_learn_event.type-8);
      item.stream_size = 2;
      item.stream = (u8 *)&stream;
      stream[0] = midi_learn_event.evnt0;
      stream[1] = midi_learn_event.evnt1;
      item.secondary_value = stream[1];
      break;

    case ProgramChange:
    case Aftertouch:
    case PitchBend:
      item_modified = 1;
      item.flags.type = MBNG_EVENT_TYPE_NOTE_OFF + (midi_learn_event.type-8);
      item.stream_size = 1;
      item.stream = (u8 *)&stream;
      stream[0] = midi_learn_event.evnt0;
      break;
    }
  } else {
    // valid NRPN
    item_modified = 1;
    item.flags.type = MBNG_EVENT_TYPE_NRPN;
    item.stream_size = 4;
    item.stream = (u8 *)&stream;
    stream[0] = midi_learn_event.evnt0;
    stream[1] = (midi_learn_nrpn_address & 0xff);
    stream[2] = (midi_learn_nrpn_address >> 8);
    stream[3] = MBNG_EVENT_NRPN_FORMAT_UNSIGNED;
    item.secondary_value = stream[1];
  }

  if( !item_modified ) {
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("[MIDI_LEARN] MIDI event not supported.\n");
    }
    return -3; // MIDI event not supported
  }

  // set min/max value
  item.min = (midi_learn_min == 0xffff) ? 0 : midi_learn_min;
  item.max = (midi_learn_max == 0xffff) ? 127 : midi_learn_max;

  // add/modify to/in pool
  if( new_item ) {
    // just add it to the pool...
    item.flags.active = 1;
    if( MBNG_EVENT_ItemAdd(&item) < 0 ) {
      DEBUG_MSG("[MBNG_FILE_C] ERROR: couldn't add id=%s:%d: out of memory!\n",
		MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff);
      MBNG_EVENT_MidiLearnModeSet(0); // disable learn mode
      return -3; // out of memory...
    }
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("[MIDI_LEARN] item id=%s:%d has been created.\n", MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff);
    }
  } else {
    // modify in pool
    s32 status;
    if( (status=MBNG_EVENT_ItemModify(&item)) < 0 ) {
      if( status == -1 ) {
	DEBUG_MSG("[MBNG_FILE_C] FATAL: unexpected malfunction of firmware while adding id=%s:%d!\n",
		  MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff);
      } else {
	DEBUG_MSG("[MBNG_FILE_C] ERROR: couldn't add id=%s:%d: out of memory!\n",
		  MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff);
      }
      MBNG_EVENT_MidiLearnModeSet(0); // disable learn mode
      return -3; // out of memory...
    }
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("[MIDI_LEARN] item id=%s:%d has been modified.\n", MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff);
    }
  }

  MBNG_EVENT_MidiLearnModeSet(0); // disable learn mode

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! \returns name of controller
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemControllerStrGet(mbng_event_item_id_t id)
{
  switch( id & 0xf000 ) {
  case MBNG_EVENT_CONTROLLER_DISABLED:      return "DISABLED";
  case MBNG_EVENT_CONTROLLER_SENDER:        return "SENDER";
  case MBNG_EVENT_CONTROLLER_RECEIVER:      return "RECEIVER";
  case MBNG_EVENT_CONTROLLER_BUTTON:        return "BUTTON";
  case MBNG_EVENT_CONTROLLER_LED:           return "LED";
  case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX: return "BUTTON_MATRIX";
  case MBNG_EVENT_CONTROLLER_LED_MATRIX:    return "LED_MATRIX";
  case MBNG_EVENT_CONTROLLER_ENC:           return "ENC";
  case MBNG_EVENT_CONTROLLER_AIN:           return "AIN";
  case MBNG_EVENT_CONTROLLER_AINSER:        return "AINSER";
  case MBNG_EVENT_CONTROLLER_MF:            return "MF";
  case MBNG_EVENT_CONTROLLER_CV:            return "CV";
  case MBNG_EVENT_CONTROLLER_KB:            return "KB";
  }
  return "DISABLED";
}

/////////////////////////////////////////////////////////////////////////////
//! \returns ID of controller given as string
/////////////////////////////////////////////////////////////////////////////
mbng_event_item_id_t MBNG_EVENT_ItemIdFromControllerStrGet(char *event)
{
  if( strcasecmp(event, "SENDER") == 0 )        return MBNG_EVENT_CONTROLLER_SENDER;
  if( strcasecmp(event, "RECEIVER") == 0 )      return MBNG_EVENT_CONTROLLER_RECEIVER;
  if( strcasecmp(event, "BUTTON") == 0 )        return MBNG_EVENT_CONTROLLER_BUTTON;
  if( strcasecmp(event, "LED") == 0 )           return MBNG_EVENT_CONTROLLER_LED;
  if( strcasecmp(event, "BUTTON_MATRIX") == 0 ) return MBNG_EVENT_CONTROLLER_BUTTON_MATRIX;
  if( strcasecmp(event, "LED_MATRIX") == 0 )    return MBNG_EVENT_CONTROLLER_LED_MATRIX;
  if( strcasecmp(event, "ENC") == 0 )           return MBNG_EVENT_CONTROLLER_ENC;
  if( strcasecmp(event, "AIN") == 0 )           return MBNG_EVENT_CONTROLLER_AIN;
  if( strcasecmp(event, "AINSER") == 0 )        return MBNG_EVENT_CONTROLLER_AINSER;
  if( strcasecmp(event, "MF") == 0 )            return MBNG_EVENT_CONTROLLER_MF;
  if( strcasecmp(event, "CV") == 0 )            return MBNG_EVENT_CONTROLLER_CV;
  if( strcasecmp(event, "KB") == 0 )            return MBNG_EVENT_CONTROLLER_KB;

  return MBNG_EVENT_CONTROLLER_DISABLED;
}

/////////////////////////////////////////////////////////////////////////////
//! \returns name of event type
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemTypeStrGet(mbng_event_item_t *item)
{
  switch( item->flags.type ) {
  case MBNG_EVENT_TYPE_NOTE_OFF:       return "NoteOff";
  case MBNG_EVENT_TYPE_NOTE_ON:        return "NoteOn";
  case MBNG_EVENT_TYPE_POLY_PRESSURE:  return "PolyPressure";
  case MBNG_EVENT_TYPE_CC:             return "CC";
  case MBNG_EVENT_TYPE_PROGRAM_CHANGE: return "ProgramChange";
  case MBNG_EVENT_TYPE_AFTERTOUCH:     return "Aftertouch";
  case MBNG_EVENT_TYPE_PITCHBEND:      return "Pitchbend";
  case MBNG_EVENT_TYPE_SYSEX:          return "SysEx";
  case MBNG_EVENT_TYPE_NRPN:           return "NRPN";
  case MBNG_EVENT_TYPE_META:           return "Meta";
  case MBNG_EVENT_TYPE_MBFM:           return "MBFM";
  }
  return "Disabled";
}

/////////////////////////////////////////////////////////////////////////////
//! \returns value of event type given as string
//! \returns <0 if invalid type
/////////////////////////////////////////////////////////////////////////////
mbng_event_type_t MBNG_EVENT_ItemTypeFromStrGet(char *event_type)
{
  if( strcasecmp(event_type, "NoteOff") == 0 )       return MBNG_EVENT_TYPE_NOTE_OFF;
  if( strcasecmp(event_type, "NoteOn") == 0 || strcasecmp(event_type, "Note") == 0 ) return MBNG_EVENT_TYPE_NOTE_ON;
  if( strcasecmp(event_type, "PolyPressure") == 0 )  return MBNG_EVENT_TYPE_POLY_PRESSURE;
  if( strcasecmp(event_type, "CC") == 0 )            return MBNG_EVENT_TYPE_CC;
  if( strcasecmp(event_type, "ProgramChange") == 0 ) return MBNG_EVENT_TYPE_PROGRAM_CHANGE;
  if( strcasecmp(event_type, "Aftertouch") == 0 )    return MBNG_EVENT_TYPE_AFTERTOUCH;
  if( strcasecmp(event_type, "Pitchbend") == 0 )     return MBNG_EVENT_TYPE_PITCHBEND;
  if( strcasecmp(event_type, "SysEx") == 0 )         return MBNG_EVENT_TYPE_SYSEX;
  if( strcasecmp(event_type, "NRPN") == 0 )          return MBNG_EVENT_TYPE_NRPN;
  if( strcasecmp(event_type, "Meta") == 0 )          return MBNG_EVENT_TYPE_META;
  if( strcasecmp(event_type, "MBFM") == 0 )          return MBNG_EVENT_TYPE_MBFM;

  return MBNG_EVENT_TYPE_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
//! \returns name of condition
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemConditionStrGet(mbng_event_item_t *item)
{
  switch( item->cond.condition ) {
  case MBNG_EVENT_IF_COND_EQ:                  return "equal";
  case MBNG_EVENT_IF_COND_EQ_STOP_ON_MATCH:    return "equal_stop_on_match";
  case MBNG_EVENT_IF_COND_UNEQ:                return "unequal";
  case MBNG_EVENT_IF_COND_UNEQ_STOP_ON_MATCH:  return "unequal_stop_on_match";
  case MBNG_EVENT_IF_COND_LT:                  return "less";
  case MBNG_EVENT_IF_COND_LT_STOP_ON_MATCH:    return "less_stop_on_match";
  case MBNG_EVENT_IF_COND_LEQ:                 return "lessequal";
  case MBNG_EVENT_IF_COND_LEQ_STOP_ON_MATCH:   return "lessequal_stop_on_match";
  }
  return "none";
}

/////////////////////////////////////////////////////////////////////////////
//! \returns value of condition given as string
//! \returns <0 if invalid type
/////////////////////////////////////////////////////////////////////////////
mbng_event_if_cond_t MBNG_EVENT_ItemConditionFromStrGet(char *condition)
{
  if( strcasecmp(condition, "equal") == 0 )                    return MBNG_EVENT_IF_COND_EQ;
  if( strcasecmp(condition, "equal_stop_on_match") == 0 )      return MBNG_EVENT_IF_COND_EQ_STOP_ON_MATCH;
  if( strcasecmp(condition, "unequal") == 0 )                  return MBNG_EVENT_IF_COND_UNEQ;
  if( strcasecmp(condition, "unequal_stop_on_match") == 0 )    return MBNG_EVENT_IF_COND_UNEQ_STOP_ON_MATCH;
  if( strcasecmp(condition, "less") == 0 )                     return MBNG_EVENT_IF_COND_LT;
  if( strcasecmp(condition, "less_stop_on_match") == 0 )       return MBNG_EVENT_IF_COND_LT_STOP_ON_MATCH;
  if( strcasecmp(condition, "lessequal") == 0 )                return MBNG_EVENT_IF_COND_LEQ;
  if( strcasecmp(condition, "lessequal_stop_on_match") == 0 )  return MBNG_EVENT_IF_COND_LEQ_STOP_ON_MATCH;

  return MBNG_EVENT_IF_COND_NONE;
}



/////////////////////////////////////////////////////////////////////////////
//! for button mode
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemButtonModeStrGet(mbng_event_item_t *item)
{
  mbng_event_button_mode_t button_mode = item->custom_flags.DIN.button_mode;
  switch( button_mode ) {
  case MBNG_EVENT_BUTTON_MODE_ON_OFF:  return "OnOff";
  case MBNG_EVENT_BUTTON_MODE_ON_ONLY: return "OnOnly";
  case MBNG_EVENT_BUTTON_MODE_TOGGLE:  return "Toggle";
  }
  return "Undefined";
}

mbng_event_button_mode_t MBNG_EVENT_ItemButtonModeFromStrGet(char *button_mode)
{
  if( strcasecmp(button_mode, "OnOff") == 0 )  return MBNG_EVENT_BUTTON_MODE_ON_OFF;
  if( strcasecmp(button_mode, "OnOnly") == 0 ) return MBNG_EVENT_BUTTON_MODE_ON_ONLY;
  if( strcasecmp(button_mode, "Toggle") == 0 ) return MBNG_EVENT_BUTTON_MODE_TOGGLE;

  return MBNG_EVENT_BUTTON_MODE_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
//! for AIN Mode
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemAinModeStrGet(mbng_event_item_t *item)
{
  mbng_event_ain_mode_t ain_mode = item->custom_flags.AIN.ain_mode;
  switch( ain_mode ) {
  case MBNG_EVENT_AIN_MODE_DIRECT:                return "Direct";
  case MBNG_EVENT_AIN_MODE_SNAP:                  return "Snap";
  case MBNG_EVENT_AIN_MODE_RELATIVE:              return "Relative";
  case MBNG_EVENT_AIN_MODE_PARALLAX:              return "Parallax";
  case MBNG_EVENT_AIN_MODE_SWITCH:              return "Switch";
  }
  return "Undefined";
}

mbng_event_ain_mode_t MBNG_EVENT_ItemAinModeFromStrGet(char *ain_mode)
{
  if( strcasecmp(ain_mode, "Direct") == 0 )       return MBNG_EVENT_AIN_MODE_DIRECT;
  if( strcasecmp(ain_mode, "Snap") == 0 )         return MBNG_EVENT_AIN_MODE_SNAP;
  if( strcasecmp(ain_mode, "Relative") == 0 )     return MBNG_EVENT_AIN_MODE_RELATIVE;
  if( strcasecmp(ain_mode, "Parallax") == 0 )     return MBNG_EVENT_AIN_MODE_PARALLAX;
  if( strcasecmp(ain_mode, "Switch") == 0 )       return MBNG_EVENT_AIN_MODE_SWITCH;

  return MBNG_EVENT_AIN_MODE_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
//! for AIN Sensor Mode
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemAinSensorModeStrGet(mbng_event_item_t *item)
{
  mbng_event_ain_sensor_mode_t ain_sensor_mode = item->custom_flags.AIN.ain_sensor_mode;
  switch( ain_sensor_mode ) {
  case MBNG_EVENT_AIN_SENSOR_MODE_NOTE_ON_OFF:    return "NoteOnOff";
  }
  return "None";
}

mbng_event_ain_sensor_mode_t MBNG_EVENT_ItemAinSensorModeFromStrGet(char *ain_sensor_mode)
{
  if( strcasecmp(ain_sensor_mode, "None") == 0 )         return MBNG_EVENT_AIN_SENSOR_MODE_NONE;
  if( strcasecmp(ain_sensor_mode, "NoteOnOff") == 0 )    return MBNG_EVENT_AIN_SENSOR_MODE_NOTE_ON_OFF;

  return MBNG_EVENT_AIN_SENSOR_MODE_NONE;
}


/////////////////////////////////////////////////////////////////////////////
//! for ENC Mode
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemEncModeStrGet(mbng_event_enc_mode_t enc_mode)
{
  switch( enc_mode ) {
  case MBNG_EVENT_ENC_MODE_ABSOLUTE:              return "Absolute";
  case MBNG_EVENT_ENC_MODE_40SPEED:               return "40Speed";
  case MBNG_EVENT_ENC_MODE_00SPEED:               return "00Speed";
  case MBNG_EVENT_ENC_MODE_INC00SPEED_DEC40SPEED: return "Inc00Speed_Dec40Speed";
  case MBNG_EVENT_ENC_MODE_INC41_DEC3F:           return "Inc41_Dec3F";
  case MBNG_EVENT_ENC_MODE_INC01_DEC7F:           return "Inc01_Dec7F";
  case MBNG_EVENT_ENC_MODE_INC01_DEC41:           return "Inc01_Dec41";
  }
  return "Undefined";
}

mbng_event_enc_mode_t MBNG_EVENT_ItemEncModeFromStrGet(char *enc_mode)
{
  if( strcasecmp(enc_mode, "Absolute") == 0 )               return MBNG_EVENT_ENC_MODE_ABSOLUTE;
  if( strcasecmp(enc_mode, "40Speed") == 0 )                return MBNG_EVENT_ENC_MODE_40SPEED;
  if( strcasecmp(enc_mode, "00Speed") == 0 )                return MBNG_EVENT_ENC_MODE_00SPEED;
  if( strcasecmp(enc_mode, "Inc00Speed_Dec40Speed") == 0 )  return MBNG_EVENT_ENC_MODE_INC00SPEED_DEC40SPEED;
  if( strcasecmp(enc_mode, "Inc41_Dec3F") == 0 )            return MBNG_EVENT_ENC_MODE_INC41_DEC3F;
  if( strcasecmp(enc_mode, "Inc01_Dec7F") == 0 )            return MBNG_EVENT_ENC_MODE_INC01_DEC7F;
  if( strcasecmp(enc_mode, "Inc01_Dec41") == 0 )            return MBNG_EVENT_ENC_MODE_INC01_DEC41;

  return MBNG_EVENT_ENC_MODE_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
//! for ENC Speed Mode
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemEncSpeedModeStrGet(mbng_event_item_t *item)
{
  mbng_event_enc_speed_mode_t enc_speed_mode = item->custom_flags.ENC.enc_speed_mode;
  switch( enc_speed_mode ) {
  case MBNG_EVENT_ENC_SPEED_MODE_AUTO:    return "Auto";
  case MBNG_EVENT_ENC_SPEED_MODE_SLOW:    return "Slow";
  case MBNG_EVENT_ENC_SPEED_MODE_NORMAL:  return "Normal";
  case MBNG_EVENT_ENC_SPEED_MODE_FAST:    return "Fast";
  }
  return "Undefined";
}

mbng_event_enc_speed_mode_t MBNG_EVENT_ItemEncSpeedModeFromStrGet(char *enc_speed_mode)
{
  if( strcasecmp(enc_speed_mode, "Auto") == 0 )     return MBNG_EVENT_ENC_SPEED_MODE_AUTO;
  if( strcasecmp(enc_speed_mode, "Slow") == 0 )     return MBNG_EVENT_ENC_SPEED_MODE_SLOW;
  if( strcasecmp(enc_speed_mode, "Normal") == 0 )   return MBNG_EVENT_ENC_SPEED_MODE_NORMAL;
  if( strcasecmp(enc_speed_mode, "Fast") == 0 )     return MBNG_EVENT_ENC_SPEED_MODE_FAST;

  return MBNG_EVENT_ENC_SPEED_MODE_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
//! for DOUT Matrix pattern selection
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemLedMatrixPatternStrGet(mbng_event_item_t *item)
{
  mbng_event_led_matrix_pattern_t led_matrix_pattern = item->flags.led_matrix_pattern;
  switch( led_matrix_pattern ) {
  case MBNG_EVENT_LED_MATRIX_PATTERN_UNDEFINED: return "Undefined";
  case MBNG_EVENT_LED_MATRIX_PATTERN_1:         return "1";
  case MBNG_EVENT_LED_MATRIX_PATTERN_2:         return "2";
  case MBNG_EVENT_LED_MATRIX_PATTERN_3:         return "3";
  case MBNG_EVENT_LED_MATRIX_PATTERN_4:         return "4";
  case MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT1:    return "Digit1";
  case MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT2:    return "Digit2";
  case MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT3:    return "Digit3";
  case MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT4:    return "Digit4";
  case MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT5:    return "Digit5";
  case MBNG_EVENT_LED_MATRIX_PATTERN_LC_DIGIT:  return "LcDigit";
  case MBNG_EVENT_LED_MATRIX_PATTERN_LC_AUTO:   return "LcAuto";
  }
  return "Undefined";
}

mbng_event_led_matrix_pattern_t MBNG_EVENT_ItemLedMatrixPatternFromStrGet(char *led_matrix_pattern)
{
  if( strcasecmp(led_matrix_pattern, "Undefined") == 0 ) return MBNG_EVENT_LED_MATRIX_PATTERN_UNDEFINED;
  if( strcasecmp(led_matrix_pattern, "1") == 0 )         return MBNG_EVENT_LED_MATRIX_PATTERN_1;
  if( strcasecmp(led_matrix_pattern, "2") == 0 )         return MBNG_EVENT_LED_MATRIX_PATTERN_2;
  if( strcasecmp(led_matrix_pattern, "3") == 0 )         return MBNG_EVENT_LED_MATRIX_PATTERN_3;
  if( strcasecmp(led_matrix_pattern, "4") == 0 )         return MBNG_EVENT_LED_MATRIX_PATTERN_4;
  if( strcasecmp(led_matrix_pattern, "Digit1") == 0 )    return MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT1;
  if( strcasecmp(led_matrix_pattern, "Digit2") == 0 )    return MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT2;
  if( strcasecmp(led_matrix_pattern, "Digit3") == 0 )    return MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT3;
  if( strcasecmp(led_matrix_pattern, "Digit4") == 0 )    return MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT4;
  if( strcasecmp(led_matrix_pattern, "Digit5") == 0 )    return MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT5;
  if( strcasecmp(led_matrix_pattern, "LcDigit") == 0 )   return MBNG_EVENT_LED_MATRIX_PATTERN_LC_DIGIT;
  if( strcasecmp(led_matrix_pattern, "LcAuto") == 0 )    return MBNG_EVENT_LED_MATRIX_PATTERN_LC_AUTO;

  return MBNG_EVENT_LED_MATRIX_PATTERN_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
//! for (N)RPN format
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemNrpnFormatStrGet(mbng_event_item_t *item)
{
  mbng_event_nrpn_format_t nrpn_format = (item->stream_size >= 3) ? item->stream[2] : MBNG_EVENT_NRPN_FORMAT_UNSIGNED;
  switch( nrpn_format ) {
  case MBNG_EVENT_NRPN_FORMAT_SIGNED: return "Signed";
  case MBNG_EVENT_NRPN_FORMAT_MSB_ONLY: return "MsbOnly";
  }
  return "Unsigned";
}

mbng_event_nrpn_format_t MBNG_EVENT_ItemNrpnFormatFromStrGet(char *nrpn_format)
{
  if( strcasecmp(nrpn_format, "Unsigned") == 0 ) return MBNG_EVENT_NRPN_FORMAT_UNSIGNED;
  if( strcasecmp(nrpn_format, "Signed") == 0 ) return MBNG_EVENT_NRPN_FORMAT_SIGNED;
  if( strcasecmp(nrpn_format, "MsbOnly") == 0 ) return MBNG_EVENT_NRPN_FORMAT_MSB_ONLY;

  return MBNG_EVENT_NRPN_FORMAT_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
//! for SysEx variables
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemSysExVarStrGet(mbng_event_item_t *item, u8 stream_pos)
{
  mbng_event_sysex_var_t sysex_var = (item->stream_size >= stream_pos) ? item->stream[stream_pos] : MBNG_EVENT_SYSEX_VAR_UNDEFINED;
  switch( sysex_var ) {
  case MBNG_EVENT_SYSEX_VAR_DEV:        return "dev";
  case MBNG_EVENT_SYSEX_VAR_PAT:        return "pat";
  case MBNG_EVENT_SYSEX_VAR_BNK:        return "bnk";
  case MBNG_EVENT_SYSEX_VAR_INS:        return "ins";
  case MBNG_EVENT_SYSEX_VAR_CHN:        return "chn";
  case MBNG_EVENT_SYSEX_VAR_CHK_START:  return "chk_start";
  case MBNG_EVENT_SYSEX_VAR_CHK:        return "chk";
  case MBNG_EVENT_SYSEX_VAR_CHK_INV:    return "chk_inv";
  case MBNG_EVENT_SYSEX_VAR_VAL:        return "val";
  case MBNG_EVENT_SYSEX_VAR_VAL_H:      return "val_h";
  case MBNG_EVENT_SYSEX_VAR_VAL_N1:     return "val_n1";
  case MBNG_EVENT_SYSEX_VAR_VAL_N2:     return "val_n2";
  case MBNG_EVENT_SYSEX_VAR_VAL_N3:     return "val_n3";
  case MBNG_EVENT_SYSEX_VAR_VAL_N4:     return "val_n4";
  case MBNG_EVENT_SYSEX_VAR_IGNORE:     return "ignore";
  case MBNG_EVENT_SYSEX_VAR_DUMP:       return "dump";
  case MBNG_EVENT_SYSEX_VAR_CURSOR:     return "cursor";
  case MBNG_EVENT_SYSEX_VAR_TXT:        return "txt";
  case MBNG_EVENT_SYSEX_VAR_TXT56:      return "txt56";
  case MBNG_EVENT_SYSEX_VAR_LABEL:      return "label";
  }
  return "undef";
}

mbng_event_sysex_var_t MBNG_EVENT_ItemSysExVarFromStrGet(char *sysex_var)
{
  if( strcasecmp(sysex_var, "dev") == 0 )        return MBNG_EVENT_SYSEX_VAR_DEV;
  if( strcasecmp(sysex_var, "pat") == 0 )        return MBNG_EVENT_SYSEX_VAR_PAT;
  if( strcasecmp(sysex_var, "bnk") == 0 )        return MBNG_EVENT_SYSEX_VAR_BNK;
  if( strcasecmp(sysex_var, "ins") == 0 )        return MBNG_EVENT_SYSEX_VAR_INS;
  if( strcasecmp(sysex_var, "chn") == 0 )        return MBNG_EVENT_SYSEX_VAR_CHN;
  if( strcasecmp(sysex_var, "chk_start") == 0 )  return MBNG_EVENT_SYSEX_VAR_CHK_START;
  if( strcasecmp(sysex_var, "chk") == 0 )        return MBNG_EVENT_SYSEX_VAR_CHK;
  if( strcasecmp(sysex_var, "chk_inv") == 0 )    return MBNG_EVENT_SYSEX_VAR_CHK_INV;
  if( strcasecmp(sysex_var, "val") == 0 || strcasecmp(sysex_var, "value") == 0 ) return MBNG_EVENT_SYSEX_VAR_VAL;
  if( strcasecmp(sysex_var, "val_h") == 0 )      return MBNG_EVENT_SYSEX_VAR_VAL_H;
  if( strcasecmp(sysex_var, "val_n1") == 0 )     return MBNG_EVENT_SYSEX_VAR_VAL_N1;
  if( strcasecmp(sysex_var, "val_n2") == 0 )     return MBNG_EVENT_SYSEX_VAR_VAL_N2;
  if( strcasecmp(sysex_var, "val_n3") == 0 )     return MBNG_EVENT_SYSEX_VAR_VAL_N3;
  if( strcasecmp(sysex_var, "val_n4") == 0 )     return MBNG_EVENT_SYSEX_VAR_VAL_N4;
  if( strcasecmp(sysex_var, "ignore") == 0 )     return MBNG_EVENT_SYSEX_VAR_IGNORE;
  if( strcasecmp(sysex_var, "dump") == 0 )       return MBNG_EVENT_SYSEX_VAR_DUMP;
  if( strcasecmp(sysex_var, "cursor") == 0 )     return MBNG_EVENT_SYSEX_VAR_CURSOR;
  if( strcasecmp(sysex_var, "txt") == 0 )        return MBNG_EVENT_SYSEX_VAR_TXT;
  if( strcasecmp(sysex_var, "txt56") == 0 )      return MBNG_EVENT_SYSEX_VAR_TXT56;
  if( strcasecmp(sysex_var, "label") == 0 )      return MBNG_EVENT_SYSEX_VAR_LABEL;
  return MBNG_EVENT_SYSEX_VAR_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
//! for Meta events
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemMetaTypeStrGet(mbng_event_meta_type_t meta_type)
{
  switch( meta_type ) {
  case MBNG_EVENT_META_TYPE_SET_BANK:            return "SetBank";
  case MBNG_EVENT_META_TYPE_DEC_BANK:            return "DecBank";
  case MBNG_EVENT_META_TYPE_INC_BANK:            return "IncBank";
  case MBNG_EVENT_META_TYPE_CYCLE_BANK:          return "CycleBank";

  case MBNG_EVENT_META_TYPE_SET_BANK_OF_HW_ID:   return "SetBankOfHwId";
  case MBNG_EVENT_META_TYPE_DEC_BANK_OF_HW_ID:   return "DecBankOfHwId";
  case MBNG_EVENT_META_TYPE_INC_BANK_OF_HW_ID:   return "IncBankOfHwId";
  case MBNG_EVENT_META_TYPE_CYCLE_BANK_OF_HW_ID: return "CycleBankOfHwId";

  case MBNG_EVENT_META_TYPE_SET_SNAPSHOT:        return "SetSnapshot";
  case MBNG_EVENT_META_TYPE_DEC_SNAPSHOT:        return "DecSnapshot";
  case MBNG_EVENT_META_TYPE_INC_SNAPSHOT:        return "IncSnapshot";
  case MBNG_EVENT_META_TYPE_CYCLE_SNAPSHOT:      return "CycleSnapshot";
  case MBNG_EVENT_META_TYPE_LOAD_SNAPSHOT:       return "LoadSnapshot";
  case MBNG_EVENT_META_TYPE_SAVE_SNAPSHOT:       return "SaveSnapshot";
  case MBNG_EVENT_META_TYPE_DUMP_SNAPSHOT:       return "DumpSnapshot";

  case MBNG_EVENT_META_TYPE_RETRIEVE_AIN_VALUES: return "RetrieveAinValues";
  case MBNG_EVENT_META_TYPE_RETRIEVE_AINSER_VALUES: return "RetrieveAinserValues";

  case MBNG_EVENT_META_TYPE_ENC_FAST:            return "EncFast";

  case MBNG_EVENT_META_TYPE_MIDI_LEARN:          return "MidiLearn";

  case MBNG_EVENT_META_TYPE_UPDATE_LCD:          return "UpdateLcd";

  case MBNG_EVENT_META_TYPE_RESET_METERS:        return "ResetMeters";

  case MBNG_EVENT_META_TYPE_SWAP_VALUES:         return "SwapValues";

  case MBNG_EVENT_META_TYPE_RUN_SECTION:         return "RunSection";
  case MBNG_EVENT_META_TYPE_RUN_STOP:            return "RunStop";

  case MBNG_EVENT_META_TYPE_MCLK_PLAY:           return "MClkPlay";
  case MBNG_EVENT_META_TYPE_MCLK_STOP:           return "MClkStop";
  case MBNG_EVENT_META_TYPE_MCLK_PLAYSTOP:       return "MClkPlayStop";
  case MBNG_EVENT_META_TYPE_MCLK_PAUSE:          return "MClkPause";
  case MBNG_EVENT_META_TYPE_MCLK_SET_TEMPO:      return "MClkSetTempo";
  case MBNG_EVENT_META_TYPE_MCLK_DEC_TEMPO:      return "MClkDecTempo";
  case MBNG_EVENT_META_TYPE_MCLK_INC_TEMPO:      return "MClkIncTempo";

  case MBNG_EVENT_META_TYPE_CV_PITCHBEND_14BIT:     return "CvPitchBend14Bit";
  case MBNG_EVENT_META_TYPE_CV_PITCHBEND_7BIT:      return "CvPitchBend7Bit";
  case MBNG_EVENT_META_TYPE_CV_PITCHRANGE:          return "CvPitchRange";
  case MBNG_EVENT_META_TYPE_CV_TRANSPOSE_OCTAVE:    return "CvTransposeOctave";
  case MBNG_EVENT_META_TYPE_CV_TRANSPOSE_SEMITONES: return "CvTransposeSemitones";
  
  /*
  case MBNG_EVENT_META_TYPE_SCS_ENC:             return "ScsEnc";
  case MBNG_EVENT_META_TYPE_SCS_MENU:            return "ScsMenu";
  case MBNG_EVENT_META_TYPE_SCS_SOFT1:           return "ScsSoft1";
  case MBNG_EVENT_META_TYPE_SCS_SOFT2:           return "ScsSoft2";
  case MBNG_EVENT_META_TYPE_SCS_SOFT3:           return "ScsSoft3";
  case MBNG_EVENT_META_TYPE_SCS_SOFT4:           return "ScsSoft4";
  case MBNG_EVENT_META_TYPE_SCS_SOFT5:           return "ScsSoft5";
  case MBNG_EVENT_META_TYPE_SCS_SOFT6:           return "ScsSoft6";
  case MBNG_EVENT_META_TYPE_SCS_SOFT7:           return "ScsSoft7";
  case MBNG_EVENT_META_TYPE_SCS_SOFT8:           return "ScsSoft8";
  case MBNG_EVENT_META_TYPE_SCS_SHIFT:           return "ScsShift";
  */
  }

  return "Undefined";
}

mbng_event_meta_type_t MBNG_EVENT_ItemMetaTypeFromStrGet(char *meta_type)
{
  if( strcasecmp(meta_type, "SetBank") == 0 )       return MBNG_EVENT_META_TYPE_SET_BANK;
  if( strcasecmp(meta_type, "DecBank") == 0 )       return MBNG_EVENT_META_TYPE_DEC_BANK;
  if( strcasecmp(meta_type, "IncBank") == 0 )       return MBNG_EVENT_META_TYPE_INC_BANK;
  if( strcasecmp(meta_type, "CycleBank") == 0 )     return MBNG_EVENT_META_TYPE_CYCLE_BANK;

  if( strcasecmp(meta_type, "SetBankOfHwId") == 0 )       return MBNG_EVENT_META_TYPE_SET_BANK_OF_HW_ID;
  if( strcasecmp(meta_type, "DecBankOfHwId") == 0 )       return MBNG_EVENT_META_TYPE_DEC_BANK_OF_HW_ID;
  if( strcasecmp(meta_type, "IncBankOfHwId") == 0 )       return MBNG_EVENT_META_TYPE_INC_BANK_OF_HW_ID;
  if( strcasecmp(meta_type, "CycleBankOfHwId") == 0 )     return MBNG_EVENT_META_TYPE_CYCLE_BANK_OF_HW_ID;

  if( strcasecmp(meta_type, "SetSnapshot") == 0 )   return MBNG_EVENT_META_TYPE_SET_SNAPSHOT;
  if( strcasecmp(meta_type, "DecSnapshot") == 0 )   return MBNG_EVENT_META_TYPE_DEC_SNAPSHOT;
  if( strcasecmp(meta_type, "IncSnapshot") == 0 )   return MBNG_EVENT_META_TYPE_INC_SNAPSHOT;
  if( strcasecmp(meta_type, "CycleSnapshot") == 0 ) return MBNG_EVENT_META_TYPE_CYCLE_SNAPSHOT;
  if( strcasecmp(meta_type, "LoadSnapshot") == 0 )  return MBNG_EVENT_META_TYPE_LOAD_SNAPSHOT;
  if( strcasecmp(meta_type, "SaveSnapshot") == 0 )  return MBNG_EVENT_META_TYPE_SAVE_SNAPSHOT;
  if( strcasecmp(meta_type, "DumpSnapshot") == 0 )  return MBNG_EVENT_META_TYPE_DUMP_SNAPSHOT;

  if( strcasecmp(meta_type, "RetrieveAinValues") == 0 ) return MBNG_EVENT_META_TYPE_RETRIEVE_AIN_VALUES;
  if( strcasecmp(meta_type, "RetrieveAinserValues") == 0 ) return MBNG_EVENT_META_TYPE_RETRIEVE_AINSER_VALUES;

  if( strcasecmp(meta_type, "EncFast") == 0 )       return MBNG_EVENT_META_TYPE_ENC_FAST;

  if( strcasecmp(meta_type, "MidiLearn") == 0 )     return MBNG_EVENT_META_TYPE_MIDI_LEARN;

  if( strcasecmp(meta_type, "UpdateLcd") == 0 )     return MBNG_EVENT_META_TYPE_UPDATE_LCD;

  if( strcasecmp(meta_type, "ResetMeters") == 0 )   return MBNG_EVENT_META_TYPE_RESET_METERS;

  if( strcasecmp(meta_type, "SwapValues") == 0 )    return MBNG_EVENT_META_TYPE_SWAP_VALUES;

  if( strcasecmp(meta_type, "RunSection") == 0 )    return MBNG_EVENT_META_TYPE_RUN_SECTION;
  if( strcasecmp(meta_type, "RunStop") == 0 )       return MBNG_EVENT_META_TYPE_RUN_STOP;

  if( strcasecmp(meta_type, "MClkPlay") == 0 )      return MBNG_EVENT_META_TYPE_MCLK_PLAY;
  if( strcasecmp(meta_type, "MClkStop") == 0 )      return MBNG_EVENT_META_TYPE_MCLK_STOP;
  if( strcasecmp(meta_type, "MClkPlayStop") == 0 )  return MBNG_EVENT_META_TYPE_MCLK_PLAYSTOP;
  if( strcasecmp(meta_type, "MClkPause") == 0 )     return MBNG_EVENT_META_TYPE_MCLK_PAUSE;
  if( strcasecmp(meta_type, "MClkSetTempo") == 0 )  return MBNG_EVENT_META_TYPE_MCLK_SET_TEMPO;
  if( strcasecmp(meta_type, "MClkDecTempo") == 0 )  return MBNG_EVENT_META_TYPE_MCLK_DEC_TEMPO;
  if( strcasecmp(meta_type, "MClkIncTempo") == 0 )  return MBNG_EVENT_META_TYPE_MCLK_INC_TEMPO;

  if( strcasecmp(meta_type, "CvPitchBend14Bit") == 0 )     return MBNG_EVENT_META_TYPE_CV_PITCHBEND_14BIT;
  if( strcasecmp(meta_type, "CvPitchBend7Bit") == 0 )      return MBNG_EVENT_META_TYPE_CV_PITCHBEND_7BIT;
  if( strcasecmp(meta_type, "CvPitchRange") == 0 )         return MBNG_EVENT_META_TYPE_CV_PITCHRANGE;
  if( strcasecmp(meta_type, "CvTransposeOctave") == 0 )    return MBNG_EVENT_META_TYPE_CV_TRANSPOSE_OCTAVE;
  if( strcasecmp(meta_type, "CvTransposeSemitones") == 0 ) return MBNG_EVENT_META_TYPE_CV_TRANSPOSE_SEMITONES;
  
  /*
  if( strcasecmp(meta_type, "ScsEnc") == 0 )        return MBNG_EVENT_META_TYPE_SCS_ENC;
  if( strcasecmp(meta_type, "ScsMenu") == 0 )       return MBNG_EVENT_META_TYPE_SCS_MENU;
  if( strcasecmp(meta_type, "ScsSoft1") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SOFT1;
  if( strcasecmp(meta_type, "ScsSoft2") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SOFT2;
  if( strcasecmp(meta_type, "ScsSoft3") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SOFT3;
  if( strcasecmp(meta_type, "ScsSoft4") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SOFT4;
  if( strcasecmp(meta_type, "ScsSoft5") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SOFT5;
  if( strcasecmp(meta_type, "ScsSoft6") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SOFT6;
  if( strcasecmp(meta_type, "ScsSoft7") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SOFT7;
  if( strcasecmp(meta_type, "ScsSoft8") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SOFT8;
  if( strcasecmp(meta_type, "ScsShift") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SHIFT;
  */
  return MBNG_EVENT_META_TYPE_UNDEFINED;
}


u8 MBNG_EVENT_ItemMetaNumBytesGet(mbng_event_meta_type_t meta_type)
{
  switch( meta_type ) {
  case MBNG_EVENT_META_TYPE_SET_BANK:            return 0;
  case MBNG_EVENT_META_TYPE_DEC_BANK:            return 0;
  case MBNG_EVENT_META_TYPE_INC_BANK:            return 0;
  case MBNG_EVENT_META_TYPE_CYCLE_BANK:          return 0;

  case MBNG_EVENT_META_TYPE_SET_BANK_OF_HW_ID:   return 1;
  case MBNG_EVENT_META_TYPE_DEC_BANK_OF_HW_ID:   return 1;
  case MBNG_EVENT_META_TYPE_INC_BANK_OF_HW_ID:   return 1;
  case MBNG_EVENT_META_TYPE_CYCLE_BANK_OF_HW_ID: return 1;

  case MBNG_EVENT_META_TYPE_SET_SNAPSHOT:        return 0;
  case MBNG_EVENT_META_TYPE_DEC_SNAPSHOT:        return 0;
  case MBNG_EVENT_META_TYPE_INC_SNAPSHOT:        return 0;
  case MBNG_EVENT_META_TYPE_CYCLE_SNAPSHOT:      return 0;
  case MBNG_EVENT_META_TYPE_LOAD_SNAPSHOT:       return 0;
  case MBNG_EVENT_META_TYPE_SAVE_SNAPSHOT:       return 0;
  case MBNG_EVENT_META_TYPE_DUMP_SNAPSHOT:       return 0;

  case MBNG_EVENT_META_TYPE_RETRIEVE_AIN_VALUES: return 0;
  case MBNG_EVENT_META_TYPE_RETRIEVE_AINSER_VALUES: return 0;

  case MBNG_EVENT_META_TYPE_ENC_FAST:            return 0;

  case MBNG_EVENT_META_TYPE_MIDI_LEARN:          return 0;

  case MBNG_EVENT_META_TYPE_UPDATE_LCD:          return 0;

  case MBNG_EVENT_META_TYPE_RESET_METERS:        return 0;

  case MBNG_EVENT_META_TYPE_SWAP_VALUES:         return 0;

  case MBNG_EVENT_META_TYPE_RUN_SECTION:         return 1;
  case MBNG_EVENT_META_TYPE_RUN_STOP:            return 0;

  case MBNG_EVENT_META_TYPE_MCLK_PLAY:           return 0;
  case MBNG_EVENT_META_TYPE_MCLK_STOP:           return 0;
  case MBNG_EVENT_META_TYPE_MCLK_PLAYSTOP:       return 0;
  case MBNG_EVENT_META_TYPE_MCLK_PAUSE:          return 0;
  case MBNG_EVENT_META_TYPE_MCLK_SET_TEMPO:      return 0;
  case MBNG_EVENT_META_TYPE_MCLK_DEC_TEMPO:      return 0;
  case MBNG_EVENT_META_TYPE_MCLK_INC_TEMPO:      return 0;

  case MBNG_EVENT_META_TYPE_CV_PITCHBEND_14BIT:     return 1;
  case MBNG_EVENT_META_TYPE_CV_PITCHBEND_7BIT:      return 1;
  case MBNG_EVENT_META_TYPE_CV_PITCHRANGE:          return 1;
  case MBNG_EVENT_META_TYPE_CV_TRANSPOSE_OCTAVE:    return 1;
  case MBNG_EVENT_META_TYPE_CV_TRANSPOSE_SEMITONES: return 1;
  
  /*
  case MBNG_EVENT_META_TYPE_SCS_ENC:             return 0;
  case MBNG_EVENT_META_TYPE_SCS_MENU:            return 0;
  case MBNG_EVENT_META_TYPE_SCS_SOFT1:           return 0;
  case MBNG_EVENT_META_TYPE_SCS_SOFT2:           return 0;
  case MBNG_EVENT_META_TYPE_SCS_SOFT3:           return 0;
  case MBNG_EVENT_META_TYPE_SCS_SOFT4:           return 0;
  case MBNG_EVENT_META_TYPE_SCS_SOFT5:           return 0;
  case MBNG_EVENT_META_TYPE_SCS_SOFT6:           return 0;
  case MBNG_EVENT_META_TYPE_SCS_SOFT7:           return 0;
  case MBNG_EVENT_META_TYPE_SCS_SOFT8:           return 0;
  case MBNG_EVENT_META_TYPE_SCS_SHIFT:           return 0;
  */
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! for MBFM events
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemMBFMTypeStrGet(mbng_event_mbfm_type_t mbfm_type)
{
  switch( mbfm_type ) {
  case MBNG_EVENT_MBFM_TYPE_SELECTOPL3:          return "SelectOPL3";
  case MBNG_EVENT_MBFM_TYPE_SELECTVOICE:         return "SelectVoice";
  case MBNG_EVENT_MBFM_TYPE_SELECTMODE:          return "SelectMode";
  case MBNG_EVENT_MBFM_TYPE_BTNSOFTKEY:          return "BtnSoftkey";
  case MBNG_EVENT_MBFM_TYPE_BTNSELECT:           return "BtnSelect";
  case MBNG_EVENT_MBFM_TYPE_BTNMENU:             return "BtnMenu";
  case MBNG_EVENT_MBFM_TYPE_BTNDUPL:             return "BtnDupl";
  case MBNG_EVENT_MBFM_TYPE_BTNLINK:             return "BtnLink";
  case MBNG_EVENT_MBFM_TYPE_BTNRETRIG:           return "BtnRetrig";
  case MBNG_EVENT_MBFM_TYPE_BTNDLYSCALE:         return "BtnDlyScale";
  case MBNG_EVENT_MBFM_TYPE_BTN4OP:              return "Btn4Op";
  case MBNG_EVENT_MBFM_TYPE_BTNALG:              return "BtnAlg";
  case MBNG_EVENT_MBFM_TYPE_BTNDEST:             return "BtnDest";
  case MBNG_EVENT_MBFM_TYPE_BTNOPPARAM:          return "BtnOpParam";
  case MBNG_EVENT_MBFM_TYPE_BTNOPWAVE:           return "BtnOpWave";
  case MBNG_EVENT_MBFM_TYPE_BTNOPMUTE:           return "BtnOpMute";
  case MBNG_EVENT_MBFM_TYPE_BTNEGASSIGN:         return "BtnEGAssign";
  case MBNG_EVENT_MBFM_TYPE_BTNLFOASSIGN:        return "BtnLFOAssign";
  case MBNG_EVENT_MBFM_TYPE_BTNLFOWAVE:          return "BtnLFOWave";
  case MBNG_EVENT_MBFM_TYPE_BTNWTASSIGN:         return "BtnWTAssign";
  case MBNG_EVENT_MBFM_TYPE_BTNVELASSIGN:        return "BtnVelAssign";
  case MBNG_EVENT_MBFM_TYPE_BTNMODASSIGN:        return "BtnModAssign";
  case MBNG_EVENT_MBFM_TYPE_BTNVARIASSIGN:       return "BtnVariAssign";
  case MBNG_EVENT_MBFM_TYPE_BTNSEQREC:           return "BtnSeqRec";
  case MBNG_EVENT_MBFM_TYPE_BTNSEQPLAY:          return "BtnSeqPlay";
  case MBNG_EVENT_MBFM_TYPE_BTNSEQBACK:          return "BtnSeqBack";
  case MBNG_EVENT_MBFM_TYPE_BTNSEQMUTE:          return "BtnSeqMute";
  case MBNG_EVENT_MBFM_TYPE_BTNSEQSOLO:          return "BtnSeqSolo";
  case MBNG_EVENT_MBFM_TYPE_BTNSEQVEL:           return "BtnSeqVel";
  case MBNG_EVENT_MBFM_TYPE_BTNSEQVARI:          return "BtnSeqVari";
  case MBNG_EVENT_MBFM_TYPE_BTNSEQGATE:          return "BtnSeqGate";
  case MBNG_EVENT_MBFM_TYPE_BTNSEQTRIGGER:       return "BtnSeqTrigger";
  case MBNG_EVENT_MBFM_TYPE_BTNSEQMEASURE:       return "BtnSeqMeasure";
  case MBNG_EVENT_MBFM_TYPE_BTNSEQSECTION:       return "BtnSeqSection";

  case MBNG_EVENT_MBFM_TYPE_SETDATAWHEEL:           return "SetDatawheel";
  case MBNG_EVENT_MBFM_TYPE_SETVOICETRANSPOSE:      return "SetVoiceTranspose";
  case MBNG_EVENT_MBFM_TYPE_SETVOICETUNE:           return "SetVoiceTune";
  case MBNG_EVENT_MBFM_TYPE_SETVOICEPORTA:          return "SetVoicePorta";
  case MBNG_EVENT_MBFM_TYPE_SETVOICEDLYTIME:        return "SetVoiceDlyTime";
  case MBNG_EVENT_MBFM_TYPE_SETVOICEFB:             return "SetVoiceFB";
  case MBNG_EVENT_MBFM_TYPE_SETOPFMULT:             return "SetOpFMult";
  case MBNG_EVENT_MBFM_TYPE_SETOPATK:               return "SetOpAtk";
  case MBNG_EVENT_MBFM_TYPE_SETOPDEC:               return "SetOpDec";
  case MBNG_EVENT_MBFM_TYPE_SETOPSUS:               return "SetOpSus";
  case MBNG_EVENT_MBFM_TYPE_SETOPREL:               return "SetOpRel";
  case MBNG_EVENT_MBFM_TYPE_SETOPVOLUME:            return "SetOpVolume";
  case MBNG_EVENT_MBFM_TYPE_SETEGATK:               return "SetEGAtk";
  case MBNG_EVENT_MBFM_TYPE_SETEGDEC1:              return "SetEGDec1";
  case MBNG_EVENT_MBFM_TYPE_SETEGLEVEL:             return "SetEGLevel";
  case MBNG_EVENT_MBFM_TYPE_SETEGDEC2:              return "SetEGDec2";
  case MBNG_EVENT_MBFM_TYPE_SETEGSUS:               return "SetEGSus";
  case MBNG_EVENT_MBFM_TYPE_SETEGREL:               return "SetEGRel";
  case MBNG_EVENT_MBFM_TYPE_SETLFOFREQ:             return "SetLFOFreq";
  case MBNG_EVENT_MBFM_TYPE_SETLFODELAY:            return "SetLFODelay";
  
  case MBNG_EVENT_MBFM_TYPE_INCDATAWHEEL:           return "IncDatawheel";
  case MBNG_EVENT_MBFM_TYPE_INCVOICETRANSPOSE:      return "IncVoiceTranspose";
  case MBNG_EVENT_MBFM_TYPE_INCVOICETUNE:           return "IncVoiceTune";
  case MBNG_EVENT_MBFM_TYPE_INCVOICEPORTA:          return "IncVoicePorta";
  case MBNG_EVENT_MBFM_TYPE_INCVOICEDLYTIME:        return "IncVoiceDlyTime";
  case MBNG_EVENT_MBFM_TYPE_INCVOICEFB:             return "IncVoiceFB";
  case MBNG_EVENT_MBFM_TYPE_INCOPFMULT:             return "IncOpFMult";
  case MBNG_EVENT_MBFM_TYPE_INCOPATK:               return "IncOpAtk";
  case MBNG_EVENT_MBFM_TYPE_INCOPDEC:               return "IncOpDec";
  case MBNG_EVENT_MBFM_TYPE_INCOPSUS:               return "IncOpSus";
  case MBNG_EVENT_MBFM_TYPE_INCOPREL:               return "IncOpRel";
  case MBNG_EVENT_MBFM_TYPE_INCOPVOLUME:            return "IncOpVolume";
  case MBNG_EVENT_MBFM_TYPE_INCEGATK:               return "IncEGAtk";
  case MBNG_EVENT_MBFM_TYPE_INCEGDEC1:              return "IncEGDec1";
  case MBNG_EVENT_MBFM_TYPE_INCEGLEVEL:             return "IncEGLevel";
  case MBNG_EVENT_MBFM_TYPE_INCEGDEC2:              return "IncEGDec2";
  case MBNG_EVENT_MBFM_TYPE_INCEGSUS:               return "IncEGSus";
  case MBNG_EVENT_MBFM_TYPE_INCEGREL:               return "IncEGRel";
  case MBNG_EVENT_MBFM_TYPE_INCLFOFREQ:             return "IncLFOFreq";
  case MBNG_EVENT_MBFM_TYPE_INCLFODELAY:            return "IncLFODelay";

  case MBNG_EVENT_MBFM_TYPE_DISPVOICE:           return "DispVoice";
  case MBNG_EVENT_MBFM_TYPE_DISPVALUE:           return "DispValue";
  case MBNG_EVENT_MBFM_TYPE_CLEARVOICE:          return "ClearVoice";
  case MBNG_EVENT_MBFM_TYPE_CLEARVALUE:          return "ClearValue";
  case MBNG_EVENT_MBFM_TYPE_VOICELEDRED:         return "VoiceLEDRed";
  case MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN:       return "VoiceLEDGreen";
  case MBNG_EVENT_MBFM_TYPE_ALGOPLEDRED:         return "AlgOpLEDRed";
  case MBNG_EVENT_MBFM_TYPE_ALGOPLEDGREEN:       return "AlgOpLEDGreen";
  case MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED:       return "SeqTriggerLED";
  case MBNG_EVENT_MBFM_TYPE_ISMODE:              return "IsMode";
  case MBNG_EVENT_MBFM_TYPE_ISDUPL:              return "IsDupl";
  case MBNG_EVENT_MBFM_TYPE_ISLINK:              return "IsLink";
  case MBNG_EVENT_MBFM_TYPE_ISRETRIG:            return "IsRetrig";
  case MBNG_EVENT_MBFM_TYPE_ISDLYSCALE:          return "IsDlyScale";
  case MBNG_EVENT_MBFM_TYPE_ISCURVOICE4OP:       return "IsCurVoice4Op";
  case MBNG_EVENT_MBFM_TYPE_ISSOMEVOICE4OP:      return "IsSomeVoice4Op";
  case MBNG_EVENT_MBFM_TYPE_ISRECORDING:         return "IsRecording";
  case MBNG_EVENT_MBFM_TYPE_ISPLAYING:           return "IsPlaying";
  case MBNG_EVENT_MBFM_TYPE_ISMUTE:              return "IsMute";
  case MBNG_EVENT_MBFM_TYPE_ISSOLO:              return "IsSolo";
  case MBNG_EVENT_MBFM_TYPE_ISMEASURE:           return "IsMeasure";
  case MBNG_EVENT_MBFM_TYPE_ISSEQVEL:            return "IsSeqVel";
  case MBNG_EVENT_MBFM_TYPE_ISSEQVARI:           return "IsSeqVari";
  case MBNG_EVENT_MBFM_TYPE_ISSEQGATE:           return "IsSeqGate";
  case MBNG_EVENT_MBFM_TYPE_ISALGFB:             return "IsAlgFB";
  case MBNG_EVENT_MBFM_TYPE_ISALGFM:             return "IsAlgFM";
  case MBNG_EVENT_MBFM_TYPE_ISALGOUT:            return "IsAlgOut";

  case MBNG_EVENT_MBFM_TYPE_CONTROLMOD:          return "ControlMod";
  case MBNG_EVENT_MBFM_TYPE_CONTROLVARI:         return "ControlVari";
  case MBNG_EVENT_MBFM_TYPE_CONTROLSUS:          return "ControlSus";
  }

  return "Undefined";
}

mbng_event_mbfm_type_t MBNG_EVENT_ItemMBFMTypeFromStrGet(char *mbfm_type)
{
  if( strcasecmp(mbfm_type, "SelectOPL3") == 0)     return MBNG_EVENT_MBFM_TYPE_SELECTOPL3;
  if( strcasecmp(mbfm_type, "SelectVoice") == 0)    return MBNG_EVENT_MBFM_TYPE_SELECTVOICE;
  if( strcasecmp(mbfm_type, "SelectMode") == 0)     return MBNG_EVENT_MBFM_TYPE_SELECTMODE;
  if( strcasecmp(mbfm_type, "BtnSoftkey") == 0)     return MBNG_EVENT_MBFM_TYPE_BTNSOFTKEY;
  if( strcasecmp(mbfm_type, "BtnSelect") == 0)      return MBNG_EVENT_MBFM_TYPE_BTNSELECT;
  if( strcasecmp(mbfm_type, "BtnMenu") == 0)        return MBNG_EVENT_MBFM_TYPE_BTNMENU;
  if( strcasecmp(mbfm_type, "BtnDupl") == 0)        return MBNG_EVENT_MBFM_TYPE_BTNDUPL;
  if( strcasecmp(mbfm_type, "BtnLink") == 0)        return MBNG_EVENT_MBFM_TYPE_BTNLINK;
  if( strcasecmp(mbfm_type, "BtnRetrig") == 0)      return MBNG_EVENT_MBFM_TYPE_BTNRETRIG;
  if( strcasecmp(mbfm_type, "BtnDlyScale") == 0)    return MBNG_EVENT_MBFM_TYPE_BTNDLYSCALE;
  if( strcasecmp(mbfm_type, "Btn4Op") == 0)         return MBNG_EVENT_MBFM_TYPE_BTN4OP;
  if( strcasecmp(mbfm_type, "BtnAlg") == 0)         return MBNG_EVENT_MBFM_TYPE_BTNALG;
  if( strcasecmp(mbfm_type, "BtnDest") == 0)        return MBNG_EVENT_MBFM_TYPE_BTNDEST;
  if( strcasecmp(mbfm_type, "BtnOpParam") == 0)     return MBNG_EVENT_MBFM_TYPE_BTNOPPARAM;
  if( strcasecmp(mbfm_type, "BtnOpWave") == 0)      return MBNG_EVENT_MBFM_TYPE_BTNOPWAVE;
  if( strcasecmp(mbfm_type, "BtnOpMute") == 0)      return MBNG_EVENT_MBFM_TYPE_BTNOPMUTE;
  if( strcasecmp(mbfm_type, "BtnEGAssign") == 0)    return MBNG_EVENT_MBFM_TYPE_BTNEGASSIGN;
  if( strcasecmp(mbfm_type, "BtnLFOAssign") == 0)   return MBNG_EVENT_MBFM_TYPE_BTNLFOASSIGN;
  if( strcasecmp(mbfm_type, "BtnLFOWave") == 0)     return MBNG_EVENT_MBFM_TYPE_BTNLFOWAVE;
  if( strcasecmp(mbfm_type, "BtnWTAssign") == 0)    return MBNG_EVENT_MBFM_TYPE_BTNWTASSIGN;
  if( strcasecmp(mbfm_type, "BtnVelAssign") == 0)   return MBNG_EVENT_MBFM_TYPE_BTNVELASSIGN;
  if( strcasecmp(mbfm_type, "BtnModAssign") == 0)   return MBNG_EVENT_MBFM_TYPE_BTNMODASSIGN;
  if( strcasecmp(mbfm_type, "BtnVariAssign") == 0)  return MBNG_EVENT_MBFM_TYPE_BTNVARIASSIGN;
  if( strcasecmp(mbfm_type, "BtnSeqRec") == 0)      return MBNG_EVENT_MBFM_TYPE_BTNSEQREC;
  if( strcasecmp(mbfm_type, "BtnSeqPlay") == 0)     return MBNG_EVENT_MBFM_TYPE_BTNSEQPLAY;
  if( strcasecmp(mbfm_type, "BtnSeqBack") == 0)     return MBNG_EVENT_MBFM_TYPE_BTNSEQBACK;
  if( strcasecmp(mbfm_type, "BtnSeqMute") == 0)     return MBNG_EVENT_MBFM_TYPE_BTNSEQMUTE;
  if( strcasecmp(mbfm_type, "BtnSeqSolo") == 0)     return MBNG_EVENT_MBFM_TYPE_BTNSEQSOLO;
  if( strcasecmp(mbfm_type, "BtnSeqVel") == 0)      return MBNG_EVENT_MBFM_TYPE_BTNSEQVEL;
  if( strcasecmp(mbfm_type, "BtnSeqVari") == 0)     return MBNG_EVENT_MBFM_TYPE_BTNSEQVARI;
  if( strcasecmp(mbfm_type, "BtnSeqGate") == 0)     return MBNG_EVENT_MBFM_TYPE_BTNSEQGATE;
  if( strcasecmp(mbfm_type, "BtnSeqTrigger") == 0)  return MBNG_EVENT_MBFM_TYPE_BTNSEQTRIGGER;
  if( strcasecmp(mbfm_type, "BtnSeqMeasure") == 0)  return MBNG_EVENT_MBFM_TYPE_BTNSEQMEASURE;
  if( strcasecmp(mbfm_type, "BtnSeqSection") == 0)  return MBNG_EVENT_MBFM_TYPE_BTNSEQSECTION;

  if( strcasecmp(mbfm_type, "SetDatawheel") == 0)      return MBNG_EVENT_MBFM_TYPE_SETDATAWHEEL;
  if( strcasecmp(mbfm_type, "SetVoiceTranspose") == 0) return MBNG_EVENT_MBFM_TYPE_SETVOICETRANSPOSE;
  if( strcasecmp(mbfm_type, "SetVoiceTune") == 0)      return MBNG_EVENT_MBFM_TYPE_SETVOICETUNE;
  if( strcasecmp(mbfm_type, "SetVoicePorta") == 0)     return MBNG_EVENT_MBFM_TYPE_SETVOICEPORTA;
  if( strcasecmp(mbfm_type, "SetVoiceDlyTime") == 0)   return MBNG_EVENT_MBFM_TYPE_SETVOICEDLYTIME;
  if( strcasecmp(mbfm_type, "SetVoiceFB") == 0)        return MBNG_EVENT_MBFM_TYPE_SETVOICEFB;
  if( strcasecmp(mbfm_type, "SetOpFMult") == 0)        return MBNG_EVENT_MBFM_TYPE_SETOPFMULT;
  if( strcasecmp(mbfm_type, "SetOpAtk") == 0)          return MBNG_EVENT_MBFM_TYPE_SETOPATK;
  if( strcasecmp(mbfm_type, "SetOpDec") == 0)          return MBNG_EVENT_MBFM_TYPE_SETOPDEC;
  if( strcasecmp(mbfm_type, "SetOpSus") == 0)          return MBNG_EVENT_MBFM_TYPE_SETOPSUS;
  if( strcasecmp(mbfm_type, "SetOpRel") == 0)          return MBNG_EVENT_MBFM_TYPE_SETOPREL;
  if( strcasecmp(mbfm_type, "SetOpVolume") == 0)       return MBNG_EVENT_MBFM_TYPE_SETOPVOLUME;
  if( strcasecmp(mbfm_type, "SetEGAtk") == 0)          return MBNG_EVENT_MBFM_TYPE_SETEGATK;
  if( strcasecmp(mbfm_type, "SetEGDec1") == 0)         return MBNG_EVENT_MBFM_TYPE_SETEGDEC1;
  if( strcasecmp(mbfm_type, "SetEGLevel") == 0)        return MBNG_EVENT_MBFM_TYPE_SETEGLEVEL;
  if( strcasecmp(mbfm_type, "SetEGDec2") == 0)         return MBNG_EVENT_MBFM_TYPE_SETEGDEC2;
  if( strcasecmp(mbfm_type, "SetEGSus") == 0)          return MBNG_EVENT_MBFM_TYPE_SETEGSUS;
  if( strcasecmp(mbfm_type, "SetEGRel") == 0)          return MBNG_EVENT_MBFM_TYPE_SETEGREL;
  if( strcasecmp(mbfm_type, "SetLFOFreq") == 0)        return MBNG_EVENT_MBFM_TYPE_SETLFOFREQ;
  if( strcasecmp(mbfm_type, "SetLFODelay") == 0)       return MBNG_EVENT_MBFM_TYPE_SETLFODELAY;
  
  if( strcasecmp(mbfm_type, "IncDatawheel") == 0)      return MBNG_EVENT_MBFM_TYPE_INCDATAWHEEL;
  if( strcasecmp(mbfm_type, "IncVoiceTranspose") == 0) return MBNG_EVENT_MBFM_TYPE_INCVOICETRANSPOSE;
  if( strcasecmp(mbfm_type, "IncVoiceTune") == 0)      return MBNG_EVENT_MBFM_TYPE_INCVOICETUNE;
  if( strcasecmp(mbfm_type, "IncVoicePorta") == 0)     return MBNG_EVENT_MBFM_TYPE_INCVOICEPORTA;
  if( strcasecmp(mbfm_type, "IncVoiceDlyTime") == 0)   return MBNG_EVENT_MBFM_TYPE_INCVOICEDLYTIME;
  if( strcasecmp(mbfm_type, "IncVoiceFB") == 0)        return MBNG_EVENT_MBFM_TYPE_INCVOICEFB;
  if( strcasecmp(mbfm_type, "IncOpFMult") == 0)        return MBNG_EVENT_MBFM_TYPE_INCOPFMULT;
  if( strcasecmp(mbfm_type, "IncOpAtk") == 0)          return MBNG_EVENT_MBFM_TYPE_INCOPATK;
  if( strcasecmp(mbfm_type, "IncOpDec") == 0)          return MBNG_EVENT_MBFM_TYPE_INCOPDEC;
  if( strcasecmp(mbfm_type, "IncOpSus") == 0)          return MBNG_EVENT_MBFM_TYPE_INCOPSUS;
  if( strcasecmp(mbfm_type, "IncOpRel") == 0)          return MBNG_EVENT_MBFM_TYPE_INCOPREL;
  if( strcasecmp(mbfm_type, "IncOpVolume") == 0)       return MBNG_EVENT_MBFM_TYPE_INCOPVOLUME;
  if( strcasecmp(mbfm_type, "IncEGAtk") == 0)          return MBNG_EVENT_MBFM_TYPE_INCEGATK;
  if( strcasecmp(mbfm_type, "IncEGDec1") == 0)         return MBNG_EVENT_MBFM_TYPE_INCEGDEC1;
  if( strcasecmp(mbfm_type, "IncEGLevel") == 0)        return MBNG_EVENT_MBFM_TYPE_INCEGLEVEL;
  if( strcasecmp(mbfm_type, "IncEGDec2") == 0)         return MBNG_EVENT_MBFM_TYPE_INCEGDEC2;
  if( strcasecmp(mbfm_type, "IncEGSus") == 0)          return MBNG_EVENT_MBFM_TYPE_INCEGSUS;
  if( strcasecmp(mbfm_type, "IncEGRel") == 0)          return MBNG_EVENT_MBFM_TYPE_INCEGREL;
  if( strcasecmp(mbfm_type, "IncLFOFreq") == 0)        return MBNG_EVENT_MBFM_TYPE_INCLFOFREQ;
  if( strcasecmp(mbfm_type, "IncLFODelay") == 0)       return MBNG_EVENT_MBFM_TYPE_INCLFODELAY;

  if( strcasecmp(mbfm_type, "DispVoice") == 0)      return MBNG_EVENT_MBFM_TYPE_DISPVOICE;
  if( strcasecmp(mbfm_type, "DispValue") == 0)      return MBNG_EVENT_MBFM_TYPE_DISPVALUE;
  if( strcasecmp(mbfm_type, "ClearVoice") == 0)     return MBNG_EVENT_MBFM_TYPE_CLEARVOICE;
  if( strcasecmp(mbfm_type, "ClearValue") == 0)     return MBNG_EVENT_MBFM_TYPE_CLEARVALUE;
  if( strcasecmp(mbfm_type, "VoiceLEDRed") == 0)    return MBNG_EVENT_MBFM_TYPE_VOICELEDRED;
  if( strcasecmp(mbfm_type, "VoiceLEDGreen") == 0)  return MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN;
  if( strcasecmp(mbfm_type, "AlgOpLEDRed") == 0)    return MBNG_EVENT_MBFM_TYPE_ALGOPLEDRED;
  if( strcasecmp(mbfm_type, "AlgOpLEDGreen") == 0)  return MBNG_EVENT_MBFM_TYPE_ALGOPLEDGREEN;
  if( strcasecmp(mbfm_type, "SeqTriggerLED") == 0)  return MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED;
  if( strcasecmp(mbfm_type, "IsMode") == 0)         return MBNG_EVENT_MBFM_TYPE_ISMODE;
  if( strcasecmp(mbfm_type, "IsDupl") == 0)         return MBNG_EVENT_MBFM_TYPE_ISDUPL;
  if( strcasecmp(mbfm_type, "IsLink") == 0)         return MBNG_EVENT_MBFM_TYPE_ISLINK;
  if( strcasecmp(mbfm_type, "IsRetrig") == 0)       return MBNG_EVENT_MBFM_TYPE_ISRETRIG;
  if( strcasecmp(mbfm_type, "IsDlyScale") == 0)     return MBNG_EVENT_MBFM_TYPE_ISDLYSCALE;
  if( strcasecmp(mbfm_type, "IsCurVoice4Op") == 0)  return MBNG_EVENT_MBFM_TYPE_ISCURVOICE4OP;
  if( strcasecmp(mbfm_type, "IsSomeVoice4Op") == 0) return MBNG_EVENT_MBFM_TYPE_ISSOMEVOICE4OP;
  if( strcasecmp(mbfm_type, "IsRecording") == 0)    return MBNG_EVENT_MBFM_TYPE_ISRECORDING;
  if( strcasecmp(mbfm_type, "IsPlaying") == 0)      return MBNG_EVENT_MBFM_TYPE_ISPLAYING;
  if( strcasecmp(mbfm_type, "IsMute") == 0)         return MBNG_EVENT_MBFM_TYPE_ISMUTE;
  if( strcasecmp(mbfm_type, "IsSolo") == 0)         return MBNG_EVENT_MBFM_TYPE_ISSOLO;
  if( strcasecmp(mbfm_type, "IsMeasure") == 0)      return MBNG_EVENT_MBFM_TYPE_ISMEASURE;
  if( strcasecmp(mbfm_type, "IsSeqVel") == 0)       return MBNG_EVENT_MBFM_TYPE_ISSEQVEL;
  if( strcasecmp(mbfm_type, "IsSeqVari") == 0)      return MBNG_EVENT_MBFM_TYPE_ISSEQVARI;
  if( strcasecmp(mbfm_type, "IsSeqGate") == 0)      return MBNG_EVENT_MBFM_TYPE_ISSEQGATE;
  if( strcasecmp(mbfm_type, "IsAlgFB") == 0)        return MBNG_EVENT_MBFM_TYPE_ISALGFB;
  if( strcasecmp(mbfm_type, "IsAlgFM") == 0)        return MBNG_EVENT_MBFM_TYPE_ISALGFM;
  if( strcasecmp(mbfm_type, "IsAlgOut") == 0)       return MBNG_EVENT_MBFM_TYPE_ISALGOUT;

  if( strcasecmp(mbfm_type, "ControlMod") == 0)     return MBNG_EVENT_MBFM_TYPE_CONTROLMOD;
  if( strcasecmp(mbfm_type, "ControlVari") == 0)    return MBNG_EVENT_MBFM_TYPE_CONTROLVARI;
  if( strcasecmp(mbfm_type, "ControlSus") == 0)     return MBNG_EVENT_MBFM_TYPE_CONTROLSUS;
  
  return MBNG_EVENT_MBFM_TYPE_UNDEFINED;
}


u8 MBNG_EVENT_ItemMBFMNumBytesGet(mbng_event_mbfm_type_t mbfm_type)
{
  switch( mbfm_type ) {
  case MBNG_EVENT_MBFM_TYPE_SELECTOPL3:          return 0;
  case MBNG_EVENT_MBFM_TYPE_SELECTVOICE:         return 1;
  case MBNG_EVENT_MBFM_TYPE_SELECTMODE:          return 1;
  case MBNG_EVENT_MBFM_TYPE_BTNSOFTKEY:          return 1;
  case MBNG_EVENT_MBFM_TYPE_BTNSELECT:           return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNMENU:             return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNDUPL:             return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNLINK:             return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNRETRIG:           return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNDLYSCALE:         return 0;
  case MBNG_EVENT_MBFM_TYPE_BTN4OP:              return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNALG:              return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNDEST:             return 1;
  case MBNG_EVENT_MBFM_TYPE_BTNOPPARAM:          return 1;
  case MBNG_EVENT_MBFM_TYPE_BTNOPWAVE:           return 1;
  case MBNG_EVENT_MBFM_TYPE_BTNOPMUTE:           return 1;
  case MBNG_EVENT_MBFM_TYPE_BTNEGASSIGN:         return 1;
  case MBNG_EVENT_MBFM_TYPE_BTNLFOASSIGN:        return 1;
  case MBNG_EVENT_MBFM_TYPE_BTNLFOWAVE:          return 1;
  case MBNG_EVENT_MBFM_TYPE_BTNWTASSIGN:         return 1;
  case MBNG_EVENT_MBFM_TYPE_BTNVELASSIGN:        return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNMODASSIGN:        return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNVARIASSIGN:       return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNSEQREC:           return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNSEQPLAY:          return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNSEQBACK:          return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNSEQMUTE:          return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNSEQSOLO:          return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNSEQVEL:           return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNSEQVARI:          return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNSEQGATE:          return 0;
  case MBNG_EVENT_MBFM_TYPE_BTNSEQTRIGGER:       return 1;
  case MBNG_EVENT_MBFM_TYPE_BTNSEQMEASURE:       return 1;
  case MBNG_EVENT_MBFM_TYPE_BTNSEQSECTION:       return 0;

  case MBNG_EVENT_MBFM_TYPE_SETDATAWHEEL:        return 0;
  case MBNG_EVENT_MBFM_TYPE_SETVOICETRANSPOSE:   return 0;
  case MBNG_EVENT_MBFM_TYPE_SETVOICETUNE:        return 0;
  case MBNG_EVENT_MBFM_TYPE_SETVOICEPORTA:       return 0;
  case MBNG_EVENT_MBFM_TYPE_SETVOICEDLYTIME:     return 0;
  case MBNG_EVENT_MBFM_TYPE_SETVOICEFB:          return 0;
  case MBNG_EVENT_MBFM_TYPE_SETOPFMULT:          return 1;
  case MBNG_EVENT_MBFM_TYPE_SETOPATK:            return 1;
  case MBNG_EVENT_MBFM_TYPE_SETOPDEC:            return 1;
  case MBNG_EVENT_MBFM_TYPE_SETOPSUS:            return 1;
  case MBNG_EVENT_MBFM_TYPE_SETOPREL:            return 1;
  case MBNG_EVENT_MBFM_TYPE_SETOPVOLUME:         return 1;
  case MBNG_EVENT_MBFM_TYPE_SETEGATK:            return 1;
  case MBNG_EVENT_MBFM_TYPE_SETEGDEC1:           return 1;
  case MBNG_EVENT_MBFM_TYPE_SETEGLEVEL:          return 1;
  case MBNG_EVENT_MBFM_TYPE_SETEGDEC2:           return 1;
  case MBNG_EVENT_MBFM_TYPE_SETEGSUS:            return 1;
  case MBNG_EVENT_MBFM_TYPE_SETEGREL:            return 1;
  case MBNG_EVENT_MBFM_TYPE_SETLFOFREQ:          return 1;
  case MBNG_EVENT_MBFM_TYPE_SETLFODELAY:         return 1;
  
  case MBNG_EVENT_MBFM_TYPE_INCDATAWHEEL:        return 0;
  case MBNG_EVENT_MBFM_TYPE_INCVOICETRANSPOSE:   return 0;
  case MBNG_EVENT_MBFM_TYPE_INCVOICETUNE:        return 0;
  case MBNG_EVENT_MBFM_TYPE_INCVOICEPORTA:       return 0;
  case MBNG_EVENT_MBFM_TYPE_INCVOICEDLYTIME:     return 0;
  case MBNG_EVENT_MBFM_TYPE_INCVOICEFB:          return 0;
  case MBNG_EVENT_MBFM_TYPE_INCOPFMULT:          return 1;
  case MBNG_EVENT_MBFM_TYPE_INCOPATK:            return 1;
  case MBNG_EVENT_MBFM_TYPE_INCOPDEC:            return 1;
  case MBNG_EVENT_MBFM_TYPE_INCOPSUS:            return 1;
  case MBNG_EVENT_MBFM_TYPE_INCOPREL:            return 1;
  case MBNG_EVENT_MBFM_TYPE_INCOPVOLUME:         return 1;
  case MBNG_EVENT_MBFM_TYPE_INCEGATK:            return 1;
  case MBNG_EVENT_MBFM_TYPE_INCEGDEC1:           return 1;
  case MBNG_EVENT_MBFM_TYPE_INCEGLEVEL:          return 1;
  case MBNG_EVENT_MBFM_TYPE_INCEGDEC2:           return 1;
  case MBNG_EVENT_MBFM_TYPE_INCEGSUS:            return 1;
  case MBNG_EVENT_MBFM_TYPE_INCEGREL:            return 1;
  case MBNG_EVENT_MBFM_TYPE_INCLFOFREQ:          return 1;
  case MBNG_EVENT_MBFM_TYPE_INCLFODELAY:         return 1;

  case MBNG_EVENT_MBFM_TYPE_DISPVOICE:           return 0;
  case MBNG_EVENT_MBFM_TYPE_DISPVALUE:           return 0;
  case MBNG_EVENT_MBFM_TYPE_CLEARVOICE:          return 0;
  case MBNG_EVENT_MBFM_TYPE_CLEARVALUE:          return 0;
  case MBNG_EVENT_MBFM_TYPE_VOICELEDRED:         return 1;
  case MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN:       return 1;
  case MBNG_EVENT_MBFM_TYPE_ALGOPLEDRED:         return 1;
  case MBNG_EVENT_MBFM_TYPE_ALGOPLEDGREEN:       return 1;
  case MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED:       return 1;
  case MBNG_EVENT_MBFM_TYPE_ISMODE:              return 1;
  case MBNG_EVENT_MBFM_TYPE_ISDUPL:              return 0;
  case MBNG_EVENT_MBFM_TYPE_ISLINK:              return 0;
  case MBNG_EVENT_MBFM_TYPE_ISRETRIG:            return 0;
  case MBNG_EVENT_MBFM_TYPE_ISDLYSCALE:          return 0;
  case MBNG_EVENT_MBFM_TYPE_ISCURVOICE4OP:       return 0;
  case MBNG_EVENT_MBFM_TYPE_ISSOMEVOICE4OP:      return 1;
  case MBNG_EVENT_MBFM_TYPE_ISRECORDING:         return 0;
  case MBNG_EVENT_MBFM_TYPE_ISPLAYING:           return 0;
  case MBNG_EVENT_MBFM_TYPE_ISMUTE:              return 0;
  case MBNG_EVENT_MBFM_TYPE_ISSOLO:              return 0;
  case MBNG_EVENT_MBFM_TYPE_ISMEASURE:           return 1;
  case MBNG_EVENT_MBFM_TYPE_ISSEQVEL:            return 0;
  case MBNG_EVENT_MBFM_TYPE_ISSEQVARI:           return 0;
  case MBNG_EVENT_MBFM_TYPE_ISSEQGATE:           return 0;
  case MBNG_EVENT_MBFM_TYPE_ISALGFB:             return 0;
  case MBNG_EVENT_MBFM_TYPE_ISALGFM:             return 1;
  case MBNG_EVENT_MBFM_TYPE_ISALGOUT:            return 1;

  case MBNG_EVENT_MBFM_TYPE_CONTROLMOD:          return 0;
  case MBNG_EVENT_MBFM_TYPE_CONTROLVARI:         return 0;
  case MBNG_EVENT_MBFM_TYPE_CONTROLSUS:          return 0;
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a NRPN event. Skips address bytes if they already have been sent!
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_SendOptimizedNRPN(mios32_midi_port_t port, mios32_midi_chn_t chn, u16 nrpn_address, u16 nrpn_value, u8 msb_only)
{
  u8 nrpn_address_msb = (nrpn_address >> 7) & 0x7f;
  u8 nrpn_address_lsb = (nrpn_address >> 0) & 0x7f;
  u8 nrpn_value_msb = (nrpn_value >> 7) & 0x7f;
  u8 nrpn_value_lsb = (nrpn_value >> 0) & 0x7f;

  // create MIDI package
  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = CC;
  p.event = CC;
  p.chn = chn;

  // quick&dirty
#if MBNG_EVENT_NRPN_SEND_PORTS_MASK != 0x00f0
# error "Please adapt MBNG_EVENT_SendOptimizedNRPN!"
#endif
#if MBNG_EVENT_NRPN_SEND_PORTS_OFFSET != 4
# error "Please adapt MBNG_EVENT_SendOptimizedNRPN!"
#endif

  if( port >= UART0 && port <= UART3 ) {
    int port_ix = port - UART0;

    if( (nrpn_address ^ nrpn_sent_address[port_ix][p.chn]) >> 7 ) { // new MSB - will also cover the case that nrpn_sent_address == 0xffff
      p.cc_number = 0x63; // Address MSB
      p.value = nrpn_address_msb;
      MIOS32_MIDI_SendPackage(port, p);
    }

    if( (nrpn_address ^ nrpn_sent_address[port_ix][p.chn]) & 0x7f ) { // new LSB
      p.cc_number = 0x62; // Address LSB
      p.value = nrpn_address_lsb;
      MIOS32_MIDI_SendPackage(port, p);
    }

    if( (nrpn_value ^ nrpn_sent_value[port_ix][p.chn]) >> 7 ) { // new MSB - will also cover the case that nrpn_sent_value == 0xffff
      p.cc_number = 0x06; // Data MSB
      p.value = nrpn_value_msb;
      MIOS32_MIDI_SendPackage(port, p);
    }

    if( !msb_only ) {
      if( (nrpn_value ^ nrpn_sent_value[port_ix][p.chn]) & 0x7f ) { // new LSB
	p.cc_number = 0x26; // Data LSB
	p.value = nrpn_value_lsb;
	MIOS32_MIDI_SendPackage(port, p);
      }
    }

    nrpn_sent_address[port_ix][p.chn] = nrpn_address;
    nrpn_sent_value[port_ix][p.chn] = nrpn_value;

  } else {
    p.cc_number = 0x63; // Address MSB
    p.value = nrpn_address_msb;
    MIOS32_MIDI_SendPackage(port, p);

    p.cc_number = 0x62; // Address LSB
    p.value = nrpn_address_lsb;
    MIOS32_MIDI_SendPackage(port, p);

    p.cc_number = 0x06; // Data MSB
    p.value = nrpn_value_msb;
    MIOS32_MIDI_SendPackage(port, p);

    if( !msb_only ) {
      p.cc_number = 0x26; // Data LSB
      p.value = nrpn_value_lsb;
      MIOS32_MIDI_SendPackage(port, p);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a SysEx stream with derived SysEx variables
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_SendSysExStream(mios32_midi_port_t port, mbng_event_item_t *item)
{
  u8 *stream_in = item->stream;
  u32 stream_size = item->stream_size;
  s16 item_value = item->value;

#define STREAM_MAX_SIZE 128
  u8 stream[STREAM_MAX_SIZE];
  u8 *stream_out = stream;
  u8 *stream_in_end = (u8 *)(stream_in + stream_size - 1);
  u32 len = 0;
  u8 chk = 0;

  // to simplify stream usage
#define MBNG_EVENT_ADD_STREAM(b) { if( len < STREAM_MAX_SIZE ) { *stream_out = b; ++len; chk += *stream_out++; } }

  while( stream_in <= stream_in_end ) {
    if( *stream_in == 0xff ) {
      ++stream_in;
      switch( *stream_in++ ) {
      case MBNG_EVENT_SYSEX_VAR_DEV:        MBNG_EVENT_ADD_STREAM(mbng_patch_cfg.sysex_dev); break;
      case MBNG_EVENT_SYSEX_VAR_PAT:        MBNG_EVENT_ADD_STREAM(mbng_patch_cfg.sysex_pat); break;
      case MBNG_EVENT_SYSEX_VAR_BNK:        MBNG_EVENT_ADD_STREAM(mbng_patch_cfg.sysex_bnk); break;
      case MBNG_EVENT_SYSEX_VAR_INS:        MBNG_EVENT_ADD_STREAM(mbng_patch_cfg.sysex_ins); break;
      case MBNG_EVENT_SYSEX_VAR_CHN:        MBNG_EVENT_ADD_STREAM(mbng_patch_cfg.sysex_chn); break;
      case MBNG_EVENT_SYSEX_VAR_CHK_START:  chk = 0; break;
      case MBNG_EVENT_SYSEX_VAR_CHK:        MBNG_EVENT_ADD_STREAM(chk & 0x7f); break;
      case MBNG_EVENT_SYSEX_VAR_CHK_INV:    MBNG_EVENT_ADD_STREAM((chk ^ 0x7f) & 0x7f); break;
      case MBNG_EVENT_SYSEX_VAR_VAL:        MBNG_EVENT_ADD_STREAM(item_value & 0x7f); break;
      case MBNG_EVENT_SYSEX_VAR_VAL_H:      MBNG_EVENT_ADD_STREAM((item_value >>  7) & 0x7f); break;
      case MBNG_EVENT_SYSEX_VAR_VAL_N1:     MBNG_EVENT_ADD_STREAM((item_value >>  0) & 0xf); break;
      case MBNG_EVENT_SYSEX_VAR_VAL_N2:     MBNG_EVENT_ADD_STREAM((item_value >>  4) & 0xf); break;
      case MBNG_EVENT_SYSEX_VAR_VAL_N3:     MBNG_EVENT_ADD_STREAM((item_value >>  8) & 0xf); break;
      case MBNG_EVENT_SYSEX_VAR_VAL_N4:     MBNG_EVENT_ADD_STREAM((item_value >> 12) & 0xf); break;
      case MBNG_EVENT_SYSEX_VAR_IGNORE:     break;
      case MBNG_EVENT_SYSEX_VAR_DUMP:       break; // not relevant for transmitter (yet) - or should we allow to send a dump stored dump?
      case MBNG_EVENT_SYSEX_VAR_CURSOR:     break; // not relevant for transmitter (yet) - or should we allow to send a dump stored dump?
      case MBNG_EVENT_SYSEX_VAR_TXT:        break; // not relevant for transmitter (yet) - or should we allow to send a dump stored dump?
      case MBNG_EVENT_SYSEX_VAR_TXT56:      break; // not relevant for transmitter (yet) - or should we allow to send a dump stored dump?
      case MBNG_EVENT_SYSEX_VAR_LABEL: {
	u32 max_len = STREAM_MAX_SIZE - len;
	if( max_len ) {
	  s32 status;
	  if( (status=MBNG_LCD_PrintItemLabel(item, (char *)stream_out, max_len)) > 0 ) {
	    len += status;
	    stream_out += status;
	  }
	}
      } break;
      default: {}
      }
    } else {
      MBNG_EVENT_ADD_STREAM(*stream_in++);
    }
  }

  return MIOS32_MIDI_SendSysEx(port, stream, len);
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a Meta Event
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ExecMeta(mbng_event_item_t *item)
{
  int i;
  for(i=0; i<item->stream_size; i++) {
    mbng_event_meta_type_t meta_type = item->stream[i+0];
    u8 num_bytes = MBNG_EVENT_ItemMetaNumBytesGet(meta_type);
    u8 meta_value = item->stream[i+1]; // optional
    i += num_bytes;

    switch( meta_type ) {
    case MBNG_EVENT_META_TYPE_SET_BANK: {
      if( item->value ) {
	MBNG_EVENT_SelectedBankSet(item->value);
      }
      item->value = MBNG_EVENT_SelectedBankGet(); // take over the new bank value (allows to forward it to other components)
    } break;

    case MBNG_EVENT_META_TYPE_DEC_BANK: {
      int bank = MBNG_EVENT_SelectedBankGet();
      if( bank > 1 )
	MBNG_EVENT_SelectedBankSet(bank - 1);
      item->value = MBNG_EVENT_SelectedBankGet(); // take over the new bank value (allows to forward it to other components)
    } break;

    case MBNG_EVENT_META_TYPE_INC_BANK: {
      int bank = MBNG_EVENT_SelectedBankGet();
      if( bank < MBNG_EVENT_NumBanksGet() )
	MBNG_EVENT_SelectedBankSet(bank + 1);
      item->value = MBNG_EVENT_SelectedBankGet(); // take over the new bank value (allows to forward it to other components)
    } break;

    case MBNG_EVENT_META_TYPE_CYCLE_BANK: {
      int bank = MBNG_EVENT_SelectedBankGet();
      if( bank < MBNG_EVENT_NumBanksGet() )
	MBNG_EVENT_SelectedBankSet(bank + 1);
      else
	MBNG_EVENT_SelectedBankSet(1);
      item->value = MBNG_EVENT_SelectedBankGet(); // take over the new bank value (allows to forward it to other components)
    } break;


    case MBNG_EVENT_META_TYPE_SET_BANK_OF_HW_ID: {
      if( item->value ) {
	MBNG_EVENT_HwIdBankSet(meta_value, item->value);
      }
      item->value = MBNG_EVENT_HwIdBankGet(meta_value); // take over the new bank value (allows to forward it to other components)
    } break;

    case MBNG_EVENT_META_TYPE_DEC_BANK_OF_HW_ID: {
      int bank = MBNG_EVENT_HwIdBankGet(meta_value);
      if( bank > 1 )
	MBNG_EVENT_HwIdBankSet(meta_value, bank - 1);
      item->value = MBNG_EVENT_HwIdBankGet(meta_value); // take over the new bank value (allows to forward it to other components)
    } break;

    case MBNG_EVENT_META_TYPE_INC_BANK_OF_HW_ID: {
      int bank = MBNG_EVENT_HwIdBankGet(meta_value);
      if( bank < MBNG_EVENT_NumBanksGet() )
	MBNG_EVENT_HwIdBankSet(meta_value, bank + 1);
      item->value = MBNG_EVENT_HwIdBankGet(meta_value); // take over the new bank value (allows to forward it to other components)
    } break;

    case MBNG_EVENT_META_TYPE_CYCLE_BANK_OF_HW_ID: {
      int bank = MBNG_EVENT_HwIdBankGet(meta_value);
      if( bank < MBNG_EVENT_NumBanksGet() )
	MBNG_EVENT_HwIdBankSet(meta_value, bank + 1);
      else
	MBNG_EVENT_HwIdBankSet(meta_value, 1);
      item->value = MBNG_EVENT_HwIdBankGet(meta_value); // take over the new bank value (allows to forward it to other components)
    } break;


    case MBNG_EVENT_META_TYPE_SET_SNAPSHOT: {
      if( item->value < MBNG_FILE_S_NUM_SNAPSHOTS ) {
	MBNG_FILE_S_SnapshotSet(item->value);
      }
      item->value = MBNG_FILE_S_SnapshotGet(); // take over the new snapshot value (allows to forward it to other components)
    } break;

    case MBNG_EVENT_META_TYPE_DEC_SNAPSHOT: {
      int snapshot = MBNG_FILE_S_SnapshotGet();
      if( snapshot > 1 )
	MBNG_FILE_S_SnapshotSet(snapshot - 1);
      item->value = MBNG_FILE_S_SnapshotGet(); // take over the new snapshot value (allows to forward it to other components)
    } break;

    case MBNG_EVENT_META_TYPE_INC_SNAPSHOT: {
      int snapshot = MBNG_FILE_S_SnapshotGet();
      if( snapshot < (MBNG_FILE_S_NUM_SNAPSHOTS-1) )
	MBNG_FILE_S_SnapshotSet(snapshot + 1);
      item->value = MBNG_FILE_S_SnapshotGet(); // take over the new snapshot value (allows to forward it to other components)
    } break;

    case MBNG_EVENT_META_TYPE_CYCLE_SNAPSHOT: {
      int snapshot = MBNG_FILE_S_SnapshotGet();
      if( snapshot < (MBNG_FILE_S_NUM_SNAPSHOTS-1) )
	MBNG_FILE_S_SnapshotSet(snapshot + 1);
      else
	MBNG_FILE_S_SnapshotSet(0);
      item->value = MBNG_FILE_S_SnapshotGet(); // take over the new snapshot value (allows to forward it to other components)
    } break;

    case MBNG_EVENT_META_TYPE_LOAD_SNAPSHOT: {
      MBNG_FILE_S_Read(mbng_file_s_patch_name, MBNG_FILE_S_SnapshotGet());
    } break;

    case MBNG_EVENT_META_TYPE_SAVE_SNAPSHOT: {
      MBNG_FILE_S_Write(mbng_file_s_patch_name, MBNG_FILE_S_SnapshotGet());
    } break;

    case MBNG_EVENT_META_TYPE_DUMP_SNAPSHOT: {
      MBNG_EVENT_Dump();
    } break;

    case MBNG_EVENT_META_TYPE_RETRIEVE_AIN_VALUES: {
      int pin;
      for(pin=0; pin<MBNG_PATCH_NUM_AIN; ++pin) {
	MBNG_AIN_NotifyChange(pin, MIOS32_AIN_PinGet(pin), 1); // no_midi==1
      }
    } break;

    case MBNG_EVENT_META_TYPE_RETRIEVE_AINSER_VALUES: {
      int module;
      for(module=0; module<MBNG_PATCH_NUM_AINSER_MODULES; ++module) {
	int pin;
	for(pin=0; pin<AINSER_NUM_PINS; ++pin) {
	  MBNG_AINSER_NotifyChange(module, pin, AINSER_PinGet(module, pin), 1); // no_midi==1
	}
      }
    } break;

    case MBNG_EVENT_META_TYPE_ENC_FAST: {
      MBNG_ENC_FastModeSet(item->value);
    } break;

    case MBNG_EVENT_META_TYPE_MIDI_LEARN: {
      MBNG_EVENT_MidiLearnModeSet(item->value);
    } break;

    case MBNG_EVENT_META_TYPE_UPDATE_LCD: {
      MBNG_EVENT_Refresh();
    } break;

    case MBNG_EVENT_META_TYPE_RESET_METERS: {
      MBNG_EVENT_LCMeters_Init();
    } break;

    case MBNG_EVENT_META_TYPE_SWAP_VALUES: {
      u8 secondary = item->secondary_value;
      item->secondary_value = item->value;
      item->value = secondary;
    } break;

    case MBNG_EVENT_META_TYPE_RUN_SECTION: {
      MBNG_FILE_R_ReadRequest(NULL, meta_value, item->value, 0);
    } break;

    case MBNG_EVENT_META_TYPE_RUN_STOP: {
      MBNG_FILE_R_RunStop();
    } break;


    case MBNG_EVENT_META_TYPE_MCLK_PLAY: {
      MBNG_SEQ_PlayButton();
    } break;

    case MBNG_EVENT_META_TYPE_MCLK_STOP: {
      MBNG_SEQ_StopButton();
    } break;

    case MBNG_EVENT_META_TYPE_MCLK_PLAYSTOP: {
      MBNG_SEQ_PlayStopButton();
    } break;

    case MBNG_EVENT_META_TYPE_MCLK_PAUSE: {
      MBNG_SEQ_PauseButton();
    } break;

    case MBNG_EVENT_META_TYPE_MCLK_SET_TEMPO: {
      SEQ_BPM_Set((float)item->value);
    } break;

    case MBNG_EVENT_META_TYPE_MCLK_DEC_TEMPO: {
      u16 tempo = (u16)SEQ_BPM_Get();
      tempo = (tempo > item->min) ? (tempo - 1) : item->min;
      SEQ_BPM_Set((float)tempo);
    } break;

    case MBNG_EVENT_META_TYPE_MCLK_INC_TEMPO: {
      u16 tempo = (u16)SEQ_BPM_Get();
      tempo = (tempo < item->max) ? (tempo + 1) : item->max;
      SEQ_BPM_Set((float)tempo);
    } break;



    case MBNG_EVENT_META_TYPE_CV_PITCHBEND_14BIT: {
      MBNG_CV_PitchSet(meta_value - 1, (int)item->value - 8192);
    } break;

    case MBNG_EVENT_META_TYPE_CV_PITCHBEND_7BIT: {
      MBNG_CV_PitchSet(meta_value - 1, 128 * ((int)item->value - 64));
    } break;

    case MBNG_EVENT_META_TYPE_CV_PITCHRANGE: {
      MBNG_CV_PitchRangeSet(meta_value - 1, item->value);
    } break;

    case MBNG_EVENT_META_TYPE_CV_TRANSPOSE_OCTAVE: {
      MBNG_CV_TransposeOctaveSet(meta_value - 1, (int)item->value - 64);
    } break;

    case MBNG_EVENT_META_TYPE_CV_TRANSPOSE_SEMITONES: {
      MBNG_CV_TransposeSemitonesSet(meta_value - 1, (int)item->value - 64);
    } break;

    /*
    case MBNG_EVENT_META_TYPE_SCS_ENC: {
      SCS_ENC_MENU_NotifyChange((s32)(item->value - 64));
    } break;
    case MBNG_EVENT_META_TYPE_SCS_MENU: {
      SCS_DIN_NotifyToggle(SCS_PIN_EXIT, item->value == 0);
    } break;
    case MBNG_EVENT_META_TYPE_SCS_SOFT1: {
      SCS_DIN_NotifyToggle(SCS_PIN_SOFT1, item->value == 0);
    } break;
    case MBNG_EVENT_META_TYPE_SCS_SOFT2: {
      SCS_DIN_NotifyToggle(SCS_PIN_SOFT2, item->value == 0);
    } break;
    case MBNG_EVENT_META_TYPE_SCS_SOFT3: {
      SCS_DIN_NotifyToggle(SCS_PIN_SOFT3, item->value == 0);
    } break;
    case MBNG_EVENT_META_TYPE_SCS_SOFT4: {
      SCS_DIN_NotifyToggle(SCS_PIN_SOFT4, item->value == 0);
    } break;
    case MBNG_EVENT_META_TYPE_SCS_SOFT5: {
      SCS_DIN_NotifyToggle(SCS_PIN_SOFT5, item->value == 0);
    } break;
    case MBNG_EVENT_META_TYPE_SCS_SOFT6: {
      SCS_DIN_NotifyToggle(SCS_PIN_SOFT6, item->value == 0);
    } break;
    case MBNG_EVENT_META_TYPE_SCS_SOFT7: {
      SCS_DIN_NotifyToggle(SCS_PIN_SOFT7, item->value == 0);
    } break;
    case MBNG_EVENT_META_TYPE_SCS_SOFT8: {
      SCS_DIN_NotifyToggle(SCS_PIN_SOFT8, item->value == 0);
    } break;
    case MBNG_EVENT_META_TYPE_SCS_SHIFT: {
      SCS_DIN_NotifyToggle(SCS_PIN_SOFT1 + SCS_NumMenuItemsGet(), item->value == 0);
    } break;
    */
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Processes an outgoing MBFM event
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ExecMBFM(mbng_event_item_t *item)
{
  int i;
  for(i=0; i<item->stream_size; i++) {
    mbng_event_mbfm_type_t mbfm_type = item->stream[i+0];
    u8 num_bytes = MBNG_EVENT_ItemMBFMNumBytesGet(mbfm_type);
    u8 mbfm_value = item->stream[i+1]; // optional
    i += num_bytes;

    switch( mbfm_type ) {
    case MBNG_EVENT_MBFM_TYPE_SELECTOPL3: {
      MBFM_SelectOPL3(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SELECTVOICE: {
      MBFM_SelectVoice(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SELECTMODE: {
      MBFM_SelectMode(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSOFTKEY: {
      MBFM_BtnSoftkey(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSELECT: {
      MBFM_BtnSelect(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNMENU: {
      MBFM_BtnMenu(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNDUPL: {
      MBFM_BtnDupl(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNLINK: {
      MBFM_BtnLink(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNRETRIG: {
      MBFM_BtnRetrig(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNDLYSCALE: {
      MBFM_BtnDlyScale(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTN4OP: {
      MBFM_Btn4Op(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNALG: {
      MBFM_BtnAlg(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNDEST: {
      MBFM_BtnDest(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNOPPARAM: {
      MBFM_BtnOpParam(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNOPWAVE: {
      MBFM_BtnOpWave(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNOPMUTE: {
      MBFM_BtnOpMute(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNEGASSIGN: {
      MBFM_BtnEGAssign(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNLFOASSIGN: {
      MBFM_BtnLFOAssign(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNLFOWAVE: {
      MBFM_BtnLFOWave(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNWTASSIGN: {
      MBFM_BtnWTAssign(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNVELASSIGN: {
      MBFM_BtnVelAssign(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNMODASSIGN: {
      MBFM_BtnModAssign(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNVARIASSIGN: {
      MBFM_BtnVariAssign(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSEQREC: {
      MBFM_BtnSeqRec(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSEQPLAY: {
      MBFM_BtnSeqPlay(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSEQBACK: {
      MBFM_BtnSeqBack(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSEQMUTE: {
      MBFM_BtnSeqMute(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSEQSOLO: {
      MBFM_BtnSeqSolo(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSEQVEL: {
      MBFM_BtnSeqVel(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSEQVARI: {
      MBFM_BtnSeqVari(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSEQGATE: {
      MBFM_BtnSeqGate(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSEQTRIGGER: {
      MBFM_BtnSeqTrigger(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSEQMEASURE: {
      MBFM_BtnSeqMeasure(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_BTNSEQSECTION: {
      MBFM_BtnSeqSection(item->value);
    } break;
    
    ///////////////////////////////////////////////////////////////////////////
    
    case MBNG_EVENT_MBFM_TYPE_SETDATAWHEEL: {
      MBFM_SetDatawheel(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETVOICETRANSPOSE: {
      MBFM_SetVoiceTranspose(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETVOICETUNE: {
      MBFM_SetVoiceTune(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETVOICEPORTA: {
      MBFM_SetVoicePorta(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETVOICEDLYTIME: {
      MBFM_SetVoiceDlyTime(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETVOICEFB: {
      MBFM_SetVoiceFB(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETOPFMULT: {
      MBFM_SetOpFMult(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETOPATK: {
      MBFM_SetOpAtk(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETOPDEC: {
      MBFM_SetOpDec(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETOPSUS: {
      MBFM_SetOpSus(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETOPREL: {
      MBFM_SetOpRel(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETOPVOLUME: {
      MBFM_SetOpVolume(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETEGATK: {
      MBFM_SetEGAtk(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETEGDEC1: {
      MBFM_SetEGDec1(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETEGLEVEL: {
      MBFM_SetEGLevel(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETEGDEC2: {
      MBFM_SetEGDec2(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETEGSUS: {
      MBFM_SetEGSus(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETEGREL: {
      MBFM_SetEGRel(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETLFOFREQ: {
      MBFM_SetLFOFreq(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_SETLFODELAY: {
      MBFM_SetLFODelay(mbfm_value, item->value);
    } break;
    
    ///////////////////////////////////////////////////////////////////////////
    
    case MBNG_EVENT_MBFM_TYPE_INCDATAWHEEL: {
      MBFM_IncDatawheel(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCVOICETRANSPOSE: {
      MBFM_IncVoiceTranspose(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCVOICETUNE: {
      MBFM_IncVoiceTune(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCVOICEPORTA: {
      MBFM_IncVoicePorta(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCVOICEDLYTIME: {
      MBFM_IncVoiceDlyTime(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCVOICEFB: {
      MBFM_IncVoiceFB(item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCOPFMULT: {
      MBFM_IncOpFMult(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCOPATK: {
      MBFM_IncOpAtk(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCOPDEC: {
      MBFM_IncOpDec(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCOPSUS: {
      MBFM_IncOpSus(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCOPREL: {
      MBFM_IncOpRel(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCOPVOLUME: {
      MBFM_IncOpVolume(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCEGATK: {
      MBFM_IncEGAtk(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCEGDEC1: {
      MBFM_IncEGDec1(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCEGLEVEL: {
      MBFM_IncEGLevel(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCEGDEC2: {
      MBFM_IncEGDec2(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCEGSUS: {
      MBFM_IncEGSus(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCEGREL: {
      MBFM_IncEGRel(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCLFOFREQ: {
      MBFM_IncLFOFreq(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_INCLFODELAY: {
      MBFM_IncLFODelay(mbfm_value, item->value);
    } break;
    
    
    /* TODO what to do about sending-type events?
    case MBNG_EVENT_MBFM_TYPE_DISPVOICE:           return 0;
    case MBNG_EVENT_MBFM_TYPE_DISPVALUE:           return 0;
    case MBNG_EVENT_MBFM_TYPE_CLEARVOICE:          return 0;
    case MBNG_EVENT_MBFM_TYPE_CLEARVALUE:          return 0;
    case MBNG_EVENT_MBFM_TYPE_VOICELEDRED:         return 1;
    case MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN:       return 1;
    case MBNG_EVENT_MBFM_TYPE_ALGOPLEDRED:         return 1;
    case MBNG_EVENT_MBFM_TYPE_ALGOPLEDGREEN:       return 1;
    case MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED:       return 1;
    case MBNG_EVENT_MBFM_TYPE_ISMODEFM:            return 0;
    case MBNG_EVENT_MBFM_TYPE_ISMODEMIDI:          return 0;
    case MBNG_EVENT_MBFM_TYPE_ISMODESEQ:           return 0;
    case MBNG_EVENT_MBFM_TYPE_ISMODEWTED:          return 0;
    case MBNG_EVENT_MBFM_TYPE_ISRETRIG:            return 0;
    case MBNG_EVENT_MBFM_TYPE_ISDLYSCALE:          return 0;
    case MBNG_EVENT_MBFM_TYPE_ISCURVOICE4OP:       return 0;
    case MBNG_EVENT_MBFM_TYPE_ISSOMEVOICE4OP:      return 1;
    case MBNG_EVENT_MBFM_TYPE_ISRECORDING:         return 0;
    case MBNG_EVENT_MBFM_TYPE_ISPLAYING:           return 0;
    case MBNG_EVENT_MBFM_TYPE_ISMUTE:              return 0;
    case MBNG_EVENT_MBFM_TYPE_ISSOLO:              return 0;
    case MBNG_EVENT_MBFM_TYPE_ISMEASURE:           return 1;
    case MBNG_EVENT_MBFM_TYPE_ISSEQVEL:            return 0;
    case MBNG_EVENT_MBFM_TYPE_ISSEQVARI:           return 0;
    case MBNG_EVENT_MBFM_TYPE_ISSEQGATE:           return 0;
    case MBNG_EVENT_MBFM_TYPE_ISALGFB:             return 0;
    case MBNG_EVENT_MBFM_TYPE_ISALGFM:             return 1;
    case MBNG_EVENT_MBFM_TYPE_ISALGOUT:            return 1;
    */

    case MBNG_EVENT_MBFM_TYPE_CONTROLMOD: {
      MBFM_ControlMod(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_CONTROLVARI: {
      MBFM_ControlVari(mbfm_value, item->value);
    } break;
    case MBNG_EVENT_MBFM_TYPE_CONTROLSUS: {
      MBFM_ControlSus(mbfm_value, item->value);
    } break;
    }
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! Sends the item via MIDI
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemSend(mbng_event_item_t *item)
{
  if( !item->stream_size )
    return 0; // nothing to send

  mbng_event_type_t event_type = item->flags.type;
  if( event_type <= MBNG_EVENT_TYPE_PITCHBEND ) {
    u8 event = (item->stream[0] >> 4) | 8;

    // create MIDI package
    mios32_midi_package_t p;
    p.ALL = 0;
    p.type = event;
    p.event = event;

    if( mbng_patch_cfg.global_chn )
      p.chn = mbng_patch_cfg.global_chn - 1;
    else
      p.chn = item->stream[0] & 0xf;

    switch( event_type ) {
    case MBNG_EVENT_TYPE_PROGRAM_CHANGE:
    case MBNG_EVENT_TYPE_AFTERTOUCH:
      p.evnt1 = item->value & 0x7f;
      p.evnt2 = 0;
      break;

    case MBNG_EVENT_TYPE_PITCHBEND:
      if( (item->max-item->min) < 128 ) { // 7bit range
	u8 value = item->value & 0x7f; // just to ensure
	p.evnt1 = (value == 0x40) ? 0x00 : value;
	p.evnt2 = value;
      } else {
	p.evnt1 = (item->value & 0x7f);
	p.evnt2 = (item->value >> 7) & 0x7f;
      }
      break;

    default:
      if( item->flags.use_key_or_cc ) { // key=any or cc=any
	p.evnt1 = item->value & 0x7f;
	p.evnt2 = item->secondary_value;
      } else {
	p.evnt1 = item->secondary_value & 0x7f; // TK: was 'item->stream[1] & 0x7f;', now using secondary value for more flexibility
	p.evnt2 = item->value & 0x7f;
      }
    }

    // EXTRA for BUTTON_MATRIX: modify MIDI event depending on pin number
    // NOTE: if dedicated MIDI events should be sent, use the button_emu_id_offset feature!
    if( item->matrix_pin && event_type < MBNG_EVENT_TYPE_CC ) {
      int new = p.evnt1 + item->matrix_pin;
      if( new >= 127 )
	new = 127;
      p.evnt1 = new;
    }

    // send MIDI package over enabled ports
    int i;
    u16 mask = 1;
    for(i=0; i<16; ++i, mask <<= 1) {
      if( item->enabled_ports & mask ) {
	// USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	mios32_midi_port_t port = USB0 + ((i&0xc) << 2) + (i&3);
	MUTEX_MIDIOUT_TAKE;
	MIOS32_MIDI_SendPackage(port, p);
	MUTEX_MIDIOUT_GIVE;
      }
    }
    return 0; // no error

  } else if( event_type == MBNG_EVENT_TYPE_NRPN ) {

    u16 nrpn_address = item->stream[1] | ((u16)item->stream[2] << 7);
    mbng_event_nrpn_format_t nrpn_format = item->stream[3];
    u8 msb_only = nrpn_format == MBNG_EVENT_NRPN_FORMAT_MSB_ONLY;

    u16 nrpn_value = item->value;
    if( nrpn_format == MBNG_EVENT_NRPN_FORMAT_SIGNED ) {
      nrpn_value = (nrpn_value - 8192) & 0x3fff;
    } else if( nrpn_format == MBNG_EVENT_NRPN_FORMAT_MSB_ONLY ) {
      nrpn_value = nrpn_value * 128;
      if( nrpn_value > 16383 )
	nrpn_value = 16383;
    }

    mios32_midi_chn_t chn = mbng_patch_cfg.global_chn ? (mbng_patch_cfg.global_chn - 1) : item->stream[0] & 0xf;

    // send optimized NRPNs over enabled ports
    int i;
    u16 mask = 1;
    for(i=0; i<16; ++i, mask <<= 1) {
      if( item->enabled_ports & mask ) {
	// USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	mios32_midi_port_t port = USB0 + ((i&0xc) << 2) + (i&3);

	MUTEX_MIDIOUT_TAKE;
	MBNG_EVENT_SendOptimizedNRPN(port, chn, nrpn_address, nrpn_value, msb_only);
	MUTEX_MIDIOUT_GIVE;
      }
    }

    return 0; // no error

  } else if( event_type == MBNG_EVENT_TYPE_SYSEX ) {
    // send SysEx over enabled ports
    int i;
    u16 mask = 1;
    for(i=0; i<16; ++i, mask <<= 1) {
      if( item->enabled_ports & mask ) {
	// USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	mios32_midi_port_t port = USB0 + ((i&0xc) << 2) + (i&3);
	MUTEX_MIDIOUT_TAKE;
	MBNG_EVENT_SendSysExStream(port, item);
	MUTEX_MIDIOUT_GIVE;
      }
    }
    return 0; // no error

  } else if( event_type == MBNG_EVENT_TYPE_META ) {
    MUTEX_MIDIOUT_TAKE;
    MBNG_EVENT_ExecMeta(item);
    MUTEX_MIDIOUT_GIVE;
    return 0; // no error

  } else if( event_type == MBNG_EVENT_TYPE_MBFM ) {
    //MUTEX_MIDIOUT_TAKE; //None of these commands generate MIDI messages, so...
    MBNG_EVENT_ExecMBFM(item);
    //MUTEX_MIDIOUT_GIVE;
    return 0; // no error

  } else {
    return -1; // unsupported
  }

  return -1; // unsupported item type
}


/////////////////////////////////////////////////////////////////////////////
//! Sends to a virtual item (which doesn't exist in the pool)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemSendVirtual(mbng_event_item_t *item, mbng_event_item_id_t send_id)
{
  // notify by temporary changing the ID - forwarding disabled
  mbng_event_item_id_t tmp_id = item->id;
  mbng_event_item_id_t tmp_hw_id = item->hw_id;
  mbng_event_item_id_t tmp_fwd_id = item->fwd_id;
  mbng_event_flags_t flags; flags.ALL = item->flags.ALL;
  u8 tmp_map = item->map;
  u16 tmp_pool_address = item->pool_address;

  item->id = send_id;
  item->hw_id = send_id;
  item->fwd_id = 0;
  item->flags.fwd_to_lcd = 0;
  item->map = 0; // we assume that the value has already been mapped
  item->pool_address = 0xffff;

  MBNG_EVENT_ItemReceive(item, item->value, 0, 0); // forwarding not enabled

  item->id = tmp_id;
  item->hw_id = tmp_hw_id;
  item->fwd_id = tmp_fwd_id;
  item->flags.ALL = flags.ALL;
  item->map = tmp_map;
  item->pool_address = tmp_pool_address;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Called when an item should be notified about a new value
//! \retval 0 if no matching condition
//! \retval 1 if matching condition (or no condition)
//! \retval 2 if matching condition and stop requested
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemReceive(mbng_event_item_t *item, u16 value, u8 from_midi, u8 fwd_enabled)
{
  // write operation locked?
  if( from_midi && item->flags.write_locked )
    return 0; // stop here

  // take over new value
  item->value = value;

  // value modified from MIDI?
  item->flags.value_from_midi = from_midi;

  // take over in pool item
  if( item->pool_address < MBNG_EVENT_POOL_MAX_SIZE ) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + item->pool_address);
    pool_item->value = item->value;
    if( item->flags.use_key_or_cc ) // only change secondary value if key_or_cc option selected
      pool_item->secondary_value = item->secondary_value;
    pool_item->flags.value_from_midi = from_midi;
  }

  // item active?
  if( !item->flags.active )
    return 0; // stop here

  // matching condition?
  s32 cond_match;
  if( (cond_match=MBNG_EVENT_ItemCheckMatchingCondition(item)) < 1 )
    return 0; // stop here

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_DEBUG ) {
    MBNG_EVENT_ItemPrint(item, 0);
  }

  switch( item->id & 0xf000 ) {
  //case MBNG_EVENT_CONTROLLER_DISABLED:
  case MBNG_EVENT_CONTROLLER_BUTTON:        MBNG_DIN_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_LED:           MBNG_DOUT_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX: MBNG_MATRIX_DIN_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_LED_MATRIX:    MBNG_MATRIX_DOUT_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_ENC:           MBNG_ENC_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_AIN:           MBNG_AIN_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_AINSER:        MBNG_AINSER_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_MF:            MBNG_MF_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_CV:            MBNG_CV_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_KB:            MBNG_KB_NotifyReceivedValue(item); break;

  case MBNG_EVENT_CONTROLLER_SENDER: {
    int sender_ix = item->id & 0xfff;

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("MBNG_EVENT_ItemReceive(%d, %d) (Sender)\n", sender_ix, item->value);
    }

    // map?
    u8 *map_values;
    int map_len = MBNG_EVENT_MapGet(item->map, &map_values);
    if( map_len > 0 ) {
      item->value = map_values[(value < map_len) ? value : (map_len-1)];
    }

    // send MIDI event
    MBNG_EVENT_ItemSend(item);
  } break;

  case MBNG_EVENT_CONTROLLER_RECEIVER: {
    int receiver_ix = item->id & 0xfff;

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("MBNG_EVENT_ItemReceive(%d, %d) (Receiver)\n", receiver_ix, item->value);
    }

    // map?
    u8 *map_values;
    int map_len = MBNG_EVENT_MapGet(item->map, &map_values);
    if( map_len > 0 ) {
      item->value = map_values[(value < map_len) ? value : (map_len-1)];
    } else {
      // range?
      if( value < item->min )
	item->value = item->min;
      else if( value > item->max )
	item->value = item->max;
    }

    // store in pool
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + item->pool_address);
    pool_item->value = item->value;

    // ENC emulation mode?
    if( item->custom_flags.RECEIVER.emu_enc_hw_id ) {
      s32 incrementer = 0;

      switch( item->custom_flags.RECEIVER.emu_enc_mode ) {
      case MBNG_EVENT_ENC_MODE_40SPEED:
	incrementer = (s32)item->value - 0x40;
	break;

      case MBNG_EVENT_ENC_MODE_00SPEED:
	incrementer = (item->value < 64) ? item->value : -(128-(s32)item->value);
	break;

      case MBNG_EVENT_ENC_MODE_INC00SPEED_DEC40SPEED:
	incrementer = (item->value < 64) ? item->value : -(((s32)item->value) - 0x40);
	break;

      case MBNG_EVENT_ENC_MODE_INC41_DEC3F:
	if( item->value != 0x40 ) {
	  incrementer = (item->value > 0x40) ? 1 : -1;
	}
	break;

      case MBNG_EVENT_ENC_MODE_INC01_DEC7F:
	if( item->value != 0x00 ) {
	  incrementer = (item->value < 0x40) ? 1 : -1;
	}
	break;

      case MBNG_EVENT_ENC_MODE_INC01_DEC41:
	if( item->value != 0x00 && item->value != 0x40 ) {
	  incrementer = (item->value < 0x40) ? 1 : -1;
	}
	break;

    default: { // MBNG_EVENT_ENC_MODE_ABSOLUTE
      // forward value directly to ENC
      mbng_event_item_id_t fwd_id = item->fwd_id;
      item->fwd_id = MBNG_EVENT_CONTROLLER_ENC | item->custom_flags.RECEIVER.emu_enc_hw_id;
      MBNG_EVENT_ItemForward(item);
      item->fwd_id = fwd_id;
    }
    }

    if( incrementer ) {
      MBNG_ENC_NotifyChange(item->custom_flags.RECEIVER.emu_enc_hw_id - 1, incrementer);
    }
    }
  } break;
  }

  // forward
  if( fwd_enabled ) {
    if( item->fwd_id )
      MBNG_EVENT_ItemForward(item);
    else {
      if( item->flags.radio_group )
	MBNG_EVENT_ItemForwardToRadioGroup(item, item->flags.radio_group);
    }

    if( item->flags.fwd_to_lcd && item->pool_address < MBNG_EVENT_POOL_MAX_SIZE ) {
      mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + item->pool_address);
      pool_item->flags.update_lcd = 1;
      last_event_item_id = pool_item->id;
    }
  }

  return cond_match; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Called to forward an event
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemForward(mbng_event_item_t *item)
{
  static u8 recursion_ctr = 0;

  if( !item->fwd_id )
    return -1; // no forwarding enabled

  if( item->fwd_id == item->id )
    return -2; // avoid feedback

  if( recursion_ctr >= MBNG_EVENT_MAX_FWD_RECURSION )
    return -3;
  ++recursion_ctr;

  // search for fwd item
  mbng_event_item_t fwd_item;
  u32 continue_ix = 0;
  u32 num_forwarded = 0;
  do {
    if( MBNG_EVENT_ItemSearchByHwId(item->fwd_id, &fwd_item, &continue_ix) < 0 ) {
      break;
    } else {
      ++num_forwarded;

      // notify item (will also store value in pool item)
      if( fwd_item.flags.use_key_or_cc || fwd_item.flags.use_any_key_or_cc ) // only change secondary value if key_or_cc option selected, or if fwd item allows to change the key value
	fwd_item.secondary_value = item->secondary_value;
      if( item->fwd_value == 0xffff ) { // no forward value
	if( MBNG_EVENT_ItemReceive(&fwd_item, item->value, 0, 1) == 2 )
	  break; // stop has been requested
      } else { // with forward value
	if( MBNG_EVENT_ItemReceive(&fwd_item, item->fwd_value, 0, 1) == 2 )
	  break; // stop has been requested
      }

      // special: if EVENT_RECEIVER forwarded to EVENT_AIN, EVENT_AINSER or EVENT_BUTTON, send also MIDI event
      u16 id_type = item->id & 0xf000;
      u16 fwd_type = fwd_item.id & 0xf000;
      if( id_type == MBNG_EVENT_CONTROLLER_RECEIVER &&
	  (fwd_type == MBNG_EVENT_CONTROLLER_AIN ||
	   fwd_type == MBNG_EVENT_CONTROLLER_AINSER ||
	   fwd_type == MBNG_EVENT_CONTROLLER_BUTTON) ) {
	MBNG_EVENT_ItemSend(&fwd_item);
      }
    }
  } while( continue_ix );

  // no matching ID found - create "virtual" event based on parameters of original item
  if( !num_forwarded ) {
    MBNG_EVENT_ItemSendVirtual(item, item->fwd_id);
  }

  if( recursion_ctr )
    --recursion_ctr;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Called to forward an event to a radio group
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemForwardToRadioGroup(mbng_event_item_t *item, u8 radio_group)
{
  // search for all items in the pool which are part of the radio group
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

    if( pool_item->flags.radio_group == radio_group ) {
      mbng_event_item_t fwd_item;
      MBNG_EVENT_ItemCopy2User(pool_item, &fwd_item);
      fwd_item.value = item->value; // forward the value of the sender

      // value in range?
      // (currently only used to filter MIDI output of an EVENT_SENDER)
      u8 value_in_range = (fwd_item.value >= fwd_item.min) && (fwd_item.value <= fwd_item.max);

      // sender/receiver will map the value
      u16 fwd_id_type = fwd_item.id & 0xf000;
      if( fwd_id_type == MBNG_EVENT_CONTROLLER_SENDER || fwd_id_type == MBNG_EVENT_CONTROLLER_RECEIVER ) {
	u8 *map_values;
	int map_len = MBNG_EVENT_MapGet(fwd_item.map, &map_values);
	if( map_len > 0 ) {
	  fwd_item.value = map_values[(fwd_item.value < map_len) ? fwd_item.value : (map_len-1)];
	}
      }

      // take over value of item in pool item
      if( fwd_item.pool_address < MBNG_EVENT_POOL_MAX_SIZE ) {
	mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + fwd_item.pool_address);
	pool_item->value = fwd_item.value;
	if( fwd_item.flags.use_key_or_cc ) // only change secondary value if key_or_cc option selected
	  pool_item->secondary_value = fwd_item.secondary_value;
	pool_item->flags.value_from_midi = item->flags.value_from_midi;
      }
      if( fwd_item.flags.use_key_or_cc ) // only change secondary value if key_or_cc option selected
	fwd_item.secondary_value = item->secondary_value;

      // notify button/LED element
      if( fwd_id_type == MBNG_EVENT_CONTROLLER_BUTTON )
	MBNG_DIN_NotifyReceivedValue(&fwd_item);
      else if( fwd_id_type == MBNG_EVENT_CONTROLLER_LED )
	MBNG_DOUT_NotifyReceivedValue(&fwd_item);
      else if( fwd_id_type == MBNG_EVENT_CONTROLLER_SENDER && value_in_range ) // or send MIDI value?
	MBNG_EVENT_ItemSend(&fwd_item);

      // and trigger forwarding
      MBNG_EVENT_ItemForward(&fwd_item);
    }

    pool_ptr += pool_item->len;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! called from a controller to notify that a value should be sent
//! \retval 0 if no matching condition
//! \retval 1 if matching condition (or no condition)
//! \retval 2 if matching condition and stop requested
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_NotifySendValue(mbng_event_item_t *item)
{
  // value not modified from MIDI anymore
  item->flags.value_from_midi = 0;


  // take over in pool item
  s16 prev_value = 0;
  //s16 prev_secondary_value = 0;
  if( item->pool_address < MBNG_EVENT_POOL_MAX_SIZE ) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + item->pool_address);
    prev_value = pool_item->value;
    //prev_secondary_value = pool_item->secondary_value;
    pool_item->value = item->value;
    if( item->flags.use_key_or_cc ) // only change secondary value if key_or_cc option selected
      pool_item->secondary_value = item->secondary_value;
    pool_item->flags.value_from_midi = 0;
  }

  // matching condition?
  s32 cond_match;
  if( (cond_match=MBNG_EVENT_ItemCheckMatchingCondition(item)) < 1 )
    return 0; // stop here

  // extra for AIN and AINSER: send NoteOff if required
  {
    mbng_event_ain_sensor_mode_t ain_sensor_mode = MBNG_EVENT_AIN_SENSOR_MODE_NONE;
    if( (item->id & 0xf000) == MBNG_EVENT_CONTROLLER_AIN )
      ain_sensor_mode = item->custom_flags.AIN.ain_sensor_mode;
    else if( (item->id & 0xf000) == MBNG_EVENT_CONTROLLER_AINSER )
      ain_sensor_mode = item->custom_flags.AINSER.ain_sensor_mode;

    if( ain_sensor_mode == MBNG_EVENT_AIN_SENSOR_MODE_NOTE_ON_OFF ) {
      if( (item->flags.type == MBNG_EVENT_TYPE_NOTE_ON || item->flags.type == MBNG_EVENT_TYPE_NOTE_OFF) && prev_value ) {
	s16 tmp_value = item->value;
	u8 tmp_secondary_value = item->secondary_value;

	if( item->flags.use_key_or_cc ) {
	  item->secondary_value = 0;
	  item->value = prev_value;
	  MBNG_EVENT_ItemSend(item);
	} else {
	  item->value = 0;
	  MBNG_EVENT_ItemSend(item);
	}
	item->value = tmp_value;
	item->secondary_value = tmp_secondary_value;
      }
    }
  }

  // send MIDI event
  MBNG_EVENT_ItemSend(item);

  // forward - optionally to whole radio group
  if( item->flags.radio_group )
    MBNG_EVENT_ItemForwardToRadioGroup(item, item->flags.radio_group);
  else
    MBNG_EVENT_ItemForward(item);

  // print label
  if( item->pool_address < MBNG_EVENT_POOL_MAX_SIZE ) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + item->pool_address);
    pool_item->flags.update_lcd = 1;
    last_event_item_id = pool_item->id;
  }

  return cond_match;
}


/////////////////////////////////////////////////////////////////////////////
//! Refresh all items
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_Refresh(void)
{
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

    u8 allow_refresh = 1;
    switch( pool_item->hw_id & 0xf000 ) {
    case MBNG_EVENT_CONTROLLER_SENDER:   allow_refresh = 0; break;
    case MBNG_EVENT_CONTROLLER_RECEIVER: allow_refresh = 0; break;
    }

    if( allow_refresh ) {
      mbng_event_item_t item;
      MBNG_EVENT_ItemCopy2User(pool_item, &item);
      pool_item->flags.update_lcd = 1; // force LCD update
      MBNG_EVENT_ItemReceive(&item, pool_item->value, 1, 1);
    }

    pool_ptr += pool_item->len;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! called from MBNG_EVENT_ReceiveSysEx() when a Syxdump is received
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_EVENT_NotifySyxDump(u16 from_receiver, u16 dump_pos, u8 value)
{
  // search in pool for events which listen to this receiver and dump_pos
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    if( pool_item->syxdump_pos.pos == dump_pos &&
	pool_item->syxdump_pos.receiver == from_receiver ) {

      mbng_event_item_t item;
      MBNG_EVENT_ItemCopy2User(pool_item, &item);
      MBNG_EVENT_ItemReceive(&item, value, 1, 1);
    }
    pool_ptr += pool_item->len;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Prints all items with enabled 'update_lcd' flag\n
//! With force != 0 all active items will be print regardless of this flag
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_UpdateLCD(u8 force)
{
  mbng_event_pool_item_t *last_pool_item = NULL;

  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

    if( pool_item->flags.active && (force || pool_item->flags.update_lcd) ) {
      pool_item->flags.update_lcd = 0;

      mbng_event_item_t item;
      MBNG_EVENT_ItemCopy2User(pool_item, &item);
      MBNG_LCD_PrintItemLabel(&item, NULL, 0);

      // if force: output the last active item (again) at the end
      if( force && last_event_item_id && pool_item->id == last_event_item_id ) {
	last_pool_item = pool_item;
      }
    }

    pool_ptr += pool_item->len;
  }

  // last pool item?
  // (relevant for SCS->Main Page change)
  if( last_pool_item ) {
    mbng_event_item_t item;
    MBNG_EVENT_ItemCopy2User(last_pool_item, &item);
    MBNG_LCD_PrintItemLabel(&item, NULL, 0);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function dumps all MIDI events\n
//! Used in conjunction with snapshots
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_Dump(void)
{
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

    if( !pool_item->flags.no_dump ) {
      mbng_event_item_t item;
      MBNG_EVENT_ItemCopy2User(pool_item, &item);

      if( !item.flags.radio_group || (item.value >= item.min && item.value <= item.max) ) {
	MBNG_EVENT_NotifySendValue(&item);
      }
    }

    pool_ptr += pool_item->len;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called from APP_MIDI_NotifyPackage whenver a new
//! MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // create port mask, and check if this is a supported port (USB0..3, UART0..3, IIC0..3, OSC0..3)
  u8 subport_mask = (1 << (port&3));
  u8 port_class = ((port-0x10) & 0x30)>>2;
  u16 port_mask = subport_mask << port_class;
  if( !port_mask )
    return -1; // not supported

  // Note Off -> Note On with velocity 0
  if( mbng_patch_cfg.convert_note_off_to_on0 ) {
    if( midi_package.event == NoteOff ) {
      midi_package.event = NoteOn;
      midi_package.velocity = 0;
    }
  }

  // on CC:
  u16 nrpn_address = 0xffff; // taken if < 0xffff
  u16 nrpn_value = 0;
  u8 nrpn_msb_only = 0;
  if( midi_package.event == CC ) {

    // track NRPN event
    if( port_mask & MBNG_EVENT_NRPN_RECEIVE_PORTS_MASK ) {
      int port_ix = (port_class | (port & 3)) - MBNG_EVENT_NRPN_RECEIVE_PORTS_OFFSET;
      if( port_ix >= 0 && port_ix < MBNG_EVENT_NRPN_RECEIVE_PORTS ) {
        // NRPN handling
        switch( midi_package.cc_number ) {
        case 0x63: { // Address MSB
	  nrpn_received_address[port_ix][midi_package.chn] &= ~0x3f80;
	  nrpn_received_address[port_ix][midi_package.chn] |= ((midi_package.value << 7) & 0x3f80);

	  if( port != midi_learn_nrpn_port || midi_package.chn != (midi_learn_nrpn_chn-1) ) {
	    midi_learn_nrpn_port = port;
	    midi_learn_nrpn_chn = midi_package.chn + 1;
	    midi_learn_nrpn_valid = 0;
	  }
	  midi_learn_nrpn_valid |= 0x01;
        } break;

        case 0x62: { // Address LSB
	  nrpn_received_address[port_ix][midi_package.chn] &= ~0x007f;
	  nrpn_received_address[port_ix][midi_package.chn] |= (midi_package.value & 0x007f);

	  if( port != midi_learn_nrpn_port || midi_package.chn != (midi_learn_nrpn_chn-1) ) {
	    midi_learn_nrpn_port = port;
	    midi_learn_nrpn_chn = midi_package.chn + 1;
	    midi_learn_nrpn_valid = 0;
	  }
	  midi_learn_nrpn_valid |= 0x02;
        } break;

        case 0x06: { // Data MSB
	  nrpn_received_value[port_ix][midi_package.chn] &= ~0x3f80;
	  nrpn_received_value[port_ix][midi_package.chn] |= ((midi_package.value << 7) & 0x3f80);
	  nrpn_value = nrpn_received_value[port_ix][midi_package.chn]; // pass to parser
	  nrpn_address = nrpn_received_address[port_ix][midi_package.chn];
	  nrpn_msb_only = 1; // for the MsbOnly format
#if 0
	  // MEMO: it's better to update only when LSB has been received
	  if( port != midi_learn_nrpn_port || midi_package.chn != (midi_learn_nrpn_chn-1) ) {
	    midi_learn_nrpn_port = port;
	    midi_learn_nrpn_chn = midi_package.chn + 1;
	    midi_learn_nrpn_valid = 0;
	  } else {
	    // only if valid address has been parsed
	    midi_learn_nrpn_valid |= 0x04;
	  }
#endif
        } break;

        case 0x26: { // Data LSB
	  nrpn_received_value[port_ix][midi_package.chn] &= ~0x007f;
	  nrpn_received_value[port_ix][midi_package.chn] |= (midi_package.value & 0x007f);
	  nrpn_value = nrpn_received_value[port_ix][midi_package.chn]; // pass to parser
	  nrpn_address = nrpn_received_address[port_ix][midi_package.chn];

	  if( port != midi_learn_nrpn_port || midi_package.chn != (midi_learn_nrpn_chn-1) ) {
	    midi_learn_nrpn_port = port;
	    midi_learn_nrpn_chn = midi_package.chn + 1;
	    midi_learn_nrpn_valid = 0;
	  } else {
	    // only if valid address has been parsed
	    // use same valid flag like Data MSB (complete NRPN is valid with 0x07)
	    midi_learn_nrpn_valid |= 0x04;
	  }
        } break;
        }
      }
    }

    // check for "all notes off" command
    if( mbng_patch_cfg.all_notes_off_chn &&
	(midi_package.chn == (mbng_patch_cfg.all_notes_off_chn - 1)) &&
	midi_package.cc_number == 123 ) {
      MBNG_DOUT_Init(0);
    }
  }

  // MIDI Learn Mode
  if( midi_learn_mode ) {
    int value = -1;

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      MIDIMON_Print("MIDI_LEARN:", port, midi_package, MIOS32_TIMESTAMP_Get(), 0);
    }

    if( midi_learn_mode >= 2 && nrpn_address != 0xffff && midi_learn_nrpn_valid == 0x07 ) {
      // TK: it will be interesting if a user will ever notice, that MsbOnly (e.g. for MBSEQ) won't be assigned automatically
      if( nrpn_address != midi_learn_nrpn_address ) {
	midi_learn_min = 0xffff;
	midi_learn_max = 0xffff;
      } else {
	// ignore first value after nrpn address change, because it could happen that we only received the MSB yet...
	value = nrpn_value;
      }

      midi_learn_nrpn_address = nrpn_address;
      midi_learn_nrpn_value = nrpn_value;

      if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("[MIDI_LEARN] Detected NRPN value for #%d\n", midi_learn_nrpn_address);
      }
    }

    if( value < 0 &&
	!(midi_learn_mode >= 2 && midi_package.event == CC &&
	  (midi_package.cc_number == 0x63 || midi_package.cc_number == 0x62 ||
	   midi_package.cc_number == 0x06 || midi_package.cc_number == 0x26)) ) {
      switch( midi_package.event ) {
      case NoteOff:
      case NoteOn:
      case PolyPressure:
      case CC:
	value = midi_package.evnt2;

	if( midi_package.evnt0 != midi_learn_event.evnt0 ||
	    midi_package.evnt1 != midi_learn_event.evnt1 ) {
	  midi_learn_min = 0xffff;
	  midi_learn_max = 0xffff;
	}
	break;

      case ProgramChange:
      case Aftertouch:
	value = midi_package.evnt1;

	if( midi_package.evnt0 != midi_learn_event.evnt0 ) {
	  midi_learn_min = 0xffff;
	  midi_learn_max = 0xffff;
	}
	break;

      case PitchBend:
	value = midi_package.evnt1 | ((u16)midi_package.evnt2 << 7);

	if( midi_package.evnt0 != midi_learn_event.evnt0 ) {
	  midi_learn_min = 0xffff;
	  midi_learn_max = 0xffff;
	}
	break;
      }      
    }

    if( value >= 0 ) {
      midi_learn_event.ALL = midi_package.ALL;

      if( midi_learn_min == 0xffff || value < midi_learn_min )
	midi_learn_min = value;
      if( midi_learn_max == 0xffff || value > midi_learn_max )
	midi_learn_max = value;

      if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("[MIDI_LEARN] value=%d  range=%d:%d\n",
		  value,
		  (midi_learn_min == 0xffff) ? 0 : midi_learn_min,
		  (midi_learn_max == 0xffff) ? 127 : midi_learn_max);
      }
    }
  }

  // Logic Control Meters
  if( midi_package.evnt0 == 0xd0 &&
      ((port >= USB0 && port <= USB3) || (port >= UART0 && port <= UART3)) ) {
    u8 port_ix = (((port & 0xf0) == USB0) ? 0 : 4) + (port & 0x3);
    MBNG_EVENT_LCMeters_Set(port_ix, midi_package.evnt1);
  }

  // search in pool for matching events
  u8 evnt0 = midi_package.evnt0;
  u8 evnt1 = midi_package.evnt1;
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    if( pool_item->data_begin == evnt0 && pool_item->len_stream ) { // timing critical
      // first byte is matching - now we've a bit more time for checking
      
      if( (pool_item->hw_id & 0xf000) == MBNG_EVENT_CONTROLLER_SENDER ) { // a sender doesn't receive
	pool_ptr += pool_item->len;
	continue;
      }

      if( !(pool_item->enabled_ports & port_mask) ) { // port not enabled
	pool_ptr += pool_item->len;
	continue;
      }

      mbng_event_type_t event_type = ((mbng_event_flags_t)pool_item->flags).type;
      if( event_type <= MBNG_EVENT_TYPE_CC ) {
	u8 *stream = &pool_item->data_begin;
	if( pool_item->flags.use_any_key_or_cc || stream[1] == evnt1 ) { // || pool_item->secondary_value >= 128 || evnt1 == pool_item->secondary_value ) {
	  mbng_event_item_t item;
	  MBNG_EVENT_ItemCopy2User(pool_item, &item);
	  if( item.flags.use_key_or_cc ) {
	    item.secondary_value = midi_package.value;
	    MBNG_EVENT_ItemReceive(&item, midi_package.evnt1, 1, 1);
	  } else {
	    item.secondary_value = midi_package.evnt1;
	    MBNG_EVENT_ItemReceive(&item, midi_package.value, 1, 1);
	  }
	} else {
	  // EXTRA for button/led matrices
	  int matrix = (pool_item->hw_id & 0x0fff) - 1;
	  int num_pins = -1;

	  switch( pool_item->hw_id & 0xf000 ) {
	  case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX: {
	    if( matrix >= 0 && matrix < MBNG_PATCH_NUM_MATRIX_DIN ) {
	      mbng_patch_matrix_din_entry_t *m = (mbng_patch_matrix_din_entry_t *)&mbng_patch_matrix_din[matrix];

	      if( m->sr_din1 ) {
		u8 row_size = m->sr_din2 ? 16 : 8;
		num_pins = row_size * row_size;
	      }
	    }
	  } break;
	  case MBNG_EVENT_CONTROLLER_LED_MATRIX: {
	    if( matrix >= 0 && matrix < MBNG_PATCH_NUM_MATRIX_DOUT ) {
	      mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[matrix];

	      if( m->sr_dout_r1 && !pool_item->flags.led_matrix_pattern ) {
		u8 row_size = m->sr_dout_r2 ? 16 : 8; // we assume that the same condition is valid for dout_g2 and dout_b2
		num_pins = row_size * row_size;
	      }
	    }
	  } break;
	  }

	  if( num_pins >= 0 ) {
	    int first_evnt1 = stream[1];
	    if( evnt1 >= first_evnt1 && evnt1 < (first_evnt1 + num_pins) ) {
	      mbng_event_item_t item;
	      MBNG_EVENT_ItemCopy2User(pool_item, &item);
	      item.matrix_pin = evnt1 - first_evnt1;
	      MBNG_EVENT_ItemReceive(&item, midi_package.value, 1, 1);
	    }
	  }
	}
      } else if( event_type <= MBNG_EVENT_TYPE_AFTERTOUCH ) {
	mbng_event_item_t item;
	MBNG_EVENT_ItemCopy2User(pool_item, &item);
	MBNG_EVENT_ItemReceive(&item, evnt1, 1, 1);
      } else if( event_type == MBNG_EVENT_TYPE_PITCHBEND ) {
	mbng_event_item_t item;
	MBNG_EVENT_ItemCopy2User(pool_item, &item);
	MBNG_EVENT_ItemReceive(&item, evnt1 | ((u16)midi_package.value << 7), 1, 1);
      } else if( event_type == MBNG_EVENT_TYPE_NRPN ) {
	u8 *stream = &pool_item->data_begin;
	u16 expected_address = stream[1] | ((u16)stream[2] << 7);
	mbng_event_nrpn_format_t nrpn_format = stream[3];
	if( nrpn_address == expected_address &&
	    (!nrpn_msb_only || nrpn_format == MBNG_EVENT_NRPN_FORMAT_MSB_ONLY) ) {
	  mbng_event_item_t item;
	  MBNG_EVENT_ItemCopy2User(pool_item, &item);

	  if( nrpn_format == MBNG_EVENT_NRPN_FORMAT_MSB_ONLY )
	    MBNG_EVENT_ItemReceive(&item, nrpn_value / 128, 1, 1);
	  else
	    MBNG_EVENT_ItemReceive(&item, nrpn_value, 1, 1);
	}
      } else {
	// no additional event types yet...
      }
    }
    pool_ptr += pool_item->len;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function should be called from APP_SYSEX_Parser on incoming SysEx data
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in)
{
  // create port mask, and check if this is a supported port (USB0..3, UART0..3, IIC0..3, OSC0..3)
  u8 subport_mask = (1 << (port&3));
  u8 port_class = ((port-0x10) & 0x30)>>2;
  u16 port_mask = subport_mask << port_class;
  if( !port_mask )
    return -1; // not supported

  // search in pool for matching SysEx streams
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    mbng_event_type_t event_type = ((mbng_event_flags_t)pool_item->flags).type;
    if( event_type == MBNG_EVENT_TYPE_SYSEX ) {
      u8 parse_sysex = 1;

      if( (pool_item->hw_id & 0xf000) == MBNG_EVENT_CONTROLLER_SENDER ) // a sender doesn't receive
	parse_sysex = 0;

      if( !(pool_item->enabled_ports & port_mask) ) // port not enabled
	parse_sysex = 0;

      // receiving a SysEx dump?
      if( pool_item->sysex_runtime_var.dump ) {
	if( midi_in >= 0xf0 ) {
	  // notify event when all values have been received
	  mbng_event_item_t item;
	  MBNG_EVENT_ItemCopy2User(pool_item, &item);
	  MBNG_EVENT_ItemReceive(&item, pool_item->value, 1, 1);

	  // and reset
	  pool_item->sysex_runtime_var.ALL = 0; // finished
	} else {
	  // notify all events which listen to this dump
	  MBNG_EVENT_NotifySyxDump(pool_item->id & 0xfff, pool_item->sysex_runtime_var.match_ctr+1, midi_in);

	  // waiting for next byte
	  ++pool_item->sysex_runtime_var.match_ctr;
	  parse_sysex = 0;
	}
      } else {
	// always reset if 0xf0 has been received
	if( midi_in == 0xf0 ) { //  || midi_in == 0xf7  --- disabled, could be too confusing for user if the specified stream shouldn't contain F7
	  pool_item->sysex_runtime_var.ALL = 0;
	}
      }

      if( parse_sysex && pool_item->sysex_runtime_var.match_ctr < pool_item->len_stream ) {
	u8 *stream = ((u8 *)&pool_item->data_begin) + pool_item->sysex_runtime_var.match_ctr;
	u8 again = 0;
	do {
	  if( *stream == 0xff ) { // SysEx variable
	    u8 match = 0;
	    switch( *++stream ) {
	    case MBNG_EVENT_SYSEX_VAR_DEV:        match = midi_in == mbng_patch_cfg.sysex_dev; break;
	    case MBNG_EVENT_SYSEX_VAR_PAT:        match = midi_in == mbng_patch_cfg.sysex_pat; break;
	    case MBNG_EVENT_SYSEX_VAR_BNK:        match = midi_in == mbng_patch_cfg.sysex_bnk; break;
	    case MBNG_EVENT_SYSEX_VAR_INS:        match = midi_in == mbng_patch_cfg.sysex_ins; break;
	    case MBNG_EVENT_SYSEX_VAR_CHN:        match = midi_in == mbng_patch_cfg.sysex_chn; break;
	    case MBNG_EVENT_SYSEX_VAR_CHK_START:  match = 1; again = 1; break;
	    case MBNG_EVENT_SYSEX_VAR_CHK:        match = 1; break; // ignore checksum
	    case MBNG_EVENT_SYSEX_VAR_CHK_INV:    match = 1; break; // ignore checksum
	    case MBNG_EVENT_SYSEX_VAR_VAL:        match = 1; pool_item->value = (pool_item->value & 0xff80) | (midi_in & 0x7f); break;
	    case MBNG_EVENT_SYSEX_VAR_VAL_H:      match = 1; pool_item->value = (pool_item->value & 0xf07f) | (((u16)midi_in & 0x7f) << 7); break;
	    case MBNG_EVENT_SYSEX_VAR_VAL_N1:     match = 1; pool_item->value = (pool_item->value & 0xfff0) | (((u16)midi_in <<  0) & 0x000f); break;
	    case MBNG_EVENT_SYSEX_VAR_VAL_N2:     match = 1; pool_item->value = (pool_item->value & 0xff0f) | (((u16)midi_in <<  4) & 0x00f0); break;
	    case MBNG_EVENT_SYSEX_VAR_VAL_N3:     match = 1; pool_item->value = (pool_item->value & 0xf0ff) | (((u16)midi_in <<  8) & 0x0f00); break;
	    case MBNG_EVENT_SYSEX_VAR_VAL_N4:     match = 1; pool_item->value = (pool_item->value & 0x0fff) | (((u16)midi_in << 12) & 0xf000); break;
	    case MBNG_EVENT_SYSEX_VAR_IGNORE:     match = 1; break;

	    case MBNG_EVENT_SYSEX_VAR_DUMP:
	      match = 1; 
	      pool_item->sysex_runtime_var.dump = 1; // enable dump receiver
	      pool_item->sysex_runtime_var.match_ctr = 0;

	      // notify all events which listen to this dump
	      MBNG_EVENT_NotifySyxDump(pool_item->id & 0xfff, pool_item->sysex_runtime_var.match_ctr, midi_in);
	      break;

	    case MBNG_EVENT_SYSEX_VAR_CURSOR:
	      match = 1;
	      pool_item->value = midi_in; // store cursor
	      pool_item->sysex_runtime_var.cursor_set = 1;
	      break;

	    case MBNG_EVENT_SYSEX_VAR_TXT:
	    case MBNG_EVENT_SYSEX_VAR_TXT56:
	    case MBNG_EVENT_SYSEX_VAR_LABEL: { // handle like txt (although ^label has actually a different meaning: it dumps out the label string)
	      match = 1;

	      // wrap at 64 or 56?
	      u8 x_wrap = (*stream == MBNG_EVENT_SYSEX_VAR_TXT56) ? 56 : 64;

	      pool_item->sysex_runtime_var.txt = 1;
	      if( midi_in < 0x80 ) {
		// take initial LCD device and position from item
		// print also the label (e.g. to initialize font)
		mbng_event_item_t item;
		MBNG_EVENT_ItemCopy2User(pool_item, &item);

		// set cursor?
		if( !pool_item->sysex_runtime_var.cursor_set ) {
		  pool_item->sysex_runtime_var.cursor_set = 1;
		  pool_item->value = 0;
		}

		// X and Y pos
		u8 x = pool_item->value % x_wrap;
		u8 y = pool_item->value / x_wrap;

		// mapped X?
		u8 *map_values;
		int map_len = MBNG_EVENT_MapGet(item.map, &map_values);
		if( map_len > 0 ) {
		  if( x < map_len )
		    x = map_values[x];
		  else
		    x = map_values[map_len-1];
		}

		// multiply Y due to big GLCD font?
		if( MIOS32_LCD_TypeIsGLCD() ) {
		  u8 *glcd_font = MBNG_LCD_FontGet();
		  u16 font_height = glcd_font ? glcd_font[MIOS32_LCD_FONT_HEIGHT_IX] : 8;
		  y *= (font_height / 8);
		}

		// add to cursor pos
		item.lcd_x += x;
		item.lcd_y += y;

		MUTEX_LCD_TAKE;
		if( item.label ) {
		  MBNG_LCD_PrintItemLabel(&item, NULL, 0);
		} else {
		  MBNG_LCD_FontInit('n');
		  MBNG_LCD_CursorSet(item.lcd, item.lcd_x, item.lcd_y);
		}
		MBNG_LCD_PrintChar(midi_in); // print char
		MUTEX_LCD_GIVE;
	      }
	      ++pool_item->value; // increment cursor
	    } break;
	    }
	    if( match ) {
	      if( !pool_item->sysex_runtime_var.dump && !pool_item->sysex_runtime_var.txt ) {
		pool_item->sysex_runtime_var.match_ctr += 2;
	      }
	    } else {
	      pool_item->sysex_runtime_var.ALL = 0;
	    }
	  } else if( *stream == midi_in ) { // matching byte
	    // begin of stream?
	    if( midi_in == 0xf0 ) {
	      pool_item->value = 0;
	      pool_item->sysex_runtime_var.ALL = 0;
	    }

	    // end of stream?
	    if( midi_in == 0xf7 ) {
	      pool_item->sysex_runtime_var.ALL = 0;

	      // all values matching!
	      mbng_event_item_t item;
	      MBNG_EVENT_ItemCopy2User(pool_item, &item);
	      MBNG_EVENT_ItemReceive(&item, pool_item->value, 1, 1);
	    } else {
	      ++pool_item->sysex_runtime_var.match_ctr;
	    }
	  } else { // no matching byte
	    pool_item->sysex_runtime_var.ALL = 0;
	  }

	  // end of parse stream reached?
	  if( pool_item->sysex_runtime_var.match_ctr == pool_item->len_stream ) {
	    again = 0;

	    pool_item->sysex_runtime_var.ALL = 0;

	    // all values matching!
	    mbng_event_item_t item;
	    MBNG_EVENT_ItemCopy2User(pool_item, &item);
	    MBNG_EVENT_ItemReceive(&item, pool_item->value, 1, 1);
	  }
	} while( again );
      }
    }
    pool_ptr += pool_item->len;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called from MBFM module when it wants to play with lights
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ReceiveMBFM(mbng_event_mbfm_type_t mbfm_type, u8 index, u16 value)
{
  u8 type_as_u8 = (u8)mbfm_type;
  //u8 count = 0;
  //DEBUG_MSG("Lights type %s index %d: %d", MBNG_EVENT_ItemMBFMTypeStrGet(mbfm_type), index, value);
  u8 num_bytes_expecting = MBNG_EVENT_ItemMBFMNumBytesGet(mbfm_type);
  //u8 secondvalue;
  // search in pool for matching MBFM events
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u8 *data_ptr;
  mbng_event_item_t item;
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    if(((mbng_event_flags_t)pool_item->flags).type == MBNG_EVENT_TYPE_MBFM ) {
      data_ptr = (u8 *)&pool_item->data_begin;
      if(data_ptr[2] == type_as_u8){
        //Matching item
        //DEBUG_MSG("Found matching item %d stream %d %d %d, expecting %d bytes", i, data_ptr[1], data_ptr[2], data_ptr[3], num_bytes_expecting);
        if(!num_bytes_expecting || data_ptr[3] == index){
          //DEBUG_MSG("Index %d matches too", index);
          MBNG_EVENT_ItemCopy2User(pool_item, &item);
          MBNG_EVENT_ItemReceive(&item, value, 1, 1);
        }
      }
    }
    pool_ptr += pool_item->len;
  }

  return 0; // no error
}


//! \}
