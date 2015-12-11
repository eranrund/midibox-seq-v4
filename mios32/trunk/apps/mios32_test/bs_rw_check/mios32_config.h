/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

#define MIOS32_LCD_BOOT_MSG_LINE1 "BS Check"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2009 M. Maechler"

// enable banksticks. must be equal or more than BS_CHECK_NUM_DEVICES (app.h)
// will be ignored if BS_CHECK_USE_FRAM_LAYER is set
#define MIOS32_IIC_NUM 2
#define MIOS32_IIC_BS_NUM 8

#define FRAM_IIC_PORT 1



#endif /* _MIOS32_CONFIG_H */
