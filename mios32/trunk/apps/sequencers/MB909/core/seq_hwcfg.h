// $Id: seq_hwcfg.h 1811 2013-06-25 20:50:00Z tk $
/*
 * Header file for HW configuration routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_HWCFG_H
#define _SEQ_HWCFG_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// changing these constants requires changes in seq_hwcfg.c, seq_file_hw.c and probably seq_ui.c

#define SEQ_HWCFG_NUM_ENCODERS     18

#define SEQ_HWCFG_NUM_GP           16
#define SEQ_HWCFG_NUM_TRACK         4
#define SEQ_HWCFG_NUM_GROUP         4
#define SEQ_HWCFG_NUM_DIRECT_TRACK 16
#define SEQ_HWCFG_NUM_PAR_LAYER     3
#define SEQ_HWCFG_NUM_TRG_LAYER     3
#define SEQ_HWCFG_NUM_DIRECT_BOOKMARK 16

// following constants can be safely changed (therefore documented)

// max. number of SRs which can be used for triggering gates (each SR provides 8 gates)
#define SEQ_HWCFG_NUM_SR_DOUT_GATES 8


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 down;
  u8 up;
  u8 left;
  u8 right;

  u8 scrub;
  u8 metronome;
  u8 record;
  u8 live;

  u8 stop;
  u8 pause;
  u8 play;
  u8 rew;
  u8 fwd;
  u8 loop;
  u8 follow;

  u8 menu;
  u8 select;
  u8 exit;
  u8 bookmark;

  u8 track[SEQ_HWCFG_NUM_TRACK];

  u8 direct_track[SEQ_HWCFG_NUM_DIRECT_TRACK];

  u8 par_layer[SEQ_HWCFG_NUM_PAR_LAYER];

  u8 direct_bookmark[SEQ_HWCFG_NUM_DIRECT_BOOKMARK];

  u8 edit;
  u8 mute;
  u8 pattern;
  u8 song;

  u8 solo;
  u8 fast;
  u8 fast2;
  u8 all;

  u8 gp[SEQ_HWCFG_NUM_GP];

  u8 group[SEQ_HWCFG_NUM_GROUP];

  u8 trg_layer[SEQ_HWCFG_NUM_TRG_LAYER];

  u8 utility;
  u8 step_view;
  u8 trg_layer_sel;
  u8 par_layer_sel;
  u8 track_sel;

  u8 tap_tempo;
  u8 tempo_preset;
  u8 ext_restart;

  u8 copy;
  u8 paste;
  u8 clear;
  u8 undo;

  u8 mixer;

  u8 save;
  u8 save_all;

  u8 track_mode;
  u8 track_groove;
  u8 track_length;
  u8 track_direction;
  u8 track_morph;
  u8 track_transpose;

  u8 mute_all_tracks;
  u8 mute_track_layers;
  u8 mute_all_tracks_and_layers;
  u8 unmute_all_tracks;
  u8 unmute_track_layers;
  u8 unmute_all_tracks_and_layers;

  u8 footswitch;
  u8 pattern_remix;
} seq_hwcfg_button_t;


typedef struct {
  u32 fast:1;
  u32 fast2:1;
  u32 all:1;
  u32 all_with_triggers:1;
  u32 solo:1;
  u32 metronome:1;
  u32 loop:1;
  u32 follow:1;
  u32 scrub:1;
  u32 menu:1;
  u32 bookmark:1;
  u32 step_view:1;
  u32 trg_layer:1;
  u32 par_layer:1;
  u32 track_sel:1;
  u32 tempo_preset:1;
} seq_hwcfg_button_beh_t;


typedef struct {
  u8 gp_dout_l_sr;
  u8 gp_dout_r_sr;
  u8 gp_dout_l2_sr;
  u8 gp_dout_r2_sr;
  u8 tracks_dout_l_sr;
  u8 tracks_dout_r_sr;

  u8 track[SEQ_HWCFG_NUM_TRACK];

  u8 par_layer[SEQ_HWCFG_NUM_PAR_LAYER];

  u8 beat;

  u8 midi_in_combined;
  u8 midi_out_combined;

  u8 edit;
  u8 mute;
  u8 pattern;
  u8 song;

  u8 solo;
  u8 fast;
  u8 fast2;
  u8 all;

  u8 group[SEQ_HWCFG_NUM_GROUP];

  u8 trg_layer[SEQ_HWCFG_NUM_TRG_LAYER];

  u8 play;
  u8 stop;
  u8 pause;
  u8 rew;
  u8 fwd;
  u8 loop;
  u8 follow;

  u8 exit;
  u8 select;
  u8 menu;
  u8 bookmark;
  u8 scrub;
  u8 metronome;

  u8 utility;
  u8 copy;
  u8 paste;
  u8 clear;
  u8 undo;

  u8 record;
  u8 live;

  u8 step_view;
  u8 trg_layer_sel;
  u8 par_layer_sel;
  u8 track_sel;

  u8 tap_tempo;
  u8 tempo_preset;
  u8 ext_restart;

  u8 down;
  u8 up;

  u8 mixer;

  u8 track_mode;
  u8 track_groove;
  u8 track_length;
  u8 track_direction;
  u8 track_transpose;
  u8 track_morph;

  u8 mute_all_tracks;
  u8 mute_track_layers;
  u8 mute_all_tracks_and_layers;
  u8 unmute_all_tracks;
  u8 unmute_track_layers;
  u8 unmute_all_tracks_and_layers;
} seq_hwcfg_led_t;


typedef struct {
  u8 datawheel_fast_speed;
  u8 bpm_fast_speed;
  u8 gp_fast_speed;
  u8 auto_fast;
} seq_hwcfg_enc_t;


typedef struct {
  u8 enabled:1;
  u8 dout_duocolour:2;
  u8 buttons_enabled:1;
  u8 buttons_no_ui:1;
  u8 gp_always_select_menu_page:1;
} seq_hwcfg_blm_t;


typedef struct {
  u8 enabled:1;
  u8 dout_gp_mapping:1;
  u8 din_gp_mapping:1;
} seq_hwcfg_blm8x8_t;

typedef struct {
  u8 enabled;
  u8 multimachine;
} seq_hwcfg_mb909_t;

typedef struct {
  u8 enabled;
  u8 segments_sr;
  u8 common1_pin;
  u8 common2_pin;
  u8 common3_pin;
  u8 common4_pin;
} seq_hwcfg_bpm_digits_t;

typedef struct {
  u8 enabled;
  u8 segments_sr;
  u8 common1_pin;
  u8 common2_pin;
  u8 common3_pin;
} seq_hwcfg_step_digits_t;

typedef struct {
  u8 enabled;
  u8 columns_sr;
  u8 rows_sr;
} seq_hwcfg_tpd_t;

typedef struct {
  u8 key;
  u8 cc;
} seq_hwcfg_midi_remote_t;

typedef struct {
  u8 mode;
  mios32_midi_port_t port;
  u8 chn;
  u8 cc;
} seq_hwcfg_track_cc_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_HWCFG_Init(u32 mode);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_hwcfg_button_t seq_hwcfg_button;
extern seq_hwcfg_button_beh_t seq_hwcfg_button_beh;
extern seq_hwcfg_led_t seq_hwcfg_led;
extern seq_hwcfg_blm_t seq_hwcfg_blm;
extern seq_hwcfg_blm8x8_t seq_hwcfg_blm8x8;
extern seq_hwcfg_enc_t seq_hwcfg_enc;
extern seq_hwcfg_bpm_digits_t seq_hwcfg_bpm_digits;
extern seq_hwcfg_step_digits_t seq_hwcfg_step_digits;
extern seq_hwcfg_tpd_t seq_hwcfg_tpd;
extern seq_hwcfg_midi_remote_t seq_hwcfg_midi_remote;
///<<<<<<< .mine
extern seq_hwcfg_mb909_t seq_hwcfg_mb909;
//=======
extern seq_hwcfg_track_cc_t seq_hwcfg_track_cc;
//>>>>>>> .r1826

extern u8 seq_hwcfg_dout_gate_sr[SEQ_HWCFG_NUM_SR_DOUT_GATES];
extern u8 seq_hwcfg_dout_gate_1ms;

#endif /* _SEQ_HWCFG_H */
