// $Id: mios32_config.h 1086 2010-10-03 17:37:29Z tk $
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
#define MIOS32_LCD_BOOT_MSG_LINE1 "FastScan Matrix"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2010 T.Klose"


// disables the default SRIO scan routine in programming_models/traditional/main.c
// allows to implement an own handler
// -> see app.c, TASK_MatrixScan()
#define MIOS32_DONT_SERVICE_SRIO_SCAN 1


#endif /* _MIOS32_CONFIG_H */
