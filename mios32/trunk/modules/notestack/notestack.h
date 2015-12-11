// $Id: notestack.h 850 2010-01-24 22:31:41Z tk $
/*
 * Header file for Notestack module
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _NOTESTACK_H
#define _NOTESTACK_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  NOTESTACK_MODE_PUSH_TOP = 0,
  NOTESTACK_MODE_PUSH_BOTTOM,
  NOTESTACK_MODE_PUSH_TOP_HOLD,
  NOTESTACK_MODE_PUSH_BOTTOM_HOLD,
  NOTESTACK_MODE_SORT,
  NOTESTACK_MODE_SORT_HOLD
} notestack_mode_t;


typedef union {
  u16 ALL;
  struct {
    u8 note:7;
    u8 depressed:1;
    u8 tag;
  };
} notestack_item_t;


typedef struct {
  notestack_mode_t mode;
  u8               size;
  u8               len;
  notestack_item_t *note_items;
} notestack_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 NOTESTACK_Init(notestack_t *n, notestack_mode_t mode, notestack_item_t *note_items, u8 size);

extern s32 NOTESTACK_Push(notestack_t *n, u8 new_note, u8 tag);
extern s32 NOTESTACK_Pop(notestack_t *n, u8 old_note);
extern s32 NOTESTACK_CountActiveNotes(notestack_t *n);
extern s32 NOTESTACK_RemoveNonActiveNotes(notestack_t *n);
extern s32 NOTESTACK_Clear(notestack_t *n);


extern s32 NOTESTACK_SendDebugMessage(notestack_t *n);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* _NOTESTACK_H */
