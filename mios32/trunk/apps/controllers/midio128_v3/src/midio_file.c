// $Id: midio_file.c 1653 2013-01-09 23:08:59Z tk $
/*
 * File access functions for MIDIO128
 *
 * NOTE: before accessing the SD Card, the upper level function should
 * synchronize with the SD Card semaphore!
 *   MUTEX_SDCARD_TAKE; // to take the semaphore
 *   MUTEX_SDCARD_GIVE; // to release the semaphore
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>

#include "file.h"
#include "tasks.h"
#include "midio_file.h"
#include "midio_file_p.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages
// and useful info messages during backups
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static char sd_card_msg[13];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_Init(u32 mode)
{
  s32 status = 0;

  strcpy(sd_card_msg, "SD Card?"); // 12 chars maximum

  status |= FILE_Init(0);
  status |= MIDIO_FILE_P_Init(0);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Loads all files
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_LoadAllFiles(u8 including_hw)
{
  s32 status = 0;

  if( including_hw ) {
    //status |= MIDIO_FILE_HW_Load();
  }

  status |= MIDIO_FILE_P_Load("DEFAULT");

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// invalidate all file infos
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_UnloadAllFiles(void)
{
  s32 status = 0;
  status |= MIDIO_FILE_P_Unload();
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// creates the default files if they don't exist on SD Card
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_CreateDefaultFiles(void)
{
  s32 status;

  portENTER_CRITICAL();

  // check if patch file exists
  if( !MIDIO_FILE_P_Valid() ) {
    // create new one
    DEBUG_MSG("Creating initial DEFAULT.MIO file\n");

    if( (status=MIDIO_FILE_P_Write("DEFAULT")) < 0 ) {
      DEBUG_MSG("Failed to create file! (status: %d)\n", status);
    }
  }

  portEXIT_CRITICAL();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// sets the SD Card status
// 12 characters max.
// if msg == NULL: ok, no special status, print filename
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_StatusMsgSet(char *msg)
{
  if( msg == NULL ) {
    sd_card_msg[0] = 0;
  } else {
    memcpy(sd_card_msg, msg, 13);
    sd_card_msg[12] = 0; // ensure that terminator set
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns the current SD Card status
// if NULL: ok, no special status, print filename
/////////////////////////////////////////////////////////////////////////////
char *MIDIO_FILE_StatusMsgGet(void)
{
  return sd_card_msg[0] ? sd_card_msg : NULL;
}

