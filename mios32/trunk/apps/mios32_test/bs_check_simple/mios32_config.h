// $Id: mios32_config.h 1882 2013-12-07 21:32:05Z tk $
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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIOS32_BS"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2011 T.Klose"

// selecting the 24LC64 of the LPCEXPRESSO board, connected to I2C1
#define MIOS32_IIC_BS_NUM 1
#define MIOS32_IIC_BS_PORT 0
#define MIOS32_IIC_BS0_SIZE 32768
//8192

#endif /* _MIOS32_CONFIG_H */
