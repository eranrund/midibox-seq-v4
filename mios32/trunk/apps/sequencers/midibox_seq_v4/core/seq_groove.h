// $Id: seq_groove.h 2197 2015-07-25 21:36:23Z tk $
/*
 * Header file for groove routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_GROOVE_H
#define _SEQ_GROOVE_H

#include "seq_layer.h"

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SEQ_GROOVE_NUM_PRESETS    7
#define SEQ_GROOVE_NUM_TEMPLATES 16


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  char name[13];
  u8   num_steps;
  s8   add_step_delay[16];
  s8   add_step_length[16];
  s8   add_step_velocity[16];
} seq_groove_entry_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_GROOVE_Init(u32 mode);

extern char *SEQ_GROOVE_NameGet(u8 groove);

extern s32 SEQ_GROOVE_DelayGet(u8 track, u8 step);
extern s32 SEQ_GROOVE_Event(u8 track, u8 step, seq_layer_evnt_t *e);

extern s32 SEQ_GROOVE_Clear(u8 groove);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_groove_entry_t seq_groove_templates[SEQ_GROOVE_NUM_TEMPLATES];
extern const seq_groove_entry_t seq_groove_presets[SEQ_GROOVE_NUM_PRESETS];
extern u16 seq_groove_ui_local_selection;

#endif /* _SEQ_GROOVE_H */
