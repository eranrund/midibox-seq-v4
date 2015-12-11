// $Id: mbcv_file.h 1912 2014-01-03 23:15:07Z tk $
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

#ifndef _MBCV_FILE_H
#define _MBCV_FILE_H

// for compatibility with DOSFS
// TODO: change
#define MAX_PATH 100

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


// additional error codes
// see also basic error codes which are documented in file.h

// used by seq_file_b.cpp
#define MBCV_FILE_B_ERR_INVALID_BANK    -128 // invalid bank number
#define MBCV_FILE_B_ERR_INVALID_XXX     -129 // reserved
#define MBCV_FILE_B_ERR_INVALID_PATCH   -130 // invalid patch number
#define MBCV_FILE_B_ERR_FORMAT          -131 // invalid bank file format
#define MBCV_FILE_B_ERR_READ            -132 // error while reading file (exact error status cannot be determined anymore)
#define MBCV_FILE_B_ERR_WRITE           -133 // error while writing file (exact error status cannot be determined anymore)
#define MBCV_FILE_B_ERR_NO_FILE         -134 // no or invalid bank file
#define MBCV_FILE_B_ERR_P_TOO_LARGE     -135 // during patch write: patch too large for slot in bank

// used by mbcv_file_p.cpp
#define MBCV_FILE_P_ERR_INVALID_BANK    -144 // invalid bank number
#define MBCV_FILE_P_ERR_INVALID_GROUP   -145 // invalid group number
#define MBCV_FILE_P_ERR_INVALID_PATCH   -146 // invalid patch number
#define MBCV_FILE_P_ERR_FORMAT          -147 // invalid bank file format
#define MBCV_FILE_P_ERR_READ            -148 // error while reading file (exact error status cannot be determined anymore)
#define MBCV_FILE_P_ERR_WRITE           -149 // error while writing file (exact error status cannot be determined anymore)
#define MBCV_FILE_P_ERR_NO_FILE         -150 // no or invalid bank file
#define MBCV_FILE_P_ERR_P_TOO_LARGE     -151 // during patch write: patch too large for slot in bank

// used by seq_file_hw.cpp
#define MBCV_FILE_HW_ERR_FORMAT         -160 // invalid config file format
#define MBCV_FILE_HW_ERR_READ           -261 // error while reading file (exact error status cannot be determined anymore)
#define MBCV_FILE_HW_ERR_WRITE          -262 // error while writing file (exact error status cannot be determined anymore)
#define MBCV_FILE_HW_ERR_NO_FILE        -263 // no or invalid config file


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBCV_FILE_Init(u32 mode);

extern s32 MBCV_FILE_LoadAllFiles(u8 including_hw);
extern s32 MBCV_FILE_UnloadAllFiles(void);
extern s32 MBCV_FILE_CreateDefaultFiles(void);

extern s32 MBCV_FILE_StatusMsgSet(char *msg);
extern char *MBCV_FILE_StatusMsgGet(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBCV_FILE_H */
