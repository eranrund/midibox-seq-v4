// $Id: mios32_config.h 2029 2014-08-09 20:13:05Z tk $
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
#define MIOS32_LCD_BOOT_MSG_LINE1 "USB MIDI 4x4 V1.010"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2014 T.Klose"

// Following settings allow to customize the USB device descriptor
#define MIOS32_USB_VENDOR_STR   "midibox.org" // you will see this in the USB device description
#define MIOS32_USB_PRODUCT_STR  "USB MIDI 4x4" // you will see this in the MIDI device list

#define MIOS32_USB_MIDI_NUM_PORTS 4           // we provide 4 USB ports

// enable all 4 UARTs
#define MIOS32_UART_NUM 4

#endif /* _MIOS32_CONFIG_H */
