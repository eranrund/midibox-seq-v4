// $Id: mios32_config.h 1567 2012-12-02 11:48:32Z tk $
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
#define MIOS32_LCD_BOOT_MSG_LINE1 "USB OSC MIDI Proxy"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2010 T.Klose"

// Following settings allow to customize the USB device descriptor
#define MIOS32_USB_VENDOR_STR   "midibox.org" // you will see this in the USB device description
#define MIOS32_USB_PRODUCT_STR  "USB OSC MIDI Proxy" // you will see this in the MIDI device list

#define MIOS32_USB_MIDI_NUM_PORTS 2           // we provide 2 USB ports


// EEPROM emulation
#define EEPROM_EMULATED_SIZE 128

// magic number in EEPROM - if it doesn't exist at address 0x00..0x03, the EEPROM will be cleared
#define EEPROM_MAGIC_NUMBER 0x47114200


#endif /* _MIOS32_CONFIG_H */
