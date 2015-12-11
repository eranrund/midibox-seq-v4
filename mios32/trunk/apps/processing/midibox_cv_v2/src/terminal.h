// $Id: terminal.h 1903 2013-12-31 00:34:54Z tk $
/*
 * The command/configuration Terminal
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _TERMINAL_H
#define _TERMINAL_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 TERMINAL_Init(u32 mode);
extern s32 TERMINAL_Parse(mios32_midi_port_t port, char byte);
extern s32 TERMINAL_ParseLine(char *input, void *_output_function);
extern s32 TERMINAL_PrintSystem(void *_output_function);
extern s32 TERMINAL_PrintMemoryInfo(void *_output_function);
extern s32 TERMINAL_PrintSdCardInfo(void *_output_function);
extern s32 TERMINAL_ShowNrpns(void *_output_function);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _TERMINAL_H */
