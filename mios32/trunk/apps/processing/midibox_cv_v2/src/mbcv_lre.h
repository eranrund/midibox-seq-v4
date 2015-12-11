// $Id: mbcv_lre.h 1912 2014-01-03 23:15:07Z tk $
/*
 * Header file for MIDIbox CV V2 LEDRing/Encoder functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2013 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBCV_LRE_H
#define _MBCV_LRE_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of LRE modules
#define MBCV_LRE_NUM 2

// maximum number of encoders (and LED rings)
#define MBCV_LRE_NUM_ENC (MBCV_LRE_NUM*16)

// maximum number of banks (should match with number of CV channels...)
#define MBCV_LRE_NUM_BANKS 8

// maximum number of LEDring patterns
#define MBCV_LRE_NUM_DOUT_PATTERNS 2

// maximum number of LEDring positions
#define MBCV_LRE_NUM_DOUT_PATTERN_POS 17

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u16 min;
  u16 max;
  u16 nrpn;
  u8  pattern;
} mbcv_lre_enc_cfg_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBCV_LRE_Init(u32 mode);

extern s32 MBCV_LRE_NotifyChange(u32 enc, s32 incrementer);

extern s32 MBCV_LRE_AutoSpeed(u32 enc, u32 range);

extern s32 MBCV_LRE_UpdateAllLedRings(void);

extern s32 MBCV_LRE_FastModeSet(u8 multiplier);
extern s32 MBCV_LRE_FastModeGet(void);

extern s32 MBCV_LRE_ConfigModeSet(u8 configMode);
extern s32 MBCV_LRE_ConfigModeGet(void);

extern s32 MBCV_LRE_BankSet(u8 bank);
extern s32 MBCV_LRE_BankGet(void);

extern s32 MBCV_LRE_PatternSet(u8 num, u8 pos, u16 pattern);
extern u16 MBCV_LRE_PatternGet(u8 num, u8 pos);

extern s32 MBCV_LRE_EncCfgSet(u32 enc, u32 bank, mbcv_lre_enc_cfg_t cfg);
extern s32 MBCV_LRE_EncCfgSetFromDefault(u32 enc, u32 bank, u16 nrpnNumber);
extern mbcv_lre_enc_cfg_t MBCV_LRE_EncCfgGet(u32 enc, u32 bank);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBCV_LRE_H */
