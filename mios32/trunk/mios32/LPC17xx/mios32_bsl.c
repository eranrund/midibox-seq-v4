// $Id: mios32_bsl.c 1185 2011-04-22 18:32:15Z tk $
//
// Includes the bootloader code into the project
// If no bootloader is provided for the board, reserve the BSL range instead
//
/* ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// This switch is only for expert usage - it frees 16k, but can lead to
// big trouble if a user downloads the binary with the MIOS32 BSL, and
// doesn't have access to a JTAG interface or COM port + MBHP_LTC module
// to recover the BSL
#ifndef MIOS32_DONT_INCLUDE_BSL
#if defined(MIOS32_PROCESSOR_LPC1769)
#include "mios32_bsl_LPCXPRESSO_1769.inc"
#elif defined(MIOS32_PROCESSOR_LPC1768)
#include "mios32_bsl_LPCXPRESSO_1768.inc"
#else
#warning "This MIOS32_PROCESSOR isn't prepared in mios32_bsl.c - selecting bootloader of LPC1768"
#include "mios32_bsl_LPCXPRESSO_1768.inc"
#endif
#endif /* MIOS32_DONT_INCLUDE_BSL */
