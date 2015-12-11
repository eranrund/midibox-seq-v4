// $Id: terminal.c 1285 2011-08-02 22:34:10Z tk $
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

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>

#include "app.h"
#include "presets.h"
#include "terminal.h"
#include "uip.h"
#include "uip_task.h"
#include "osc_server.h"

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 80


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static char line_buffer[STRING_MAX];
static u16 line_ix;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 TERMINAL_PrintSystem(void *_output_function);
static s32 TERMINAL_PrintIPs(void *_output_function);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_Init(u32 mode)
{
  // install the callback function which is called on incoming characters
  // from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(TERMINAL_Parse);

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
// help function which parses an IP value
// returns > 0 if value is valid
// returns 0 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static u32 get_ip(char *word)
{
  char *brkt;
  u8 ip[4];

  int i;
  for(i=0; i<4; ++i) {
    if( (word = strtok_r((i == 0) ? word : NULL, ".", &brkt)) ) {
      s32 value = get_dec(word);
      if( value >= 0 && value <= 255 )
	ip[i] = value;
      else
	return 0;
    }
  }

  if( i == 4 )
    return (ip[0]<<24)|(ip[1]<<16)|(ip[2]<<8)|(ip[3]<<0);
  else
    return 0; // invalid IP
}


/////////////////////////////////////////////////////////////////////////////
// Parser
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_Parse(mios32_midi_port_t port, u8 byte)
{
  // temporary change debug port (will be restored at the end of this function)
  mios32_midi_port_t prev_debug_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(port);

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    //MUTEX_MIDIOUT_TAKE;
    TERMINAL_ParseLine(line_buffer, MIOS32_MIDI_SendDebugMessage);
    //MUTEX_MIDIOUT_GIVE;
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
// Parser for a complete line - also used by shell.c for telnet
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_ParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
      out("Following commands are available:");
      out("  system:                           print system info");
      out("  set dhcp <on|off>:                enables/disables DHCP");
      out("  set ip <address>:                 changes IP address");
      out("  set netmask <mask>:               changes netmask");
      out("  set gateway <address>:            changes gateway address");
      out("  set osc_remote <con> <address>:   changes OSC Remote Address");
      out("  set osc_remote_port <con> <port>: changes OSC Remote Port (1024..65535)");
      out("  set osc_local_port <con> <port>:  changes OSC Local Port (1024..65535)");
      out("  set udpmon <0..4>:                enables UDP monitor (verbose level: %d)\n", UIP_TASK_UDP_MonitorLevelGet());
      out("  store:                            stores current config as preset");
      out("  restore:                          restores config from preset");
      out("  reset:                            resets the MIDIbox (!)\n");
      out("  help:                             this page");
      out("  exit:                             (telnet only) exits the terminal");
    } else if( strcmp(parameter, "system") == 0 ) {
      TERMINAL_PrintSystem(_output_function);
    } else if( strcmp(parameter, "store") == 0 ) {
      s32 status = PRESETS_StoreAll();
      if( status >= 0 ) {
	out("Presets stored in internal EEPROM!");
      } else {
	out("ERROR: failed to store presets in internal EEPROM (status %d)!", status);
      }
    } else if( strcmp(parameter, "restore") == 0 ) {
      s32 status = PRESETS_Init(0);
      if( status >= 0 ) {
	out("Presets restored from internal EEPROM!");
      } else {
	out("ERROR: failed to restore presets from internal EEPROM (status %d)!", status);
      }
    } else if( strcmp(parameter, "reset") == 0 ) {
      MIOS32_SYS_Reset();
    } else if( strcmp(parameter, "set") == 0 ) {
      if( (parameter = strtok_r(NULL, separators, &brkt)) ) {
	if( strcmp(parameter, "dhcp") == 0 ) {
	  s32 on_off = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off'!");
	  } else {
	    UIP_TASK_DHCP_EnableSet(on_off);
	    if( UIP_TASK_DHCP_EnableGet() ) {
	      out("DHCP enabled - waiting for IP address from server!\n");
	    } else {
	      out("DHCP disabled - using predefined values:");
	      TERMINAL_PrintIPs(_output_function);
	    }
	  }
	} else if( strcmp(parameter, "ip") == 0 ) {
	  if( UIP_TASK_DHCP_EnableGet() ) {
	    out("ERROR: DHCP enabled - please disable it first via 'set dhcp off'");
	  } else {
	    u32 ip = 0;
	    if( (parameter = strtok_r(NULL, separators, &brkt)) )
	      ip = get_ip(parameter);
	    if( !ip ) {
	      out("Expecting IP address in format a.b.c.d!");
	    } else {
	      uip_ipaddr_t ipaddr;
	      UIP_TASK_IP_AddressSet(ip);
	      uip_gethostaddr(&ipaddr);
	      out("Set IP address to %d.%d.%d.%d",
		  uip_ipaddr1(ipaddr), uip_ipaddr2(ipaddr),
		  uip_ipaddr3(ipaddr), uip_ipaddr4(ipaddr));
	    }
	  }
	} else if( strcmp(parameter, "netmask") == 0 ) {
	  if( UIP_TASK_DHCP_EnableGet() ) {
	    out("ERROR: DHCP enabled - please disable it first via 'set dhcp off'");
	  } else {
	    u32 ip = 0;
	    if( (parameter = strtok_r(NULL, separators, &brkt)) )
	      ip = get_ip(parameter);
	    if( !ip ) {
	      out("Expecting netmask in format a.b.c.d!");
	    } else {
	      uip_ipaddr_t ipaddr;
	      UIP_TASK_NetmaskSet(ip);
	      uip_getnetmask(&ipaddr);
	      out("Set netmask to %d.%d.%d.%d",
		  uip_ipaddr1(ipaddr), uip_ipaddr2(ipaddr),
		  uip_ipaddr3(ipaddr), uip_ipaddr4(ipaddr));
	    }
	  }
	} else if( strcmp(parameter, "gateway") == 0 ) {
	  if( UIP_TASK_DHCP_EnableGet() ) {
	    out("ERROR: DHCP enabled - please disable it first via 'set dhcp off'");
	  } else {
	    u32 ip = 0;
	    if( (parameter = strtok_r(NULL, separators, &brkt)) )
	      ip = get_ip(parameter);
	    if( !ip ) {
	      out("Expecting gateway address in format a.b.c.d!");
	    } else {
	      uip_ipaddr_t ipaddr;
	      UIP_TASK_GatewaySet(ip);
	      uip_getdraddr(&ipaddr);
	      out("Set gateway to %d.%d.%d.%d",
		  uip_ipaddr1(ipaddr), uip_ipaddr2(ipaddr),
		  uip_ipaddr3(ipaddr), uip_ipaddr4(ipaddr));
	    }
	  }
	} else if( strcmp(parameter, "osc_remote") == 0 ) {
	  if( !UIP_TASK_ServicesRunning() ) {
	    out("ERROR: Ethernet services not running yet!");
	  } else {
	    s32 con = -1;
	    if( (parameter = strtok_r(NULL, separators, &brkt)) )
	      con = get_dec(parameter);
	    if( con < 1 || con >= OSC_SERVER_NUM_CONNECTIONS) {
	      out("Invalid OSC connection specified as first parameter (expecting 1..%d)!", OSC_SERVER_NUM_CONNECTIONS);
	    } else {
	      con-=1; // the user counts from 1

	      u32 ip = 0;
	      if( (parameter = strtok_r(NULL, separators, &brkt)) )
		ip = get_ip(parameter);
	      if( !ip ) {
		out("Expecting OSC connection <1..%d> and remote address in format a.b.c.d!", OSC_SERVER_NUM_CONNECTIONS);
	      } else {
		if( OSC_SERVER_RemoteIP_Set(con, ip) >= 0 ) {
		  out("Set OSC%d Remote address to %d.%d.%d.%d",
		      con+1,
		      (ip>>24)&0xff, (ip>>16)&0xff, (ip>>8)&0xff, (ip>>0)&0xff);
		  OSC_SERVER_Init(0);
		} else
		  out("ERROR: failed to set OSC%d Remote address!", con+1);
	      }
	    }
	  }
	} else if( strcmp(parameter, "osc_remote_port") == 0 ) {
	  if( !UIP_TASK_ServicesRunning() ) {
	    out("ERROR: Ethernet services not running yet!");
	  } else {
	    s32 con = -1;
	    if( (parameter = strtok_r(NULL, separators, &brkt)) )
	      con = get_dec(parameter);
	    if( con < 1 || con >= OSC_SERVER_NUM_CONNECTIONS) {
	      out("Invalid OSC connection specified as first parameter (expecting 1..%d)!", OSC_SERVER_NUM_CONNECTIONS);
	    } else {
	      con-=1; // the user counts from 1

	      s32 value = -1;
	      if( (parameter = strtok_r(NULL, separators, &brkt)) )
		value = get_dec(parameter);
	      if( value < 1024 || value >= 65535) {
		out("Expecting OSC connection (1..%d) and remote port value in range 1024..65535", OSC_SERVER_NUM_CONNECTIONS);
	      } else {
		if( OSC_SERVER_RemotePortSet(con, value) >= 0 ) {
		  out("Set OSC%d Remote port to %d", con+1, value);
		  OSC_SERVER_Init(0);
		} else
		  out("ERROR: failed to set OSC%d remote port!", con+1);
	      }
	    }
	  }
	} else if( strcmp(parameter, "osc_local_port") == 0 ) {
	  if( !UIP_TASK_ServicesRunning() ) {
	    out("ERROR: Ethernet services not running yet!");
	  } else {
	    s32 con = -1;
	    if( (parameter = strtok_r(NULL, separators, &brkt)) )
	      con = get_dec(parameter);
	    if( con < 1 || con >= OSC_SERVER_NUM_CONNECTIONS) {
	      out("Invalid OSC connection specified as first parameter (expecting 1..%d)!", OSC_SERVER_NUM_CONNECTIONS);
	    } else {
	      con-=1; // the user counts from 1

	      s32 value = -1;
	      if( (parameter = strtok_r(NULL, separators, &brkt)) )
		value = get_dec(parameter);
	      if( value < 1024 || value >= 65535) {
		out("Expecting OSC connection (1..%d) and local port value in range 1024..65535", OSC_SERVER_NUM_CONNECTIONS);
	      } else {
		if( OSC_SERVER_LocalPortSet(con, value) >= 0 ) {
		  out("Set OSC%d Local port to %d", con-1, value);
		  OSC_SERVER_Init(0);
		} else
		  out("ERROR: failed to set OSC%d local port!", con-1);
	      }
	    }
	  }
	} else if( strcmp(parameter, "udpmon") == 0 ) {
	  char *arg;
	  if( (arg = strtok_r(NULL, separators, &brkt)) ) {
	    int level = get_dec(arg);
	    switch( level ) {
	    case UDP_MONITOR_LEVEL_0_OFF:
	      out("Set UDP monitor level to %d (off)\n", level);
	      break;
	    case UDP_MONITOR_LEVEL_1_OSC_REC:
	      out("Set UDP monitor level to %d (received packets assigned to a OSC1..4 port)\n", level);
	      break;
	    case UDP_MONITOR_LEVEL_2_OSC_REC_AND_SEND:
	      out("Set UDP monitor level to %d (received and sent packets assigned to a OSC1..4 port)\n", level);
	      break;
	    case UDP_MONITOR_LEVEL_3_ALL_GEQ_1024:
	      out("Set UDP monitor level to %d (all received and sent packets with port number >= 1024)\n", level);
	      break;
	    case UDP_MONITOR_LEVEL_4_ALL:
	      out("Set UDP monitor level to %d (all received and sent packets)\n", level);
	      break;
	    default:
	      out("Invalid level %d - please specify monitor level 0..4\n", level);
	      level = -1; // invalidate level for next if() check
	    }
	    
	    if( level >= 0 )
	      UIP_TASK_UDP_MonitorLevelSet(level);
	  } else {
	    out("Please specify monitor level (0..4)\n");
	  }
	} else {
	  out("Unknown set parameter: '%s'!", parameter);
	}
      } else {
	out("Missing parameter after 'set'!");
      }
    } else {
      out("Unknown command - type 'help' to list available commands!");
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// System Informations
/////////////////////////////////////////////////////////////////////////////
static s32 TERMINAL_PrintSystem(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("Application: " MIOS32_LCD_BOOT_MSG_LINE1);

  out("Ethernet module connected: %s", UIP_TASK_NetworkDeviceAvailable() ? "yes" : "no");
  out("Ethernet services running: %s", UIP_TASK_ServicesRunning() ? "yes" : "no");
  out("DHCP: %s", UIP_TASK_DHCP_EnableGet() ? "enabled" : "disabled");

  if( UIP_TASK_DHCP_EnableGet() && !UIP_TASK_ServicesRunning() ) {
    out("IP address: not available yet");
    out("Netmask: not available yet");
    out("Default Router (Gateway): not available yet");
  } else {
    TERMINAL_PrintIPs(_output_function);
  }

  int con;
  for(con=0; con<OSC_SERVER_NUM_CONNECTIONS; ++con) {
    u32 osc_remote_ip = OSC_SERVER_RemoteIP_Get(con);
    out("OSC%d Remote address: %d.%d.%d.%d",
	con+1,
	(osc_remote_ip>>24)&0xff, (osc_remote_ip>>16)&0xff,
	(osc_remote_ip>>8)&0xff, (osc_remote_ip>>0)&0xff);
    out("OSC%d Remote port: %d", con+1, OSC_SERVER_RemotePortGet(con));
    out("OSC%d Local port: %d", con+1, OSC_SERVER_LocalPortGet(con));
  }

  out("UDP Monitor: verbose level #%d\n", UIP_TASK_UDP_MonitorLevelGet());

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Print IP settings (used by multiple functions)
/////////////////////////////////////////////////////////////////////////////
static s32 TERMINAL_PrintIPs(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  uip_ipaddr_t ipaddr;
  uip_gethostaddr(&ipaddr);
  out("IP address: %d.%d.%d.%d",
      uip_ipaddr1(ipaddr), uip_ipaddr2(ipaddr),
      uip_ipaddr3(ipaddr), uip_ipaddr4(ipaddr));

  uip_ipaddr_t netmask;
  uip_getnetmask(&netmask);
  out("Netmask: %d.%d.%d.%d",
      uip_ipaddr1(netmask), uip_ipaddr2(netmask),
      uip_ipaddr3(netmask), uip_ipaddr4(netmask));

  uip_ipaddr_t draddr;
  uip_getdraddr(&draddr);
  out("Default Router (Gateway): %d.%d.%d.%d",
      uip_ipaddr1(draddr), uip_ipaddr2(draddr),
      uip_ipaddr3(draddr), uip_ipaddr4(draddr));

  return 0; // no error
}
