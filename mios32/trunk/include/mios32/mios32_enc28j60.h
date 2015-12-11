// $Id: mios32_enc28j60.h 1117 2010-10-24 12:43:57Z tk $
/*
 * Header file for ENC28J60 Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_ENC28J60_H
#define _MIOS32_ENC28J60_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Which SPI peripheral should be used
// allowed values: 0 and 1
// (note: SPI0 will allocate DMA channel 2 and 3, SPI1 will allocate DMA channel 4 and 5)
#ifndef MIOS32_ENC28J60_SPI
#define MIOS32_ENC28J60_SPI 0
#endif

// Which RC pin of the SPI port should be used
// allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC)
#ifndef MIOS32_ENC28J60_SPI_RC_PIN
#define MIOS32_ENC28J60_SPI_RC_PIN 1
#endif

// maximum frame size
// must be <= 1518 (IEEE 802.3 specified limit)
#ifndef MIOS32_ENC28J60_MAX_FRAME_SIZE
#define MIOS32_ENC28J60_MAX_FRAME_SIZE 1518
#endif

// if 0: half duplex operation
// if 1: full duplex operation
#ifndef MIOS32_ENC28J60_FULL_DUPLEX
#define MIOS32_ENC28J60_FULL_DUPLEX 1
#endif

// a unique MAC address in your network (6 bytes are required)
// If all bytes are 0, the serial number of STM32 will be taken instead,
// which should be unique in your private network.
#ifndef MIOS32_ENC28J60_MY_MAC_ADDR1
#define MIOS32_ENC28J60_MY_MAC_ADDR1 0
#define MIOS32_ENC28J60_MY_MAC_ADDR2 0
#define MIOS32_ENC28J60_MY_MAC_ADDR3 0
#define MIOS32_ENC28J60_MY_MAC_ADDR4 0
#define MIOS32_ENC28J60_MY_MAC_ADDR5 0
#define MIOS32_ENC28J60_MY_MAC_ADDR6 0
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_ENC28J60_Init(u32 mode);

extern s32 MIOS32_ENC28J60_PowerOn(void);
extern s32 MIOS32_ENC28J60_PowerOff(void);
extern s32 MIOS32_ENC28J60_CheckAvailable(u8 was_available);
extern s32 MIOS32_ENC28J60_LinkAvailable(void);
extern s32 MIOS32_ENC28J60_RevIDGet(void);

extern s32 MIOS32_ENC28J60_MAC_AddrSet(u8 new_mac_addr[6]);
extern u8 *MIOS32_ENC28J60_MAC_AddrGet(void);

extern s32 MIOS32_ENC28J60_PackageSend(u8 *buffer, u16 len, u8 *buffer2, u16 len2);
extern s32 MIOS32_ENC28J60_PackageReceive(u8 *buffer, u16 buffer_size);
extern s32 MIOS32_ENC28J60_MACDiscardRx(void);

extern s32 MIOS32_ENC28J60_ReadETHReg(u8 address);
extern s32 MIOS32_ENC28J60_ReadMACReg(u8 address);
extern s32 MIOS32_ENC28J60_ReadPHYReg(u8 reg);

extern s32 MIOS32_ENC28J60_WriteReg(u8 address, u8 data);
extern s32 MIOS32_ENC28J60_BFCReg(u8 address, u8 data);
extern s32 MIOS32_ENC28J60_BFSReg(u8 address, u8 data);
extern s32 MIOS32_ENC28J60_WritePHYReg(u8 reg, u16 data);
extern s32 MIOS32_ENC28J60_BankSel(u16 register);

extern s32 MIOS32_ENC28J60_SendSystemReset(void);

extern s32 MIOS32_ENC28J60_MACGet(void);
extern s32 MIOS32_ENC28J60_MACGetArray(u8 *buffer, u16 len);
extern s32 MIOS32_ENC28J60_MACPut(u8 value);
extern s32 MIOS32_ENC28J60_MACPutArray(u8 *buffer, u16 len);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_ENC28J60_H */
