// $Id: mbng_file_k.h 2237 2015-11-08 18:25:25Z tk $
/*
 * Header for file functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNG_FILE_K_H
#define _MBNG_FILE_K_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// limited by common 8.3 directory entry format
#define MBNG_FILE_K_FILENAME_LEN 8

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_FILE_K_Init(u32 mode);
extern s32 MBNG_FILE_K_Load(char *filename);
extern s32 MBNG_FILE_K_Unload(void);

extern s32 MBNG_FILE_K_Valid(void);

extern s32 MBNG_FILE_K_Read(char *filename);
extern s32 MBNG_FILE_K_Write(char *filename);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_FILE_K_H */
