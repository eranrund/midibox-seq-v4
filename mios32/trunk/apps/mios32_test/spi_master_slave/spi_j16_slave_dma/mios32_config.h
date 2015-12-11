// $Id: mios32_config.h 1411 2012-01-28 12:51:01Z tk $
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
#define MIOS32_LCD_BOOT_MSG_LINE1 "SPI J16 Slave DMA"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2012 T.Klose"


#endif /* _MIOS32_CONFIG_H */
