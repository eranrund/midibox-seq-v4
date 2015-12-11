// $Id: mios32_bsl.c 227 2008-12-31 18:37:52Z tk $
//
// Includes the bootloader code into the project
// If no bootloader is provided for the board, reserve the BSL range instead
//
/* ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
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

//#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
//// full supported BSL
//# include "mios32_bsl_MBHP_CORE_STM32.inc"
//#elif defined(MIOS32_BOARD_STM32_PRIMER)
//// BSL not really required, therefore this is a special "dummy" which
//// directly branches to the application.
//// The LED flickers fast if reset vector of application is invalid
//# include "mios32_bsl_STM32_PRIMER.inc"
//#else
//// same like STM32_PRIMER, but LED assigned to same pin as for MBHP_CORE_STM32 module
//# include "mios32_bsl_dummy.inc"
//#endif

#endif /* MIOS32_DONT_INCLUDE_BSL */
