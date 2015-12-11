// $Id: presets.c 2236 2015-11-08 17:05:52Z tk $
/*
 * Preset handling
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <eeprom.h>
#include <keyboard.h>
#include <midi_router.h>

#include "presets.h"
#include "midimon.h"
#include "uip_task.h"
#include "osc_server.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIOS32_MIDI_SendDebug*
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 1

#ifndef DEBUG_MSG
# define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif


/////////////////////////////////////////////////////////////////////////////
// Reads the EEPROM content during boot
// If EEPROM content isn't valid (magic number mismatch), clear EEPROM
// with default data
/////////////////////////////////////////////////////////////////////////////
s32 PRESETS_Init(u32 mode)
{
  s32 status = 0;

  // init EEPROM emulation
  if( (status=EEPROM_Init(0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[PRESETS] EEPROM initialisation failed with status %d!\n", status);
#endif
  }

  // check for magic number in EEPROM - if not available, initialize the structure
  u32 magic = PRESETS_Read32(PRESETS_ADDR_MAGIC01);
  if( status < 0 ||
      (magic != EEPROM_MAGIC_NUMBER &&
       magic != EEPROM_MAGIC_NUMBER_OLDFORMAT1 &&
       magic != EEPROM_MAGIC_NUMBER_OLDFORMAT2 &&
       magic != EEPROM_MAGIC_NUMBER_OLDFORMAT3 &&
       magic != EEPROM_MAGIC_NUMBER_OLDFORMAT4) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[PRESETS] magic number not found (was 0x%08x) - initialize EEPROM!\n", magic);
#endif

    // clear EEPROM
    if( (status=EEPROM_Init(1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[PRESETS] EEPROM clear failed with status %d!\n", status);
#endif
      return status;
    }

    // write all-0
    int addr;
    for(addr=0; addr<PRESETS_EEPROM_SIZE; ++addr)
      status |= PRESETS_Write16(addr, 0x00);

    status |= PRESETS_StoreAll();
  }

  if( magic == EEPROM_MAGIC_NUMBER_OLDFORMAT1 ||
      magic == EEPROM_MAGIC_NUMBER_OLDFORMAT2 ||
      magic == EEPROM_MAGIC_NUMBER_OLDFORMAT3 ||
      magic == EEPROM_MAGIC_NUMBER_OLDFORMAT4 ) {

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[PRESETS] new format detected: clearing upper part of EEPROM...\n");
#endif

    // clear EEPROM
    int addr;
    for(addr=0x100; addr<PRESETS_EEPROM_SIZE; ++addr)
      status |= PRESETS_Write16(addr, 0x00);
  }

  if( status >= 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[PRESETS] reading configuration data from EEPROM!\n");
#endif

    u8 midimon_setup = PRESETS_Read16(PRESETS_ADDR_MIDIMON);
    status |= MIDIMON_InitFromPresets((midimon_setup>>0) & 1, (midimon_setup>>1) & 1, (midimon_setup>>2) & 1);

    u8 num_srio = PRESETS_Read16(PRESETS_ADDR_NUM_SRIO);
    if( num_srio )
      MIOS32_SRIO_ScanNumSet(num_srio);


    status |= UIP_TASK_InitFromPresets(PRESETS_Read16(PRESETS_ADDR_UIP_USE_DHCP),
				       PRESETS_Read32(PRESETS_ADDR_UIP_IP01),
				       PRESETS_Read32(PRESETS_ADDR_UIP_NETMASK01),
				       PRESETS_Read32(PRESETS_ADDR_UIP_GATEWAY01));

    u8 con = 0;
    for(con=0; con<PRESETS_NUM_OSC_RECORDS; ++con) {
      int offset = con*PRESETS_OFFSET_BETWEEN_OSC_RECORDS;
      status |= OSC_SERVER_InitFromPresets(con,
					   PRESETS_Read32(PRESETS_ADDR_OSC0_REMOTE01 + offset),
					   PRESETS_Read16(PRESETS_ADDR_OSC0_REMOTE_PORT + offset),
					   PRESETS_Read16(PRESETS_ADDR_OSC0_LOCAL_PORT + offset));
    }

    int kb;
    keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
    for(kb=0; kb<KEYBOARD_NUM; ++kb, ++kc) {
      int offset = kb*PRESETS_OFFSET_BETWEEN_KB_RECORDS;
      kc->midi_ports = PRESETS_Read16(PRESETS_ADDR_KB1_MIDI_PORTS + offset);
      kc->midi_chn = PRESETS_Read16(PRESETS_ADDR_KB1_MIDI_CHN + offset);

      u16 note_offsets = PRESETS_Read16(PRESETS_ADDR_KB1_NOTE_OFFSET + offset);
      kc->note_offset = (note_offsets >> 0) & 0xff;
      kc->din_key_offset = (note_offsets >> 8) & 0xff;

      kc->num_rows = PRESETS_Read16(PRESETS_ADDR_KB1_ROWS + offset);
      kc->dout_sr1 = PRESETS_Read16(PRESETS_ADDR_KB1_DOUT_SR1 + offset);
      kc->dout_sr2 = PRESETS_Read16(PRESETS_ADDR_KB1_DOUT_SR2 + offset);
      kc->din_sr1 = PRESETS_Read16(PRESETS_ADDR_KB1_DIN_SR1 + offset);
      kc->din_sr2 = PRESETS_Read16(PRESETS_ADDR_KB1_DIN_SR2 + offset);

      u16 misc = PRESETS_Read16(PRESETS_ADDR_KB1_MISC + offset);
      kc->din_inverted          = (misc & (1 << 0)) ? 1 : 0;
      kc->break_inverted        = (misc & (1 << 1)) ? 1 : 0;
      kc->scan_velocity         = (misc & (1 << 2)) ? 1 : 0;
      kc->scan_optimized        = (misc & (1 << 3)) ? 1 : 0;
      kc->scan_release_velocity = (misc & (1 << 4)) ? 1 : 0;
      kc->make_debounced        = (misc & (1 << 5)) ? 1 : 0;
      kc->break_is_make         = (misc & (1 << 6)) ? 1 : 0;

      kc->delay_fastest                    = PRESETS_Read16(PRESETS_ADDR_KB1_DELAY_FASTEST + offset);
      kc->delay_slowest                    = PRESETS_Read16(PRESETS_ADDR_KB1_DELAY_SLOWEST + offset);

      if( magic != EEPROM_MAGIC_NUMBER_OLDFORMAT1 && magic != EEPROM_MAGIC_NUMBER_OLDFORMAT2 ) {
	kc->delay_fastest_black_keys         = PRESETS_Read16(PRESETS_ADDR_KB1_DELAY_FASTEST_BLACK_KEYS + offset);
	kc->delay_fastest_release            = PRESETS_Read16(PRESETS_ADDR_KB1_DELAY_FASTEST_RELEASE + offset);
	kc->delay_fastest_release_black_keys = PRESETS_Read16(PRESETS_ADDR_KB1_DELAY_FASTEST_RELEASE_BLACK_KEYS + offset);
	kc->delay_slowest_release 	   = PRESETS_Read16(PRESETS_ADDR_KB1_DELAY_SLOWEST_RELEASE + offset);
      }

      if( magic == EEPROM_MAGIC_NUMBER_OLDFORMAT1 ) {
	u16 ain_assign = PRESETS_Read16(0xcb + offset);
	kc->ain_pin[KEYBOARD_AIN_PITCHWHEEL] = (ain_assign >> 0) & 0xff;
	kc->ain_pin[KEYBOARD_AIN_MODWHEEL]   = (ain_assign >> 8) & 0xff;

	u16 ctrl_assign = PRESETS_Read16(0xcc + offset);
	kc->ain_ctrl[KEYBOARD_AIN_PITCHWHEEL] = (ctrl_assign >> 0) & 0xff;
	kc->ain_ctrl[KEYBOARD_AIN_MODWHEEL]   = (ctrl_assign >> 8) & 0xff;
      } else if( magic == EEPROM_MAGIC_NUMBER_OLDFORMAT2 || magic == EEPROM_MAGIC_NUMBER_OLDFORMAT3 ) {
	int i;
	for(i=0; i<3; ++i) {
	  u16 ain_cfg1 = PRESETS_Read16(PRESETS_ADDR_KB1_AIN_CFG1_1 + i*2 + offset);
	  kc->ain_pin[i]  = (ain_cfg1 >> 0) & 0xff;
	  kc->ain_ctrl[i] = (ain_cfg1 >> 8) & 0xff;

	  u16 ain_cfg2 = PRESETS_Read16(PRESETS_ADDR_KB1_AIN_CFG1_2 + i*2 + offset);
	  kc->ain_min[i] = (ain_cfg2 >> 0) & 0xff;
	  kc->ain_max[i] = (ain_cfg2 >> 8) & 0xff;
	}
      } else {
#if KEYBOARD_AIN_NUM != 4
# error "please adapt the code below"
#endif

	// new format
	int i;
	for(i=0; i<KEYBOARD_AIN_NUM; ++i) {
	  u16 ain_cfg1 = PRESETS_Read16(PRESETS_ADDR_KB1_AIN_CFG1_1 + i*2 + offset);
	  kc->ain_pin[i]  = (ain_cfg1 >> 0) & 0xff;
	  kc->ain_ctrl[i] = (ain_cfg1 >> 8) & 0xff;

	  u16 ain_cfg2 = PRESETS_Read16(PRESETS_ADDR_KB1_AIN_CFG1_2 + i*2 + offset);
	  kc->ain_min[i] = (ain_cfg2 >> 0) & 0xff;
	  kc->ain_max[i] = (ain_cfg2 >> 8) & 0xff;
	}

	u16 ain_cfg5 = PRESETS_Read16(PRESETS_ADDR_KB1_AIN_CFG5 + offset);
	kc->ain_bandwidth_ms = (ain_cfg5 >> 0) & 0xff;
	kc->ain_inverted[KEYBOARD_AIN_PITCHWHEEL] = (ain_cfg5 >>  8) & 1;
	kc->ain_inverted[KEYBOARD_AIN_MODWHEEL]   = (ain_cfg5 >>  9) & 1;
	kc->ain_inverted[KEYBOARD_AIN_SUSTAIN]    = (ain_cfg5 >> 10) & 1;
	kc->ain_inverted[KEYBOARD_AIN_EXPRESSION] = (ain_cfg5 >> 11) & 1;
	kc->ain_sustain_switch                    = (ain_cfg5 >> 15) & 1;
      }

      // restore calibration data
      if( magic > EEPROM_MAGIC_NUMBER_OLDFORMAT4 ) {
	int i;

	u16 calidata_base = (kb >= 1) ? PRESETS_ADDR_KB2_CALIDATA_BEGIN : PRESETS_ADDR_KB1_CALIDATA_BEGIN;
	for(i=0; i<128 && i<KEYBOARD_MAX_KEYS; ++i) { // note: actually KEYBOARD_MAX_KEYS is 128, we just want to avoid memory overwrites for the case that somebody defines a lower number
	  u16 delay = PRESETS_Read16(calidata_base + i);
	  kc->delay_key[i] = delay;
	}
      }
    }
    KEYBOARD_Init(1); // without overwriting default configuration

  }

  // restore MIDI router settings
  {
#if MIDI_ROUTER_NUM_NODES != 16
# error "EEPROM format only prepared for 16 nodes"
#endif
    u8 node;
    midi_router_node_entry_t *n = &midi_router_node[0];
    for(node=0; node<MIDI_ROUTER_NUM_NODES; ++node, ++n) {
      u16 cfg1 = PRESETS_Read16(PRESETS_ADDR_ROUTER_BEGIN + node*2 + 0);
      u16 cfg2 = PRESETS_Read16(PRESETS_ADDR_ROUTER_BEGIN + node*2 + 1);

      // default setup
      if( !cfg1 && !cfg2 ) {
	n->src_port = USB0;
	n->src_chn = 0;
	n->dst_port = UART0;
	n->dst_chn = 17; // All
      } else {
	n->src_port = (cfg1 >> 0) & 0xff;
	n->src_chn  = (cfg1 >> 8) & 0xff;
	n->dst_port = (cfg2 >> 0) & 0xff;
	n->dst_chn  = (cfg2 >> 8) & 0xff;
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help functions to read a value from EEPROM (big endian coding!)
/////////////////////////////////////////////////////////////////////////////
u16 PRESETS_Read16(u16 addr)
{
  return EEPROM_Read(addr);
}

u32 PRESETS_Read32(u16 addr)
{
  return ((u32)EEPROM_Read(addr) << 16) | EEPROM_Read(addr+1);
}

/////////////////////////////////////////////////////////////////////////////
// Help function to write a value into EEPROM (big endian coding!)
/////////////////////////////////////////////////////////////////////////////
s32 PRESETS_Write16(u16 addr, u16 value)
{
  return EEPROM_Write(addr, value);
}

s32 PRESETS_Write32(u16 addr, u32 value)
{
  return EEPROM_Write(addr, (value >> 16) & 0xffff) | EEPROM_Write(addr+1, (value >> 0) & 0xffff);
}


/////////////////////////////////////////////////////////////////////////////
// Stores all presets
// (the EEPROM emulation ensures that a value won't be written if it is already
// equal)
/////////////////////////////////////////////////////////////////////////////
s32 PRESETS_StoreAll(void)
{
  s32 status = 0;

  // magic numbers
  status |= PRESETS_Write32(PRESETS_ADDR_MAGIC01, EEPROM_MAGIC_NUMBER);

  // write MIDImon data
  status |= PRESETS_Write16(PRESETS_ADDR_MIDIMON,
			    (MIDIMON_ActiveGet() ? 1 : 0) |
			    (MIDIMON_FilterActiveGet() ? 2 : 0) |
			    (MIDIMON_TempoActiveGet() ? 4 : 0));

  // write number of SRIOs
  status |= PRESETS_Write16(PRESETS_ADDR_NUM_SRIO,
			    MIOS32_SRIO_ScanNumGet());

  // write uIP data
  status |= PRESETS_Write16(PRESETS_ADDR_UIP_USE_DHCP, UIP_TASK_DHCP_EnableGet());
  status |= PRESETS_Write32(PRESETS_ADDR_UIP_IP01, UIP_TASK_IP_AddressGet());
  status |= PRESETS_Write32(PRESETS_ADDR_UIP_NETMASK01, UIP_TASK_NetmaskGet());
  status |= PRESETS_Write32(PRESETS_ADDR_UIP_GATEWAY01, UIP_TASK_GatewayGet());

  // write OSC data
  u8 con = 0;
  for(con=0; con<PRESETS_NUM_OSC_RECORDS; ++con) {
    int offset = con*PRESETS_OFFSET_BETWEEN_OSC_RECORDS;
    status |= PRESETS_Write32(PRESETS_ADDR_OSC0_REMOTE01 + offset, OSC_SERVER_RemoteIP_Get(con));
    status |= PRESETS_Write16(PRESETS_ADDR_OSC0_REMOTE_PORT + offset, OSC_SERVER_RemotePortGet(con));
    status |= PRESETS_Write16(PRESETS_ADDR_OSC0_LOCAL_PORT + offset, OSC_SERVER_LocalPortGet(con));
  }

  {
    int kb;
    keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
    for(kb=0; kb<KEYBOARD_NUM; ++kb, ++kc) {
      int offset = kb*PRESETS_OFFSET_BETWEEN_KB_RECORDS;
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_MIDI_PORTS + offset, kc->midi_ports);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_MIDI_CHN + offset, kc->midi_chn);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_NOTE_OFFSET + offset, kc->note_offset | ((u16)kc->din_key_offset << 8));
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_ROWS + offset, kc->num_rows);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_DOUT_SR1 + offset, kc->dout_sr1);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_DOUT_SR2 + offset, kc->dout_sr2);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_DIN_SR1 + offset, kc->din_sr1);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_DIN_SR2 + offset, kc->din_sr2);

      u16 misc =
	(kc->din_inverted          << 0) |
	(kc->break_inverted        << 1) |
	(kc->scan_velocity         << 2) |
	(kc->scan_optimized        << 3) |
        (kc->scan_release_velocity << 4) |
        (kc->make_debounced        << 5) |
        (kc->break_is_make         << 6);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_MISC + offset, misc);

      status |= PRESETS_Write16(PRESETS_ADDR_KB1_DELAY_FASTEST + offset, kc->delay_fastest);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_DELAY_SLOWEST + offset, kc->delay_slowest);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_DELAY_FASTEST_BLACK_KEYS + offset, kc->delay_fastest_black_keys);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_DELAY_FASTEST_RELEASE + offset, kc->delay_fastest_release);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_DELAY_FASTEST_RELEASE_BLACK_KEYS + offset, kc->delay_fastest_release_black_keys);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_DELAY_SLOWEST_RELEASE + offset, kc->delay_slowest_release);

      int i;
      for(i=0; i<KEYBOARD_AIN_NUM; ++i) {
	u16 ain_cfg1 =
	  (kc->ain_pin[i]  << 0) |
	  (kc->ain_ctrl[i] << 8);
	status |= PRESETS_Write16(PRESETS_ADDR_KB1_AIN_CFG1_1 + i*2 + offset, ain_cfg1);

	u16 ain_cfg2 =
	  (kc->ain_min[i] << 0) |
	  (kc->ain_max[i] << 8);
	status |= PRESETS_Write16(PRESETS_ADDR_KB1_AIN_CFG1_2 + i*2 + offset, ain_cfg2);
      }

#if KEYBOARD_AIN_NUM != 4
# error "please adapt the code below"
#endif
      u16 ain_cfg5 =
	(kc->ain_bandwidth_ms << 0) |
	(kc->ain_inverted[KEYBOARD_AIN_PITCHWHEEL] ? 0x0100 : 0) |
	(kc->ain_inverted[KEYBOARD_AIN_MODWHEEL]   ? 0x0200 : 0) |
	(kc->ain_inverted[KEYBOARD_AIN_SUSTAIN]    ? 0x0400 : 0) |
	(kc->ain_inverted[KEYBOARD_AIN_EXPRESSION] ? 0x0800 : 0) |
	(kc->ain_sustain_switch                    ? 0x8000 : 0);
      status |= PRESETS_Write16(PRESETS_ADDR_KB1_AIN_CFG5 + offset, ain_cfg5);

      // store calibration data
      {
	u16 calidata_base = (kb >= 1) ? PRESETS_ADDR_KB2_CALIDATA_BEGIN : PRESETS_ADDR_KB1_CALIDATA_BEGIN;
	for(i=0; i<128 && i<KEYBOARD_MAX_KEYS; ++i) { // note: actually KEYBOARD_MAX_KEYS is 128, we just want to avoid memory overwrites for the case that somebody defines a lower number
	  status |= PRESETS_Write16(calidata_base + i, kc->delay_key[i]);
	}
      }
    }
  }

  // store MIDI router settings
  {
#if MIDI_ROUTER_NUM_NODES != 16
# error "EEPROM format only prepared for 16 nodes"
#endif
    u8 node;
    midi_router_node_entry_t *n = &midi_router_node[0];
    for(node=0; node<MIDI_ROUTER_NUM_NODES; ++node, ++n) {
      u16 cfg1 = n->src_port | ((u16)n->src_chn << 8);
      u16 cfg2 = n->dst_port | ((u16)n->dst_chn << 8);

      status |= PRESETS_Write16(PRESETS_ADDR_ROUTER_BEGIN + node*2 + 0, cfg1);
      status |= PRESETS_Write16(PRESETS_ADDR_ROUTER_BEGIN + node*2 + 1, cfg2);
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  if( status < 0 ) {
    DEBUG_MSG("[PRESETS] ERROR while writing into EEPROM!");
  }
#endif

  return 0; // no error
}

