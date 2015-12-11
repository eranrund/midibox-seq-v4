// $Id: mios32_iic_midi.h 1795 2013-06-01 22:13:42Z tk $
/*
 * Header file for IIC MIDI functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_IIC_MIDI_H
#define _MIOS32_IIC_MIDI_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// base address of first MBHP_IIC_MIDI module
#ifndef MIOS32_IIC_MIDI_ADDR_BASE
#define MIOS32_IIC_MIDI_ADDR_BASE 0x10
#endif

// IIC port
#ifndef MIOS32_IIC_MIDI_PORT
#define MIOS32_IIC_MIDI_PORT 0
#endif

// max number of IIC MIDI interfaces (0..8)
// has to be set to >0 in mios32_config.h to enable IIC MIDI!
#ifndef MIOS32_IIC_MIDI_NUM
#define MIOS32_IIC_MIDI_NUM 0
#endif

// Interface and RI_N port configuration
// _ENABLED:   0 = interface disabled
//             1 = interface enabled, don't poll receive status (OUT only)
//             2 = interface enabled, poll receive status
//             3 = interface enabled, check RI_N pin instead of polling receive status
// _RI_N_PORT: Port to which RI_N is connected
// _RI_N_PIN:  pin to which RI_N is connected
#if defined(MIOS32_FAMILY_STM32F10x) || defined(MIOS32_FAMILY_STM32F4xx)
#ifndef MIOS32_IIC_MIDI0_ENABLED
#define MIOS32_IIC_MIDI0_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI0_RI_N_PORT
#define MIOS32_IIC_MIDI0_RI_N_PORT  GPIOC
#endif
#ifndef MIOS32_IIC_MIDI0_RI_N_PIN
#define MIOS32_IIC_MIDI0_RI_N_PIN   GPIO_Pin_0
#endif

#ifndef MIOS32_IIC_MIDI1_ENABLED
#define MIOS32_IIC_MIDI1_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI1_RI_N_PORT
#define MIOS32_IIC_MIDI1_RI_N_PORT  GPIOC
#endif
#ifndef MIOS32_IIC_MIDI1_RI_N_PIN
#define MIOS32_IIC_MIDI1_RI_N_PIN   GPIO_Pin_1
#endif

#ifndef MIOS32_IIC_MIDI2_ENABLED
#define MIOS32_IIC_MIDI2_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI2_RI_N_PORT
#define MIOS32_IIC_MIDI2_RI_N_PORT  GPIOC
#endif
#ifndef MIOS32_IIC_MIDI2_RI_N_PIN
#define MIOS32_IIC_MIDI2_RI_N_PIN   GPIO_Pin_2
#endif

#ifndef MIOS32_IIC_MIDI3_ENABLED
#define MIOS32_IIC_MIDI3_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI3_RI_N_PORT
#define MIOS32_IIC_MIDI3_RI_N_PORT  GPIOC
#endif
#ifndef MIOS32_IIC_MIDI3_RI_N_PIN
#define MIOS32_IIC_MIDI3_RI_N_PIN   GPIO_Pin_3
#endif

#ifndef MIOS32_IIC_MIDI4_ENABLED
#define MIOS32_IIC_MIDI4_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI4_RI_N_PORT
#define MIOS32_IIC_MIDI4_RI_N_PORT  GPIOA
#endif
#ifndef MIOS32_IIC_MIDI4_RI_N_PIN
#define MIOS32_IIC_MIDI4_RI_N_PIN   GPIO_Pin_0
#endif

#ifndef MIOS32_IIC_MIDI5_ENABLED
#define MIOS32_IIC_MIDI5_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI5_RI_N_PORT
#define MIOS32_IIC_MIDI5_RI_N_PORT  GPIOA
#endif
#ifndef MIOS32_IIC_MIDI5_RI_N_PIN
#define MIOS32_IIC_MIDI5_RI_N_PIN   GPIO_Pin_1
#endif

#ifndef MIOS32_IIC_MIDI6_ENABLED
#define MIOS32_IIC_MIDI6_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI6_RI_N_PORT
#define MIOS32_IIC_MIDI6_RI_N_PORT  GPIOA
#endif
#ifndef MIOS32_IIC_MIDI6_RI_N_PIN
#define MIOS32_IIC_MIDI6_RI_N_PIN   GPIO_Pin_2
#endif

#ifndef MIOS32_IIC_MIDI7_ENABLED
#define MIOS32_IIC_MIDI7_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI7_RI_N_PORT
#define MIOS32_IIC_MIDI7_RI_N_PORT  GPIOA
#endif
#ifndef MIOS32_IIC_MIDI7_RI_N_PIN
#define MIOS32_IIC_MIDI7_RI_N_PIN   GPIO_Pin_3
#endif

#elif defined(MIOS32_FAMILY_LPC17xx)

#ifndef MIOS32_IIC_MIDI0_ENABLED
#define MIOS32_IIC_MIDI0_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI0_RI_N_PORT
#define MIOS32_IIC_MIDI0_RI_N_PORT  2
#endif
#ifndef MIOS32_IIC_MIDI0_RI_N_PIN
#define MIOS32_IIC_MIDI0_RI_N_PIN   2
#endif

#ifndef MIOS32_IIC_MIDI1_ENABLED
#define MIOS32_IIC_MIDI1_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI1_RI_N_PORT
#define MIOS32_IIC_MIDI1_RI_N_PORT  2
#endif
#ifndef MIOS32_IIC_MIDI1_RI_N_PIN
#define MIOS32_IIC_MIDI1_RI_N_PIN   3
#endif

#ifndef MIOS32_IIC_MIDI2_ENABLED
#define MIOS32_IIC_MIDI2_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI2_RI_N_PORT
#define MIOS32_IIC_MIDI2_RI_N_PORT  2
#endif
#ifndef MIOS32_IIC_MIDI2_RI_N_PIN
#define MIOS32_IIC_MIDI2_RI_N_PIN   4
#endif

#ifndef MIOS32_IIC_MIDI3_ENABLED
#define MIOS32_IIC_MIDI3_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI3_RI_N_PORT
#define MIOS32_IIC_MIDI3_RI_N_PORT  2
#endif
#ifndef MIOS32_IIC_MIDI3_RI_N_PIN
#define MIOS32_IIC_MIDI3_RI_N_PIN   5
#endif

#ifndef MIOS32_IIC_MIDI4_ENABLED
#define MIOS32_IIC_MIDI4_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI4_RI_N_PORT
#define MIOS32_IIC_MIDI4_RI_N_PORT  2
#endif
#ifndef MIOS32_IIC_MIDI4_RI_N_PIN
#define MIOS32_IIC_MIDI4_RI_N_PIN   6
#endif

#ifndef MIOS32_IIC_MIDI5_ENABLED
#define MIOS32_IIC_MIDI5_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI5_RI_N_PORT
#define MIOS32_IIC_MIDI5_RI_N_PORT  2
#endif
#ifndef MIOS32_IIC_MIDI5_RI_N_PIN
#define MIOS32_IIC_MIDI5_RI_N_PIN   7
#endif

#ifndef MIOS32_IIC_MIDI6_ENABLED
#define MIOS32_IIC_MIDI6_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI6_RI_N_PORT
#define MIOS32_IIC_MIDI6_RI_N_PORT  2
#endif
#ifndef MIOS32_IIC_MIDI6_RI_N_PIN
#define MIOS32_IIC_MIDI6_RI_N_PIN   8
#endif

#ifndef MIOS32_IIC_MIDI7_ENABLED
#define MIOS32_IIC_MIDI7_ENABLED    2
#endif
#ifndef MIOS32_IIC_MIDI7_RI_N_PORT
#define MIOS32_IIC_MIDI7_RI_N_PORT  1
#endif
#ifndef MIOS32_IIC_MIDI7_RI_N_PIN
#define MIOS32_IIC_MIDI7_RI_N_PIN   18
#endif

#else
# warning "mios32_iic_midi.h not prepared for this MIOS32_FAMILY!"
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_IIC_MIDI_Init(u32 mode);

extern s32 MIOS32_IIC_MIDI_ScanInterfaces(void);
extern s32 MIOS32_IIC_MIDI_CheckAvailable(u8 iic_port);

extern s32 MIOS32_IIC_MIDI_RS_OptimisationSet(u8 iic_port, u8 enable);
extern s32 MIOS32_IIC_MIDI_RS_OptimisationGet(u8 iic_port);
extern s32 MIOS32_IIC_MIDI_RS_Reset(u8 iic_port);

extern s32 MIOS32_IIC_MIDI_Periodic_mS(void);

extern s32 MIOS32_IIC_MIDI_PackageSend_NonBlocking(u8 iic_port, mios32_midi_package_t package);
extern s32 MIOS32_IIC_MIDI_PackageSend(u8 iic_port, mios32_midi_package_t package);
extern s32 MIOS32_IIC_MIDI_PackageReceive_NonBlocking(u8 iic_port, mios32_midi_package_t *package);
extern s32 MIOS32_IIC_MIDI_PackageReceive(u8 iic_port, mios32_midi_package_t *package);



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////



#endif /* _MIOS32_IIC_MIDI_H */
