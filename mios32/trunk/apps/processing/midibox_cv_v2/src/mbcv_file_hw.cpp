// $Id: mbcv_file_hw.cpp 1912 2014-01-03 23:15:07Z tk $
/*
 * Hardware Soft-Config File access functions
 *
 * NOTE: before accessing the SD Card, the upper level function should
 * synchronize with the SD Card semaphore!
 *   MUTEX_SDCARD_TAKE; // to take the semaphore
 *   MUTEX_SDCARD_GIVE; // to release the semaphore
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
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

#include <aout.h>
#include "file.h"
#include "mbcv_file.h"
#include "mbcv_file_hw.h"

#include "mbcv_hwcfg.h"
#include "mbcv_map.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the MBMBCV files located?
// use "/" for root
// use "/<dir>/" for a subdirectory in root
// use "/<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define MBCV_FILES_PATH "/"
//#define MBCV_FILES_PATH "/MySongs/"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid:1;
  unsigned config_locked: 1;   // file is only loaded after startup
} mbcv_file_hw_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static mbcv_file_hw_info_t mbcv_file_hw_info;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_Init(u32 mode)
{
  MBCV_FILE_HW_Unload();

  mbcv_file_hw_info.config_locked = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Loads hardware config file
// Called from MBCV_FILE_HWheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_Load(void)
{
  s32 error = 0;

  if( mbcv_file_hw_info.config_locked ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBCV_FILE_HW] HW config file not loaded again\n");
#endif
  } else {
    error = MBCV_FILE_HW_Read();
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBCV_FILE_HW] Tried to open HW config file, status: %d\n", error);
#endif
    mbcv_file_hw_info.config_locked = 1;
  }

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads config file
// Called from MBCV_FILE_HWheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_Unload(void)
{
  mbcv_file_hw_info.valid = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns 1 if config file valid
// Returns 0 if config file not valid
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_Valid(void)
{
  return mbcv_file_hw_info.valid;
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if config has been read and is locked (no change anymore)
// Returns 0 if config hasn't been read yet (Button/Encoder/LED functions should be disabled)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_ConfigLocked(void)
{
  return mbcv_file_hw_info.config_locked;
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if config has been read and is locked (no change anymore)
// Returns 0 if config hasn't been read yet (Button/Encoder/LED functions should be disabled)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_LockConfig(void)
{
  mbcv_file_hw_info.config_locked = 1;

  return 0; // no error
}




/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  return l; // value is valid
}


/////////////////////////////////////////////////////////////////////////////
// reads the hardware config file content
// returns < 0 on errors (error codes are documented in mbcv_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_Read(void)
{
  s32 status = 0;
  mbcv_file_hw_info_t *info = &mbcv_file_hw_info;
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBCV_HW.CV2", MBCV_FILES_PATH);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_HW] Open config file '%s'\n", filepath);
#endif

  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBCV_FILE_HW] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read config values
  char line_buffer[128];
  do {
    status=FILE_ReadLine((u8 *)line_buffer, 80);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[MBCV_FILE_HW] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t;";
      char *brkt;
      char *parameter;
      int hlp;

      if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {

	////////////////////////////////////////////////////////////////////////////////////////////
	// ignore comments
	////////////////////////////////////////////////////////////////////////////////////////////
	if( *parameter == '#' ) {

	////////////////////////////////////////////////////////////////////////////////////////////
	// BUTTON_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "BUTTON_", 7) == 0 ) {
	  parameter += 7;

	  char *word = strtok_r(NULL, separators, &brkt);

	  // M1..M8 -> SR 24..31
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR in BUTTON_%s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  word = strtok_r(NULL, separators, &brkt);
	  s32 pin = get_dec(word);
	  if( pin < 0 || pin >= 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR in BUTTON_%s definition: invalid pin value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  u8 din_value = ((sr-1)<<3) | pin;

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[MBCV_FILE_HW] Button %s: SR %d Pin %d (DIN: 0x%02x)", line_buffer, sr, pin, din_value);
#endif

	  if( strcmp(parameter, "LFO1") == 0 ) {
	    mbcv_hwcfg_button.lfo1 = din_value;
	  } else if( strcmp(parameter, "LFO2") == 0 ) {
	    mbcv_hwcfg_button.lfo2 = din_value;
	  } else if( strcmp(parameter, "ENV1") == 0 ) {
	    mbcv_hwcfg_button.env1 = din_value;
	  } else if( strcmp(parameter, "ENV2") == 0 ) {
	    mbcv_hwcfg_button.env2 = din_value;
	  } else if( strncmp(parameter, "CV", 2) == 0 && // GP%d
		     (hlp=atoi(parameter+2)) >= 1 && hlp <= CV_SE_NUM ) {
	    mbcv_hwcfg_button.cv[hlp-1] = din_value;
	  } else if( strncmp(parameter, "ENCBANK", 7) == 0 && // ENCBANK%d
		     (hlp=atoi(parameter+7)) >= 1 && hlp <= MBCV_LRE_NUM_BANKS ) {
	    mbcv_hwcfg_button.enc_bank[hlp-1] = din_value;
	  } else if( strncmp(parameter, "CV_AND_ENCBANK", 14) == 0 && // CV_AND_ENCBANK%d
		     (hlp=atoi(parameter+14)) >= 1 && hlp <= CV_SE_NUM ) {
	    mbcv_hwcfg_button.cv_and_enc_bank[hlp-1] = din_value;
	  } else if( strncmp(parameter, "SCOPE", 5) == 0 && // SCOPE%d
		     (hlp=atoi(parameter+5)) >= 1 && hlp <= CV_SCOPE_NUM ) {
	    mbcv_hwcfg_button.scope[hlp-1] = din_value;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR: unknown button function 'BUTTON_%s'!", parameter);
#endif
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// LREx
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "LRE", 3) == 0 ) {
	  s32 lre = parameter[3] - '1'; // user counts from 1, we count from 0
	  if( lre < 0 || lre >= MBCV_LRE_NUM ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR in %s definition: invalid LRE ID '%s' (expecting LRE1_ or LRE2_)!", parameter);
#endif
	    continue;
	  }
	  parameter += 4;

	  if( strncmp(parameter, "_ENC", 4) == 0 ) {
	    parameter += 4;
	    s32 enc = get_dec(parameter);
	    if( enc < 1 || enc > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_ENC%s definition: expecting ENC1..ENC16!!", lre+1, parameter);
#endif
	      continue;
	    }
	    --enc; // counting from 0

	    char *word = strtok_r(NULL, separators, &brkt);
	    s32 sr = get_dec(word);
	    if( sr < 0 || sr > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_ENC%s definition: invalid first value '%s'!", lre+1, parameter, word);
#endif
	      continue;
	    }

	    word = strtok_r(NULL, separators, &brkt);
	    s32 pin = get_dec(word);
	    if( pin < 0 || pin >= 8 ) { // should we check for odd pin values (1/3/5/7) as well?
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_ENC%s definition: invalid pin value '%s'!", lre+1, parameter, word);
#endif
	      continue;
	    }

	    word = strtok_r(NULL, separators, &brkt);
	    mios32_enc_type_t enc_type = DISABLED;
	    if( word == NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_ENC%s definition: missing encoder type!", lre+1, parameter);
#endif
	      continue;
	    } else if( strcmp(word, "NON_DETENTED") == 0 ) {
	      enc_type = NON_DETENTED;
	    } else if( strcmp(word, "DETENTED1") == 0 ) {
	      enc_type = DETENTED1;
	    } else if( strcmp(word, "DETENTED2") == 0 ) {
	      enc_type = DETENTED2;
	    } else if( strcmp(word, "DETENTED3") == 0 ) {
	      enc_type = DETENTED3;
	    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_ENC%s definition: invalid type '%s'!", lre+1, parameter, word);
#endif
	      continue;
	    }

#if DEBUG_VERBOSE_LEVEL >= 3
	    DEBUG_MSG("[MBCV_FILE_HW] LRE%d_ENC%d: SR %d Pin %d Type %d", lre+1, enc+1, sr, pin, enc_type);
#endif

	    // LRE encoders are starting at +1, since the first encoder is allocated by SCS
	    mios32_enc_config_t enc_config;
	    enc_config = MIOS32_ENC_ConfigGet(16*lre + enc+1);
	    enc_config.cfg.type = enc_type;
	    enc_config.cfg.sr = sr;
	    enc_config.cfg.pos = pin;
	    enc_config.cfg.speed = NORMAL;
	    enc_config.cfg.speed_par = 0;
	    MIOS32_ENC_ConfigSet(16*lre + enc+1, enc_config);

	  } else {
	    char *word = strtok_r(NULL, separators, &brkt);
	    s32 value = get_dec(word);

	    if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_%s definition: expecting value!", lre+1, parameter, word);
#endif
	      continue;
	    } else {
	      if( strcmp(parameter, "_ENABLED") == 0 ) {
		if( value > 2 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_%s definition: expecting 0..2!", lre+1, parameter, word);
#endif
		  continue;
		}
		mbcv_hwcfg_lre[lre].enabled = value;
	      } else if( strcmp(parameter, "_LEDRING_SELECT_DOUT_SR1") == 0 ) {
		if( value > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_%s definition: expecting 0..16!", lre+1, parameter, word);
#endif
		  continue;
		}
		mbcv_hwcfg_lre[lre].ledring_select_sr1 = value;
	      } else if( strcmp(parameter, "_LEDRING_SELECT_DOUT_SR2") == 0 ) {
		if( value > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_%s definition: expecting 0..16!", lre+1, parameter, word);
#endif
		  continue;
		}
		mbcv_hwcfg_lre[lre].ledring_select_sr2 = value;
	      } else if( strcmp(parameter, "_LEDRING_PATTERN_DOUT_SR1") == 0 ) {
		if( value > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_%s definition: expecting 0..16!", lre+1, parameter, word);
#endif
		  continue;
		}
		mbcv_hwcfg_lre[lre].ledring_pattern_sr1 = value;
	      } else if( strcmp(parameter, "_LEDRING_PATTERN_DOUT_SR2") == 0 ) {
		if( value > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_%s definition: expecting 0..16!", lre+1, parameter, word);
#endif
		  continue;
		}
		mbcv_hwcfg_lre[lre].ledring_pattern_sr2 = value;
	      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MBCV_FILE_HW] ERROR: unknown LRE parameter name 'LRE%d%s'!", lre+1, parameter);
#endif
	      }
	    }
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// DOUT assignments
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "GATE_CV_DOUT_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR: invalid SR number: %s %s", parameter, word);
#endif
	    continue;
	  }
	  mbcv_hwcfg_dout.gate_sr = sr;

	} else if( strcmp(parameter, "CLK_DOUT_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR: invalid SR number: %s %s", parameter, word);
#endif
	    continue;
	  }
	  mbcv_hwcfg_dout.clk_sr = sr;


	////////////////////////////////////////////////////////////////////////////////////////////
	// AOUT Interface
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcasecmp(parameter, "AOUT_IF") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  int aout_type;
	  for(aout_type=0; aout_type<AOUT_NUM_IF; ++aout_type) {
	    if( strcasecmp(word, MBCV_MAP_IfNameGet((aout_if_t)aout_type)) == 0 )
	      break;
	  }

	  if( aout_type == AOUT_NUM_IF ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR invalid AOUT interface name for parameter '%s': %s\n", parameter, word);
#endif
	  } else {
	    MBCV_MAP_IfSet((aout_if_t)aout_type);
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// unknown
	////////////////////////////////////////////////////////////////////////////////////////////
	} else {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBCV_FILE_HW] ERROR: unknown parameter: %s", line_buffer);
#endif
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBCV_FILE_HW] ERROR: no space separator in following line: %s", line_buffer);
#endif
      }
    }

  } while( status >= 1 );

  FILE_ReadClose(&file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_HW] ERROR while reading file, status: %d\n", status);
#endif
    return MBCV_FILE_HW_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  return 0; // no error
}
