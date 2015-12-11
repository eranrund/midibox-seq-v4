// $Id: synth_file_b.h 1261 2011-07-17 18:21:45Z tk $
/*
 * Header for file functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SYNTH_FILE_B_H
#define _SYNTH_FILE_B_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SYNTH_FILE_B_NUM_BANKS 4


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SYNTH_FILE_B_Init(u32 mode);
extern s32 SYNTH_FILE_B_LoadAllBanks(void);
extern s32 SYNTH_FILE_B_UnloadAllBanks(void);

extern s32 SYNTH_FILE_B_NumPatches(u8 bank);

extern s32 SYNTH_FILE_B_Create(u8 bank);
extern s32 SYNTH_FILE_B_Open(u8 bank);

extern s32 SYNTH_FILE_B_PatchRead(u8 bank, u8 patch, u8 target_group);
extern s32 SYNTH_FILE_B_PatchWrite(u8 bank, u8 patch, u8 source_group, u8 rename_if_empty_name);

extern s32 SYNTH_FILE_B_PatchPeekName(u8 bank, u8 patch, u8 non_cached, char *patch_name);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SYNTH_FILE_B_H */
