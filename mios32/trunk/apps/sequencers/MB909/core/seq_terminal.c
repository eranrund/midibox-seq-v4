// $Id: seq_terminal.c 1806 2013-06-16 19:17:37Z tk $
/*
 * MIDIbox SEQ MIDI Terminal
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>

#include <seq_midi_out.h>
#include <ff.h>

#include <aout.h>
#include <app_lcd.h>

#include "tasks.h"

#include "app.h"
#include "seq_terminal.h"


#include "seq_core.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_record.h"
#include "seq_midi_port.h"
#include "seq_midi_router.h"
#include "seq_blm.h"
#include "seq_song.h"
#include "seq_mixer.h"

#include "file.h"
#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_file_s.h"
#include "seq_file_m.h"
#include "seq_file_g.h"
#include "seq_file_c.h"
#include "seq_file_t.h"
#include "seq_file_gc.h"
#include "seq_file_bm.h"
#include "seq_file_hw.h"

#include "seq_ui.h"

#include "seq_statistics.h"

#if !defined(MIOS32_FAMILY_EMULATION)
#include "umm_malloc.h"
#include "uip_terminal.h"
#endif


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 100 // recommended size for file transfers via FILE_BrowserHandler()


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static char line_buffer[STRING_MAX];
static u16 line_ix;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 TERMINAL_ParseFilebrowser(mios32_midi_port_t port, char byte);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TERMINAL_Init(u32 mode)
{
  // install the callback function which is called on incoming characters from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(SEQ_TERMINAL_Parse);

  // install the callback function which is called on incoming characters from MIOS Filebrowser
  MIOS32_MIDI_FilebrowserCommandCallback_Init(TERMINAL_ParseFilebrowser);

  // clear line buffer
  line_buffer[0] = 0;
  line_ix = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  return l; // value is valid
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses for on or off
// returns 0 if 'off', 1 if 'on', -1 if invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_on_off(char *word)
{
  if( strcmp(word, "on") == 0 )
    return 1;

  if( strcmp(word, "off") == 0 )
    return 0;

  return -1;
}


/////////////////////////////////////////////////////////////////////////////
// Parser
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TERMINAL_Parse(mios32_midi_port_t port, char byte)
{
  // temporary change debug port (will be restored at the end of this function)
  mios32_midi_port_t prev_debug_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(port);

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    MUTEX_MIDIOUT_TAKE;
    SEQ_TERMINAL_ParseLine(line_buffer, MIOS32_MIDI_SendDebugMessage);
    MUTEX_MIDIOUT_GIVE;
    line_ix = 0;
    line_buffer[line_ix] = 0;
  } else if( line_ix < (STRING_MAX-1) ) {
    line_buffer[line_ix++] = byte;
    line_buffer[line_ix] = 0;
  }

  // restore debug port
  MIOS32_MIDI_DebugPortSet(prev_debug_port);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for Filebrowser
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_ParseFilebrowser(mios32_midi_port_t port, char byte)
{
  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    MUTEX_MIDIOUT_TAKE;
    MUTEX_SDCARD_TAKE;
    FILE_BrowserHandler(port, line_buffer);
    MUTEX_SDCARD_GIVE;
    MUTEX_MIDIOUT_GIVE;
    line_ix = 0;
    line_buffer[line_ix] = 0;
  } else if( line_ix < (STRING_MAX-1) ) {
    line_buffer[line_ix++] = byte;
    line_buffer[line_ix] = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line - also used by shell.c for telnet
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TERMINAL_ParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

#if !defined(MIOS32_FAMILY_EMULATION)
  if( UIP_TERMINAL_ParseLine(input, _output_function) >= 1 )
    return 0; // command parsed by UIP Terminal
#endif

#if !defined(MIOS32_FAMILY_EMULATION)
  if( AOUT_TerminalParseLine(input, _output_function) >= 1 )
    return 0; // command parsed
#endif

#ifdef MIOS32_LCD_universal
  if( APP_LCD_TerminalParseLine(input, _output_function) >= 1 )
    return 0; // command parsed
#endif

  if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      SEQ_TERMINAL_PrintHelp(out);
    } else if( strcmp(parameter, "system") == 0 ) {
      SEQ_TERMINAL_PrintSystem(out);
    } else if( strcmp(parameter, "memory") == 0 ) {
      SEQ_TERMINAL_PrintMemoryInfo(out);
    } else if( strcmp(parameter, "sdcard") == 0 ) {
      SEQ_TERMINAL_PrintSdCardInfo(out);
    } else if( strcmp(parameter, "sdcard_format") == 0 ) {
      if( !brkt || strcasecmp(brkt, "yes, I'm sure") != 0 ) {
	out("ATTENTION: this command will format your SD Card!!!");
	out("           ALL DATA WILL BE DELETED FOREVER!!!");
	out("           Check the current content with the 'sdcard' command");
	out("           Create a backup on your computer if necessary!");
	out("To start formatting, please enter: sdcard_format yes, I'm sure");
	if( brkt ) {
	  out("('%s' wasn't the right \"password\")", brkt);
	}
      } else {
	MUTEX_SDCARD_TAKE;
	out("Formatting SD Card...");
	FRESULT res;
	if( (res=f_mkfs(0,0,0)) != FR_OK ) {
	  out("Formatting failed with error code: %d!", res);
	} else {
	  out("...with success!");
#ifdef MBSEQV4L    
	  out("Please upload your MBSEQ_HW.V4L file with the MIOS Filebrowser now!");
#else
	  out("Please upload your MBSEQ_HW.V4 file with the MIOS Filebrowser now!");
#endif
	  out("Thereafter enter 'reset' to restart the application.");
	}
	MUTEX_SDCARD_GIVE;
      }
    } else if( strcmp(parameter, "global") == 0 ) {
      SEQ_TERMINAL_PrintGlobalConfig(out);
    } else if( strcmp(parameter, "bookmarks") == 0 ) {
      SEQ_TERMINAL_PrintBookmarks(out);
    } else if( strcmp(parameter, "config") == 0 ) {
      SEQ_TERMINAL_PrintSessionConfig(out);
    } else if( strcmp(parameter, "tracks") == 0 ) {
      SEQ_TERMINAL_PrintTracks(out);
    } else if( strcmp(parameter, "track") == 0 ) {
      char *arg;
      if( (arg = strtok_r(NULL, separators, &brkt)) ) {
	int track = get_dec(arg);
	if( track < 1 || track > SEQ_CORE_NUM_TRACKS ) {
	  out("Wrong track number %d - expected track 1..%d\n", track, SEQ_CORE_NUM_TRACKS);
	} else {
	  SEQ_TERMINAL_PrintTrack(out, track-1);
	}
      } else {
	out("Please specify track, e.g. \"track 1\"\n");
      }
    } else if( strcmp(parameter, "mixer") == 0 ) {
      SEQ_TERMINAL_PrintCurrentMixerMap(out);
    } else if( strcmp(parameter, "song") == 0 ) {
      SEQ_TERMINAL_PrintCurrentSong(out);
    } else if( strcmp(parameter, "grooves") == 0 ) {
      SEQ_TERMINAL_PrintGrooveTemplates(out);
    } else if( strcmp(parameter, "msd") == 0 ) {
      out("Mass Storage Device Mode not supported by this application!\n");
    } else if( strcmp(parameter, "set") == 0 ) {
      if( (parameter = strtok_r(NULL, separators, &brkt)) ) {
	if( strcmp(parameter, "router") == 0 ) {
	  char *arg;
	  if( !(arg = strtok_r(NULL, separators, &brkt)) ) {
	    out("Missing node number!");
	  } else {
	    s32 node = get_dec(arg);
	    if( node < 1 || node > SEQ_MIDI_ROUTER_NUM_NODES ) {
	      out("Expecting node number between 1..%d!", SEQ_MIDI_ROUTER_NUM_NODES);
	    } else {
	      node-=1; // user counts from 1

	      if( !(arg = strtok_r(NULL, separators, &brkt)) ) {
		out("Missing input port!");
	      } else {
		mios32_midi_port_t src_port = 0xff;
		int port_ix;
		for(port_ix=0; port_ix<SEQ_MIDI_PORT_InNumGet(); ++port_ix) {
		  // terminate port name at first space
		  char port_name[10];
		  strcpy(port_name, SEQ_MIDI_PORT_InNameGet(port_ix));
		  int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

		  if( strcmp(arg, port_name) == 0 ) {
		    src_port = SEQ_MIDI_PORT_InPortGet(port_ix);
		    break;
		  }
		}

		if( src_port >= 0xf0 ) {
		  out("Unknown or invalid MIDI input port!");
		} else {

		  char *arg_src_chn;
		  if( !(arg_src_chn = strtok_r(NULL, separators, &brkt)) ) {
		    out("Missing source channel, expecting off, 1..16 or all!");
		  } else {
		    int src_chn = -1;

		    if( strcmp(arg_src_chn, "---") == 0 || strcmp(arg_src_chn, "off") == 0 )
		      src_chn = 0;
		    else if( strcasecmp(arg_src_chn, "all") == 0 )
		      src_chn = 17;
		    else {
		      src_chn = get_dec(arg_src_chn);
		      if( src_chn > 16 )
			src_chn = -1;
		    }

		    if( src_chn < 0 ) {
		      out("Invalid source channel, expecting off, 1..16 or all!");
		    } else {

		      if( !(arg = strtok_r(NULL, separators, &brkt)) ) {
			out("Missing output port!");
		      } else {
			mios32_midi_port_t dst_port = 0xff;
			int port_ix;
			for(port_ix=0; port_ix<SEQ_MIDI_PORT_OutNumGet(); ++port_ix) {
			  // terminate port name at first space
			  char port_name[10];
			  strcpy(port_name, SEQ_MIDI_PORT_OutNameGet(port_ix));
			  int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

			  if( strcmp(arg, port_name) == 0 ) {
			    dst_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
			    break;
			  }
			}

			if( dst_port >= 0xf0 ) {
			  out("Unknown or invalid MIDI output port!");
			} else {

			  char *arg_dst_chn;
			  if( !(arg_dst_chn = strtok_r(NULL, separators, &brkt)) ) {
			    out("Missing destination channel, expecting off, 1..16 or all!");
			  } else {
			    int dst_chn = -1;

			    if( strcmp(arg_dst_chn, "---") == 0 || strcmp(arg_dst_chn, "off") == 0 )
			      dst_chn = 0;
			    else if( strcasecmp(arg_dst_chn, "all") == 0 )
			      dst_chn = 17;
			    else if( strcasecmp(arg_dst_chn, "trk") == 0 || strcasecmp(arg_dst_chn, "track") == 0 )
			      dst_chn = 18;
			    else if( strcasecmp(arg_dst_chn, "stk") == 0 ||
				     strcasecmp(arg_dst_chn, "strk") == 0 ||
				     strcasecmp(arg_dst_chn, "seltrk") == 0 ||
				     strcasecmp(arg_dst_chn, "seltrack") == 0 ||
				     strcasecmp(arg_dst_chn, "track") == 0 )
			      dst_chn = 19;
			    else {
			      dst_chn = get_dec(arg_dst_chn);
			      if( dst_chn > 16 )
				dst_chn = -1;
			    }

			    if( dst_chn < 0 ) {
			      out("Invalid destination channel, expecting off, 1..16 or all!");
			    } else {
			      //
			      // finally...
			      //
			      seq_midi_router_node_t *n = &seq_midi_router_node[node];
			      n->src_port = src_port;
			      n->src_chn = src_chn;
			      n->dst_port = dst_port;
			      n->dst_chn = dst_chn;

			      out("Changed Node %d to SRC:%s %s  DST:%s %s",
				  node+1,
				  SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(n->src_port)),
				  arg_src_chn,
				  SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(n->dst_port)),
				  arg_dst_chn);
			    }
			  }
			}
		      }
		    }
		  }
		}
	      }
	    }
	  }
	} else if( strcmp(parameter, "mclk_in") == 0 || strcmp(parameter, "mclk_out") == 0 ) {
	  int mclk_in = strcmp(parameter, "mclk_in") == 0;

	  char *arg;
	  if( !(arg = strtok_r(NULL, separators, &brkt)) ) {
	    out("Missing MIDI clock port!");
	  } else {
	    mios32_midi_port_t mclk_port = 0xff;
	    int port_ix;
	    for(port_ix=0; port_ix<SEQ_MIDI_PORT_ClkNumGet(); ++port_ix) {
	      // terminate port name at first space
	      char port_name[10];
	      strcpy(port_name, SEQ_MIDI_PORT_ClkNameGet(port_ix));
	      int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

	      if( strcmp(arg, port_name) == 0 ) {
		mclk_port = SEQ_MIDI_PORT_ClkPortGet(port_ix);
		break;
	      }
	    }

	    if( mclk_in && mclk_port >= 0xf0 ) {
	      // extra: allow 'INx' as well
	      if( strncmp(arg, "IN", 2) == 0 && arg[2] >= '1' && arg[2] <= '4' )
		mclk_port = UART0 + (arg[2] - '1');
	    }

	    if( !mclk_in && mclk_port >= 0xf0 ) {
	      // extra: allow 'OUTx' as well
	      if( strncmp(arg, "OUT", 3) == 0 && arg[3] >= '1' && arg[3] <= '4' )
		mclk_port = UART0 + (arg[3] - '1');
	    }

	    if( mclk_port >= 0xf0 ) {
	      out("Unknown or invalid MIDI Clock port!");
	    } else {
	      int on_off = -1;
	      char *arg_on_off;
	      if( !(arg_on_off = strtok_r(NULL, separators, &brkt)) ||
		  (on_off = get_on_off(arg_on_off)) < 0 ) {
		out("Missing 'on' or 'off' after port name!");
	      } else {
		if( mclk_in ) {
		  if( SEQ_MIDI_ROUTER_MIDIClockInSet(mclk_port, on_off) < 0 )
		    out("Failed to set MIDI Clock port %s", arg);
		  else
		    out("Set MIDI Clock for IN port %s to %s\n", arg, arg_on_off);
		} else {
		  if( SEQ_MIDI_ROUTER_MIDIClockOutSet(mclk_port, on_off) < 0 )
		    out("Failed to set MIDI Clock port %s", arg);
		  else
		    out("Set MIDI Clock for OUT port %s to %s\n", arg, arg_on_off);
		}
	      }
	    }
	  }
	} else if( strcmp(parameter, "blm_port") == 0 ) {
	  char *arg;
	  if( !(arg = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specifify BLM MIDI input port or 'off' to disable BLM!");
	  } else {
	    if( strcmp(arg, "off") == 0 ) {
	      seq_blm_port = 0x00;
	      out("BLM port has been disabled!");
	    } else {
	      mios32_midi_port_t blm_port = 0xff;
	      int port_ix;
	      for(port_ix=0; port_ix<SEQ_MIDI_PORT_InNumGet(); ++port_ix) {
		// terminate port name at first space
		char port_name[10];
		strcpy(port_name, SEQ_MIDI_PORT_InNameGet(port_ix));
		int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

		if( strcmp(arg, port_name) == 0 ) {
		  blm_port = SEQ_MIDI_PORT_InPortGet(port_ix);
		  break;
		}
	      }

	      if( blm_port >= 0xf0 ) {
		out("Unknown or invalid BLM input port!");
	      } else {
		seq_blm_port = blm_port;
		out("BLM port set to %s", SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(seq_blm_port)));
	      }
	    }
	  }

	} else if( strcmp(parameter, "rec_quantisation") == 0 ) {
	  char *arg;
	  int value;
	  if( !(arg = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify quantisation between 1%%..100%% (default: 10%%, current: %d%%)\n", seq_record_quantize);
	  } else {
	    int len;
	    while( (len=strlen(arg)) && arg[len-1] == '%' ) {
	      arg[len-1] = 0;
	    }

	    if( (value=get_dec(arg)) < 0 || value > 100 ) {
	      out("Quantisation should be between 1%%..100%%!\n");
	    } else {
	      seq_record_quantize = value;
	      out("Quantisation set to %d%%\n", seq_record_quantize);
	      out("Enter 'store' to save this setting on SD Card.\n");
	    }
	  }
	} else {
	  out("Unknown set parameter: '%s'!", parameter);
	}
      } else {
	out("Missing parameter after 'set'!");
      }
    } else if( strcmp(parameter, "router") == 0 ) {
      SEQ_TERMINAL_PrintRouterInfo(out);
    } else if( strcmp(parameter, "play") == 0 || strcmp(parameter, "start") == 0 ) { // play or start do the same
      SEQ_UI_Button_Play(0);
      out("Sequencer started...\n");
    } else if( strcmp(parameter, "stop") == 0 ) {
      SEQ_UI_Button_Stop(0);
      out("Sequencer stopped...\n");
    } else if( strcmp(parameter, "store") == 0 ) {
      MUTEX_SDCARD_TAKE;
      if( SEQ_FILE_SaveAllFiles() < 0 ) {
	out("Failed to store session on SD Card: /SESSIONS/%s\n", seq_file_session_name);
      } else {
	out("Stored complete session on SD Card: /SESSIONS/%s\n", seq_file_session_name);
      }
      MUTEX_SDCARD_GIVE;
    } else if( strcmp(parameter, "restore") == 0 ) {
      MUTEX_SDCARD_TAKE;
      if( SEQ_FILE_LoadAllFiles(1) < 0 ) {
	out("Failed to restore session from SD Card: /SESSIONS/%s\n", seq_file_session_name);
      } else {
	out("Restored complete session from SD Card: /SESSIONS/%s\n", seq_file_session_name);
      }
      MUTEX_SDCARD_GIVE;
    } else if( strcmp(parameter, "reset") == 0 ) {
      MIOS32_SYS_Reset();
    } else {
      out("Unknown command - type 'help' to list available commands!\n");
    }
  }

  return 0; // no error
}


s32 SEQ_TERMINAL_PrintHelp(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
  out("Following commands are available:");
  out("  system:         print system info\n");
  out("  memory:         print memory allocation info\n");
  out("  sdcard:         print SD Card info\n");
  out("  sdcard_format:  formats the SD Card (you will be asked for confirmation)\n");
  out("  global:         print global configuration\n");
  out("  config:         print local session configuration\n");
  out("  tracks:         print overview of all tracks\n");
  out("  track <track>:  print info about a specific track\n");
  out("  mixer:          print current mixer map\n");
  out("  song:           print current song info\n");
  out("  grooves:        print groove templates\n");
  out("  bookmarks:      print bookmarks\n");
  out("  router:         print MIDI router info\n");
  out("  set router <node> <in-port> <off|channel|all> <out-port> <off|channel|all>: change router setting");
  out("  set mclk_in  <in-port>  <on|off>: change MIDI IN Clock setting");
  out("  set mclk_out <out-port> <on|off>: change MIDI OUT Clock setting");
  out("  set blm_port <off|in-port>: change BLM input port (same port is used for output)");
  out("  set rec_quantisation <1..100>: change record quantisation (default: 10%%, current: %d%%)\n", seq_record_quantize);
#if !defined(MIOS32_FAMILY_EMULATION)
  AOUT_TerminalHelp(_output_function);
#endif
#ifdef MIOS32_LCD_universal
  APP_LCD_TerminalHelp(_output_function);
#endif
#if !defined(MIOS32_FAMILY_EMULATION)
  UIP_TERMINAL_Help(_output_function);
#endif
  out("  play or start:  emulates the PLAY button\n");
  out("  stop:           emulates the STOP button\n");
  out("  store:          stores complete session on SD Card\n");
  out("  restore:        restores complete session from SD Card\n");
  out("  reset:          resets the MIDIbox SEQ (!)\n");
  out("  help:           this page\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

// Help function
static void stringNote(char *label, u8 note)
{
  const char noteTab[12][3] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };

  // print "---" if note number is 0
  if( note == 0 )
    sprintf(label, "---  ");
  else {
    u8 octave = note / 12;
    note %= 12;

    // print semitone and octave (-2): up to 4 chars
    sprintf(label, "%s%d  ",
	    noteTab[note],
	    (int)octave-2);
  }
}



s32 SEQ_TERMINAL_PrintSystem(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char str_buffer[128];

  MUTEX_MIDIOUT_TAKE;

  out("System Informations:\n");
  out("====================\n");
  out(MIOS32_LCD_BOOT_MSG_LINE1 " " MIOS32_LCD_BOOT_MSG_LINE2 "\n");

  mios32_sys_time_t t = MIOS32_SYS_TimeGet();
  int hours = (t.seconds / 3600) % 24;
  int minutes = (t.seconds % 3600) / 60;
  int seconds = (t.seconds % 3600) % 60;

  out("Operating System: MIOS32\n");
  out("Board: " MIOS32_BOARD_STR "\n");
  out("Chip Family: " MIOS32_FAMILY_STR "\n");
  if( MIOS32_SYS_SerialNumberGet((char *)str_buffer) >= 0 )
    out("Serial Number: %s\n", str_buffer);
  else
    out("Serial Number: ?\n");
  out("Flash Memory Size: %d bytes\n", MIOS32_SYS_FlashSizeGet());
  out("RAM Size: %d bytes\n", MIOS32_SYS_RAMSizeGet());

  {
    out("MIDI IN Ports:\n");
    int num = SEQ_MIDI_PORT_InNumGet();
    int i;
    for(i=0; i<num; ++i) {
      mios32_midi_port_t port = SEQ_MIDI_PORT_InPortGet(i);
      out("  - %s (%s)\n", SEQ_MIDI_PORT_InNameGet(i), SEQ_MIDI_PORT_InCheckAvailable(port) ? "available" : "not available");
    }    
  }

  {
    out("MIDI OUT Ports:\n");
    int num = SEQ_MIDI_PORT_OutNumGet();
    int i;
    for(i=0; i<num; ++i) {
      mios32_midi_port_t port = SEQ_MIDI_PORT_OutPortGet(i);
      out("  - %s (%s)\n", SEQ_MIDI_PORT_OutNameGet(i), SEQ_MIDI_PORT_OutCheckAvailable(port) ? "available" : "not available");
    }    
  }

  out("Systime: %02d:%02d:%02d\n", hours, minutes, seconds);
  out("CPU Load: %02d%%\n", SEQ_STATISTICS_CurrentCPULoad());
  out("MIDI Scheduler: Alloc %3d/%3d Drops: %3d",
	    seq_midi_out_allocated, seq_midi_out_max_allocated, seq_midi_out_dropouts);

  u32 stopwatch_value_max = SEQ_STATISTICS_StopwatchGetValueMax();
  u32 stopwatch_value = SEQ_STATISTICS_StopwatchGetValue();
  if( stopwatch_value_max == 0xffffffff ) {
    out("Stopwatch: Overrun!\n");
  } else if( !stopwatch_value_max ) {
    out("Stopwatch: no result yet\n");
  } else {
    out("Stopwatch: %d/%d uS\n", stopwatch_value, stopwatch_value_max);
  }

  u8 scale, root_selection, root;
  SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);
  char root_note_str[20];
  stringNote(root_note_str, root + 0x3c);
  out("Current Root Note (via %s): %s\n",
      root_selection == 0 ? "Keyboard" : "Selection",
      root_note_str);

#if !defined(MIOS32_FAMILY_EMULATION) && (configGENERATE_RUN_TIME_STATS || configUSE_TRACE_FACILITY)
  // send Run Time Stats to MIOS terminal
  out("FreeRTOS Task RunTime Stats:\n");
  FREERTOS_UTILS_RunTimeStats();
#endif

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}


s32 SEQ_TERMINAL_PrintGlobalConfig(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  out("Global Configuration:\n");
  out("=====================\n");
  SEQ_FILE_GC_Debug();

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintBookmarks(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  out("Global Bookmarks:\n");
  out("=================\n");
  SEQ_FILE_BM_Debug(1);

  out("\n");

  out("Session Bookmarks:\n");
  out("==================\n");
  SEQ_FILE_BM_Debug(0);

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintSessionConfig(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  out("Session Configuration:\n");
  out("======================\n");
  SEQ_FILE_C_Debug();

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintTracks(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char str_buffer[128];

  MUTEX_MIDIOUT_TAKE;
  out("Track Overview:\n");
  out("===============\n");

  out("| Track | Mode  | Layer P/T/I | Steps P/T | Length | Port  | Chn. | Muted |\n");
  out("+-------+-------+-------------+-----------+--------+-------+------+-------+\n");

  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    seq_event_mode_t event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
    u16 num_instruments = SEQ_TRG_NumInstrumentsGet(track);
    u16 num_par_layers = SEQ_PAR_NumLayersGet(track);
    u16 num_par_steps = SEQ_PAR_NumStepsGet(track);
    u16 num_trg_layers = SEQ_TRG_NumLayersGet(track);
    u16 num_trg_steps = SEQ_TRG_NumStepsGet(track);
    u16 length = (u16)SEQ_CC_Get(track, SEQ_CC_LENGTH) + 1;
    mios32_midi_port_t midi_port = SEQ_CC_Get(track, SEQ_CC_MIDI_PORT);
    u8 midi_chn = SEQ_CC_Get(track, SEQ_CC_MIDI_CHANNEL) + 1;

    sprintf(str_buffer, "| G%dT%d  | %s |",
	    (track/4)+1, (track%4)+1,
	    SEQ_LAYER_GetEvntModeName(event_mode));

    sprintf((char *)(str_buffer + strlen(str_buffer)), "   %2d/%2d/%2d  |  %3d/%3d  |   %3d  | %s%c |  %2d  |",
	    num_par_layers, num_trg_layers, num_instruments, 
	    num_par_steps, num_trg_steps,
	    length,
	    SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(midi_port)),
	    SEQ_MIDI_PORT_OutCheckAvailable(midi_port) ? ' ' : '*',
	    midi_chn);

    if( seq_core_trk_muted & (1 << track) )
      sprintf((char *)(str_buffer + strlen(str_buffer)), "  yes  |\n");
    else if( seq_core_trk[track].layer_muted )
      sprintf((char *)(str_buffer + strlen(str_buffer)), " layer |\n");
    else
      sprintf((char *)(str_buffer + strlen(str_buffer)), "  no   |\n");

    out(str_buffer);
  }

  out("+-------+-------+-------------+-----------+--------+-------+------+-------+\n");

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintTrack(void *_output_function, u8 track)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;

  out("Track Parameters of G%dT%d", (track/4)+1, (track%4)+1);
  out("========================\n");

  SEQ_FILE_T_Debug(track);

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintCurrentMixerMap(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  char str_buffer[128];
  u8 map = SEQ_MIXER_NumGet();
  int i;

  out("Mixer Map #%3d\n", map+1);
  out("==============\n");

  out("|Num|Port|Chn|Prg|Vol|Pan|Rev|Cho|Mod|CC1|CC2|CC3|CC4|C1A|C2A|C3A|C4A|\n");
  out("+---+----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n");

  for(i=0; i<16; ++i) {
    sprintf(str_buffer, "|%3d|%s|", i, SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(SEQ_MIXER_Get(i, SEQ_MIXER_PAR_PORT))));

    int par;

    for(par=1; par<2; ++par)
      sprintf((char *)(str_buffer + strlen(str_buffer)), "%3d|", SEQ_MIXER_Get(i, par)+1);

    for(par=2; par<12; ++par) {
      u8 value = SEQ_MIXER_Get(i, par);
      if( value )
	sprintf((char *)(str_buffer + strlen(str_buffer)), "%3d|", value-1);
      else
	sprintf((char *)(str_buffer + strlen(str_buffer)), " - |");
    }

    for(par=12; par<16; ++par)
      sprintf((char *)(str_buffer + strlen(str_buffer)), "%3d|", SEQ_MIXER_Get(i, par));

    out("%s\n", str_buffer);
  }

  out("+---+----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n");
  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}


s32 SEQ_TERMINAL_PrintCurrentSong(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  u8 song = SEQ_SONG_NumGet();

  out("Song #%2d\n", song+1);
  out("========\n");

  out("Name: '%s'\n", seq_song_name);
  MIOS32_MIDI_SendDebugHexDump((u8 *)&seq_song_steps[0], SEQ_SONG_NUM_STEPS*sizeof(seq_song_step_t));

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintGrooveTemplates(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;
  out("Groove Templates:\n");
  out("=================\n");
  SEQ_FILE_G_Debug();

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

s32 SEQ_TERMINAL_PrintMemoryInfo(void *_output_function)
{
  //void (*out)(char *format, ...) = _output_function;
  // TODO: umm_info doesn't allow to define output function

#if !defined(MIOS32_FAMILY_EMULATION)
  umm_info( NULL, 1 );
#endif

  return 0; // no error
}


///////////////////////////////////////////////////////////////////
// These time and date functions and other bits of following code were adapted from 
// Rickey's world of Microelectronics under the creative commons 2.5 license.
// http://www.8051projects.net/mmc-sd-interface-fat16/final-code.php
static void ShowFatTime(u32 ThisTime, char* msg)
{
   u8 AM = 1;

   int Hour, Minute, Second;

   Hour = ThisTime >> 11;        // bits 15 through 11 hold Hour...
   Minute = ThisTime & 0x07E0;   // bits 10 through 5 hold Minute... 0000 0111 1110 0000
   Minute = Minute >> 5;
   Second = ThisTime & 0x001F;   //bits 4 through 0 hold Second...   0000 0000 0001 1111
   
   if( Hour > 11 )
   {
      AM = 0;
      if( Hour > 12 )
         Hour -= 12;
   }
     
   sprintf( msg, "%02d:%02d:%02d %s", Hour, Minute, Second*2,
         (AM)?"AM":"PM");
   return;
}

static void ShowFatDate(u32 ThisDate, char* msg)
{

   int Year, Month, Day;

   Year = ThisDate >> 9;         // bits 15 through 9 hold year...
   Month = ThisDate & 0x01E0;    // bits 8 through 5 hold month... 0000 0001 1110 0000
   Month = Month >> 5;
   Day = ThisDate & 0x001F;      //bits 4 through 0 hold day...    0000 0000 0001 1111
   sprintf( msg, "%02d/%02d/%02d", Month, Day, Year-20);
   return;
}


s32 SEQ_TERMINAL_PrintSdCardInfo(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  FRESULT res;
  FILINFO fno;
  DIR dir;
  char *fn;
  char str_buffer[128];

  MUTEX_MIDIOUT_TAKE;

  out("SD Card Informations\n");
  out("====================\n");

#if !defined(MIOS32_FAMILY_EMULATION)
  // this yield ensures, that Debug Messages are sent before we continue the execution
  // Since MIOS Studio displays the time at which the messages arrived, this allows
  // us to measure the delay of following operations
  taskYIELD();

  MUTEX_SDCARD_TAKE;
  FILE_PrintSDCardInfos();
  MUTEX_SDCARD_GIVE;
#endif

  out("\n");
  out("Reading Root Directory\n");
  out("======================\n");

  taskYIELD();

  if( !FILE_SDCardAvailable() ) {
    sprintf(str_buffer, "not connected");
  } else if( !FILE_VolumeAvailable() ) {
    sprintf(str_buffer, "Invalid FAT");
  } else {
    out("Retrieving SD Card informations - please wait!\n");
    MUTEX_MIDIOUT_GIVE;
    MUTEX_SDCARD_TAKE;
    FILE_UpdateFreeBytes();
    MUTEX_SDCARD_GIVE;
    MUTEX_MIDIOUT_TAKE;

    sprintf(str_buffer, "'%s': %u of %u MB free", 
	    FILE_VolumeLabel(),
	    (unsigned int)(FILE_VolumeBytesFree()/1000000),
	    (unsigned int)(FILE_VolumeBytesTotal()/1000000));
  }
  out("SD Card: %s\n", str_buffer);

  taskYIELD();

#if _USE_LFN
  static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
  fno.lfname = lfn;
  fno.lfsize = sizeof(lfn);
#endif

  MUTEX_SDCARD_TAKE;
  if( (res=f_opendir(&dir, "/")) != FR_OK ) {
    out("Failed to open root directory - error status: %d\n", res);
  } else {
    while( (f_readdir(&dir, &fno) == FR_OK) && fno.fname[0] ) {
#if _USE_LFN
      fn = *fno.lfname ? fno.lfname : fno.fname;
#else
      fn = fno.fname;
#endif
      char date[10];
      ShowFatDate(fno.fdate,(char*)&date);
      char time[12];
      ShowFatTime(fno.ftime,(char*)&time);
      out("[%s%s%s%s%s%s%s] %s  %s   %s %u %s\n",
		(fno.fattrib & AM_RDO ) ? "r" : ".",
		(fno.fattrib & AM_HID ) ? "h" : ".",
		(fno.fattrib & AM_SYS ) ? "s" : ".",
		(fno.fattrib & AM_VOL ) ? "v" : ".",
		(fno.fattrib & AM_LFN ) ? "l" : ".",
		(fno.fattrib & AM_DIR ) ? "d" : ".",
		(fno.fattrib & AM_ARC ) ? "a" : ".",
		date,time,
		(fno.fattrib & AM_DIR) ? "<DIR>" : " ",
		fno.fsize,fn);
    }
  }
  MUTEX_SDCARD_GIVE;

  taskYIELD();

  out("\n");
  out("Checking SD Card at application layer\n");
  out("=====================================\n");

  out("Current session: /SESSIONS/%s\n", seq_file_session_name);

  {
    u8 bank;
    for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank) {
      int num_patterns = SEQ_FILE_B_NumPatterns(bank);
      if( num_patterns )
	out("File /SESSIONS/%s/MBSEQ_B%d.V4: valid (%d patterns)\n", seq_file_session_name, bank+1, num_patterns);
      else
	out("File /SESSIONS/%s/MBSEQ_B%d.V4: doesn't exist\n", seq_file_session_name, bank+1, num_patterns);
    }

    int num_maps = SEQ_FILE_M_NumMaps();
    if( num_maps )
      out("File /SESSIONS/%s/MBSEQ_M.V4: valid (%d mixer maps)\n", seq_file_session_name, num_maps);
    else
      out("File /SESSIONS/%s/MBSEQ_M.V4: doesn't exist\n", seq_file_session_name);
    
    int num_songs = SEQ_FILE_S_NumSongs();
    if( num_songs )
      out("File /SESSIONS/%s/MBSEQ_S.V4: valid (%d songs)\n", seq_file_session_name, num_songs);
    else
      out("File /SESSIONS/%s/MBSEQ_S.V4: doesn't exist\n", seq_file_session_name);

    if( SEQ_FILE_G_Valid() )
      out("File /SESSIONS/%s/MBSEQ_G.V4: valid\n", seq_file_session_name);
    else
      out("File /SESSIONS/%s/MBSEQ_G.V4: doesn't exist\n", seq_file_session_name);
    
    if( SEQ_FILE_BM_Valid(0) )
      out("File /SESSIONS/%s/MBSEQ_BM.V4: valid\n", seq_file_session_name);
    else
      out("File /SESSIONS/%s/MBSEQ_BM.V4: doesn't exist\n", seq_file_session_name);
    
    if( SEQ_FILE_C_Valid() )
      out("File /SESSIONS/%s/MBSEQ_C.V4: valid\n", seq_file_session_name);
    else
      out("File /SESSIONS/%s/MBSEQ_C.V4: doesn't exist\n", seq_file_session_name);

    if( SEQ_FILE_GC_Valid() )
      out("File /MBSEQ_C.V4: valid\n");
    else
      out("File /MBSEQ_C.V4: doesn't exist\n");

    if( SEQ_FILE_BM_Valid(1) )
      out("File /MBSEQ_BM.V4: valid\n");
    else
      out("File /MBSEQ_BM.V4: doesn't exist\n");

#ifndef MBSEQV4L    
    if( SEQ_FILE_HW_Valid() )
      out("File /MBSEQ_HW.V4: valid\n");
    else
      out("File /MBSEQ_HW.V4: doesn't exist or hasn't been re-loaded\n");
#else
    if( SEQ_FILE_HW_Valid() )
      out("File /MBSEQ_HW.V4L: valid\n");
    else
      out("File /MBSEQ_HW.V4L: doesn't exist or hasn't been re-loaded\n");
#endif
  }

  out("done.\n");
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}


s32 SEQ_TERMINAL_PrintRouterInfo(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  MUTEX_MIDIOUT_TAKE;

  out("MIDI Router Nodes (change with 'set router <node> <in-port> <channel> <out-port> <channel>)");
  out("Example: set router 1 IN1 all USB1 all");

  u8 node;
  seq_midi_router_node_t *n = &seq_midi_router_node[0];
  for(node=0; node<SEQ_MIDI_ROUTER_NUM_NODES; ++node, ++n) {

    char src_chn[10];
    if( !n->src_chn )
      sprintf(src_chn, "off");
    else if( n->src_chn > 16 )
      sprintf(src_chn, "all");
    else
      sprintf(src_chn, "#%2d", n->src_chn);

    char dst_chn[10];
    if( !n->dst_chn )
      sprintf(dst_chn, "off");
    else if( n->dst_chn == 17 )
      sprintf(dst_chn, "All");
    else if( n->dst_chn == 18 )
      sprintf(dst_chn, "Trk");
    else if( n->dst_chn >= 19 )
      sprintf(dst_chn, "STk");
    else
      sprintf(dst_chn, "#%2d", n->dst_chn);

    out("  %2d  SRC:%s %s  DST:%s %s",
	node+1,
	SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(n->src_port)),
	src_chn,
	SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(n->dst_port)),
	dst_chn);
  }

  out("");
  out("MIDI Clock (change with 'set mclk_in <in-port> <on|off>' resp. 'set mclk_out <out-port> <on|off>')");

  int num_mclk_ports = SEQ_MIDI_PORT_ClkNumGet();
  int port_ix;
  for(port_ix=0; port_ix<num_mclk_ports; ++port_ix) {
    mios32_midi_port_t mclk_port = SEQ_MIDI_PORT_ClkPortGet(port_ix);

    s32 enab_rx = SEQ_MIDI_ROUTER_MIDIClockInGet(mclk_port);
    if( !SEQ_MIDI_PORT_ClkCheckAvailable(mclk_port) )
      enab_rx = -1; // MIDI In port not available

    s32 enab_tx = SEQ_MIDI_ROUTER_MIDIClockOutGet(mclk_port);
    if( !SEQ_MIDI_PORT_ClkCheckAvailable(mclk_port) )
      enab_tx = -1; // MIDI In port not available

    out("  %s  IN:%s  OUT:%s\n",
	SEQ_MIDI_PORT_ClkNameGet(port_ix),
	(enab_rx == 0) ? "off" : ((enab_rx == 1) ? "on " : "---"),
	(enab_tx == 0) ? "off" : ((enab_tx == 1) ? "on " : "---"));
  }

  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}
