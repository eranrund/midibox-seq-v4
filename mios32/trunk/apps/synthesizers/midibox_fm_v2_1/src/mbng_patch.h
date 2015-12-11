// $Id: mbng_patch.h 2045 2014-09-04 19:36:25Z tk $
/*
 * Patch Layer for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNG_PATCH_H
#define _MBNG_PATCH_H

#include <ainser.h>
//#include <scs.h>
//#include <scs_lcd.h>
#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

#define MBNG_PATCH_NUM_DIN        1024
#define MBNG_PATCH_NUM_DOUT       1024
#define MBNG_PATCH_NUM_ENC         128

#if MIOS32_AIN_CHANNEL_MASK == 0x3f
#define MBNG_PATCH_NUM_AIN           6
#elif MIOS32_AIN_CHANNEL_MASK == 0xff
#define MBNG_PATCH_NUM_AIN           8
#else
# error "please adapt for this unsupported MIOS32_AIN_CHANNEL_MASK"
#endif

#define MBNG_PATCH_NUM_AINSER_MODULES AINSER_NUM_MODULES
#define MBNG_PATCH_NUM_MF_MODULES    4
#define MBNG_PATCH_NUM_CV_CHANNELS   AOUT_NUM_CHANNELS
#define MBNG_PATCH_NUM_MATRIX_DIN    16
#define MBNG_PATCH_NUM_MATRIX_DOUT   16
#define MBNG_PATCH_NUM_MATRIX_DOUT_PATTERNS 4

#define MBNG_PATCH_NUM_MATRIX_ROWS_MAX   16
#define MBNG_PATCH_NUM_MATRIX_COLORS_MAX  3

// depends on the resolution
#define MBNG_PATCH_AIN_MAX_VALUE 4095
#define MBNG_PATCH_AINSER_MAX_VALUE 4095

// STM32F1: none
// STM32F4: J10A and J10B
// LPC17: J10 and J28
#if defined(MIOS32_FAMILY_STM32F10x)
# define MBNG_PATCH_NUM_DIO 0
#else
# define MBNG_PATCH_NUM_DIO 2
#endif


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u8 ALL;
  
  struct {
    u8 inverted_sel:1;
    u8 inverted_row:1;
    u8 mirrored_row:1;
    u8 max72xx_enabled:1;
    u8 swap_rows_columns:1; //Added by Sauraen
  };
} mbng_patch_matrix_flags_t;

typedef struct {
  u16 button_emu_id_offset;
  u8 num_rows;
  mbng_patch_matrix_flags_t flags;
  u8 sr_dout_sel1;
  u8 sr_dout_sel2;
  u8 sr_din1;
  u8 sr_din2;
} mbng_patch_matrix_din_entry_t;

typedef struct {
  u16 led_emu_id_offset;
  u8 num_rows;
  mbng_patch_matrix_flags_t flags;
  u8 sr_dout_sel1;
  u8 sr_dout_sel2;
  u8 sr_dout_r1;
  u8 sr_dout_r2;
  u8 sr_dout_g1;
  u8 sr_dout_g2;
  u8 sr_dout_b1;
  u8 sr_dout_b2;
  u8 lc_meter_port;
} mbng_patch_matrix_dout_entry_t;

typedef union {
  u8 ALL;
  
  struct {
    u8 chn:4;
    u8 enabled:1;
  };
} mbng_patch_mf_flags_t;

typedef struct {
  u16 ts_first_button_id;
  u8 midi_in_port;
  u8 midi_out_port;
  u8 config_port;
  mbng_patch_mf_flags_t flags;
} mbng_patch_mf_entry_t;


typedef union {
  u32 ALL;
  
  struct {
    u32 min:12;
    u32 max:12;
    u32 spread_center:1;
  };
} mbng_patch_ain_calibration_t;

typedef struct {
  u8 enable_mask;
  mbng_patch_ain_calibration_t cali[MBNG_PATCH_NUM_AIN];
} mbng_patch_ain_entry_t;

typedef union {
  u8 ALL;
  
  struct {
    u8 cs:1;
    u8 resolution:4;
  };
} mbng_patch_ainser_flags_t;

typedef struct {
  mbng_patch_ainser_flags_t flags;
  mbng_patch_ain_calibration_t cali[AINSER_NUM_PINS];
} mbng_patch_ainser_entry_t;

/*
#define MBNG_PATCH_SCS_BUTTONS (SCS_NUM_MENU_ITEMS+1)
typedef struct {
  u16 button_emu_id[MBNG_PATCH_SCS_BUTTONS];
  u16 enc_emu_id;
} mbng_patch_scs_t;
*/

typedef struct {
  u8 global_chn;
  u8 all_notes_off_chn;
  u8 convert_note_off_to_on0;
  u8 sysex_dev;
  u8 sysex_pat;
  u8 sysex_bnk;
  u8 sysex_ins;
  u8 sysex_chn;
} mbng_patch_cfg_t;

typedef enum {
  MBNG_PATCH_DIO_CFG_MODE_Off = 0,
  MBNG_PATCH_DIO_CFG_MODE_DIN,
  MBNG_PATCH_DIO_CFG_MODE_DOUT
} mbng_patch_dio_cfg_mode_t;

typedef struct {
  u8   mode:2; // mbng_patch_dio_cfg_mode_t
  u8   emu_sr:8;
} mbng_patch_dio_cfg_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_PATCH_Init(u32 mode);

extern s32 MBNG_PATCH_Load(char *filename);
extern s32 MBNG_PATCH_Store(char *filename);


/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////

extern mbng_patch_matrix_din_entry_t mbng_patch_matrix_din[MBNG_PATCH_NUM_MATRIX_DIN];
extern mbng_patch_matrix_dout_entry_t mbng_patch_matrix_dout[MBNG_PATCH_NUM_MATRIX_DOUT];

extern mbng_patch_ain_entry_t mbng_patch_ain;

extern mbng_patch_ainser_entry_t mbng_patch_ainser[MBNG_PATCH_NUM_AINSER_MODULES];

extern mbng_patch_mf_entry_t mbng_patch_mf[MBNG_PATCH_NUM_MF_MODULES];

extern char mbng_patch_aout_spi_rc_pin;
extern char mbng_patch_max72xx_spi_rc_pin;

//extern mbng_patch_scs_t mbng_patch_scs;

extern mbng_patch_cfg_t mbng_patch_cfg;

#if MBNG_PATCH_NUM_DIO > 0
extern mbng_patch_dio_cfg_t mbng_patch_dio_cfg[MBNG_PATCH_NUM_DIO];
#endif

#endif /* _MBNG_PATCH_H */
