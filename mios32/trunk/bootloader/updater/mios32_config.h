// $Id: mios32_config.h 2096 2014-12-05 22:04:15Z tk $
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
#define MIOS32_LCD_BOOT_MSG_LINE1 "Bootloader 1.018" // 16 chars!
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2014 T.Klose"


#endif /* _MIOS32_CONFIG_H */
