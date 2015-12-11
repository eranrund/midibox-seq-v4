// $Id: seq_hwcfg.h 2131 2015-01-17 20:38:25Z tk $
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

#define SEQ_HWCFG_NUM_ENCODERS     1


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 gp_din_l_sr;
  u8 gp_din_r_sr;

  u8 bar1;
  u8 bar2;
  u8 bar3;
  u8 bar4;

  u8 seq1;
  u8 seq2;

  u8 load;
  u8 save;

  u8 copy;
  u8 paste;
  u8 clear;
  u8 undo;

  u8 master;
  u8 tap_tempo;
  u8 stop;
  u8 play;

  u8 pause;
  u8 metronome;
  u8 ext_restart;

  u8 trigger;
  u8 length;
  u8 progression;
  u8 groove;
  u8 echo;
  u8 humanizer;
  u8 lfo;
  u8 scale;

  u8 mute;
  u8 midichn;

  u8 rec_arm;
  u8 rec_step;
  u8 rec_live;
  u8 rec_poly;
  u8 inout_fwd;
  u8 transpose;
} seq_hwcfg_button_t;


typedef struct {
  u8 gp_dout_l_sr;
  u8 gp_dout_r_sr;

  u8 pos_dout_l_sr;
  u8 pos_dout_r_sr;

  u8 bar1;
  u8 bar2;
  u8 bar3;
  u8 bar4;

  u8 seq1;
  u8 seq2;

  u8 load;
  u8 save;

  u8 copy;
  u8 paste;
  u8 clear;
  u8 undo;

  u8 master;
  u8 tap_tempo;
  u8 stop;
  u8 play;

  u8 pause;
  u8 metronome;
  u8 ext_restart;

  u8 trigger;
  u8 length;
  u8 progression;
  u8 groove;
  u8 echo;
  u8 humanizer;
  u8 lfo;
  u8 scale;

  u8 mute;
  u8 midichn;

  u8 rec_arm;
  u8 rec_step;
  u8 rec_live;
  u8 rec_poly;
  u8 inout_fwd;
  u8 transpose;
} seq_hwcfg_led_t;


typedef struct {
  u8 bpm_fast_speed;
} seq_hwcfg_enc_t;

typedef struct {
  u8 enabled;
  u8 dout_gp_mapping;
} seq_hwcfg_blm8x8_t;

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

// following constants can be safely changed (therefore documented)

// max. number of SRs which can be used for CV gates (each SR provides 8 gates)
#define SEQ_HWCFG_NUM_SR_CV_GATES 1

// max. number of SRs which can be used for triggering gates (each SR provides 8 gates)
#define SEQ_HWCFG_NUM_SR_DOUT_GATES 8


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_HWCFG_Init(u32 mode);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_hwcfg_button_t seq_hwcfg_button;
extern seq_hwcfg_led_t seq_hwcfg_led;
extern seq_hwcfg_enc_t seq_hwcfg_enc;
extern seq_hwcfg_blm8x8_t seq_hwcfg_blm8x8;
extern seq_hwcfg_midi_remote_t seq_hwcfg_midi_remote;
extern seq_hwcfg_track_cc_t seq_hwcfg_track_cc;
extern seq_hwcfg_step_digits_t seq_hwcfg_step_digits;
extern seq_hwcfg_tpd_t seq_hwcfg_tpd;
extern seq_hwcfg_bpm_digits_t seq_hwcfg_bpm_digits;

extern u8 seq_hwcfg_dout_gate_sr[SEQ_HWCFG_NUM_SR_DOUT_GATES];
extern u8 seq_hwcfg_dout_gate_1ms;
extern u8 seq_hwcfg_cv_gate_sr[SEQ_HWCFG_NUM_SR_CV_GATES];
extern u8 seq_hwcfg_clk_sr;

#endif /* _SEQ_HWCFG_H */
