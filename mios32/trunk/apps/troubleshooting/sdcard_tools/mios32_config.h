// $Id: mios32_config.h 968 2010-03-13 13:38:58Z philetaylor $
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

// The boot message which is print during startup and returned on a SysEx query
#define MIOS32_LCD_BOOT_MSG_LINE1 "SD Card Tools"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2010 T.Klose and P.Taylor"

// FatFs configuration: support long filenames
#define FATFS_USE_LFN 1
#define FATFS_MAX_LFN 255

#endif /* _MIOS32_CONFIG_H */
