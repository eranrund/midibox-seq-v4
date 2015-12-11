// $Id: shell.c 1285 2011-08-02 22:34:10Z tk $
/*
 * Telnet shell for standard UIP task
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <mios32.h>
#include <string.h>
#include <stdarg.h>

// shell.h is located in uip_task_standard directory and has to be implemented locally in application
#include <shell.h>

#include "terminal.h"

#define SHELL_PROMPT "> "

/*---------------------------------------------------------------------------*/
// TK: to avoid changes in telnetd.c
static void my_shell_output(char *format, ...)
{
  va_list args;
  char line[128]; // should be sufficient - recommented value is 80

  va_start(args, format);
  vsprintf(line, format, args);
  shell_output(line, "");
}

/*---------------------------------------------------------------------------*/
void
shell_init(void)
{
}
/*---------------------------------------------------------------------------*/
void
shell_start(void)
{
  TERMINAL_ParseLine("help", my_shell_output);
  shell_prompt(SHELL_PROMPT);
}
/*---------------------------------------------------------------------------*/
void
shell_input(char *cmd)
{
  if( strcmp(cmd, "exit") == 0 ) {
    shell_quit("");
  } else {
    TERMINAL_ParseLine(cmd, my_shell_output);
    shell_prompt(SHELL_PROMPT);
  }
}
/*---------------------------------------------------------------------------*/
