// $Id: seq_label.h 729 2009-09-30 13:32:55Z tk $
/*
 * Header file for SEQ Label Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_LABEL_H
#define _SEQ_LABEL_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_LABEL_Init(u32 mode);

extern s32 SEQ_LABEL_NumPresets(void);
extern s32 SEQ_LABEL_CopyPreset(u8 num, char *dst);

extern s32 SEQ_LABEL_NumPresetsCategory(void);
extern s32 SEQ_LABEL_CopyPresetCategory(u8 num, char *dst);

extern s32 SEQ_LABEL_NumPresetsDrum(void);
extern s32 SEQ_LABEL_CopyPresetDrum(u8 num, char *dst);



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_LABEL_H */
