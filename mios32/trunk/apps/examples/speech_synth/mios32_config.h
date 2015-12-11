// $Id: mios32_config.h 1247 2011-07-10 21:29:37Z tk $
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
#define MIOS32_LCD_BOOT_MSG_LINE1 "EmbSpeech Demo"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2011 T.Klose"

// name of USB device
#define MIOS32_USB_PRODUCT_STR MIOS32_LCD_BOOT_MSG_LINE1

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

#if defined(MIOS32_FAMILY_STM32F10x)
// I2S device connected to J8 (-> SPI1), therefore we have to use SPI0 (-> J16) for SRIO chain
# define MIOS32_SRIO_SPI 0
#endif

// I2S support has to be enabled explicitely
#define MIOS32_USE_I2S

// enable MCLK pin (not for STM32 primer)
#ifdef MIOS32_BOARD_STM32_PRIMER
# define MIOS32_I2S_MCLK_ENABLE  0
#else
# define MIOS32_I2S_MCLK_ENABLE  1
#endif

#endif /* _MIOS32_CONFIG_H */
