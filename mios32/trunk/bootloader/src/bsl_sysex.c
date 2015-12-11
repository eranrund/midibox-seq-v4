// $Id: sysex.c 78 2008-10-12 22:09:23Z tk $
/*
 * BSL SysEx Parser
 *
 * ==========================================================================
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
#include <string.h>

#include "bsl_sysex.h"


/////////////////////////////////////////////////////////////////////////////
// Local Macros
/////////////////////////////////////////////////////////////////////////////

#define MEM32(addr) (*((volatile u32 *)(addr)))
#define MEM16(addr) (*((volatile u16 *)(addr)))
#define MEM8(addr)  (*((volatile u8  *)(addr)))


#if defined(MIOS32_FAMILY_STM32F10x)
  // STM32: determine page size (mid density devices: 1k, high density devices: 2k)
  // TODO: find a proper way, as there could be high density devices with less than 256k?)
# define FLASH_PAGE_SIZE   (MIOS32_SYS_FlashSizeGet() >= (256*1024) ? 0x800 : 0x400)

  // STM32: flash memory range (16k BSL range excluded)
# define FLASH_START_ADDR  (0x08000000 + 0x4000)
# define FLASH_END_ADDR    (0x08000000 + MIOS32_SYS_FlashSizeGet() - 1)


  // STM32: base address of SRAM
# define SRAM_START_ADDR   (0x20000000)
# define SRAM_END_ADDR     (0x20000000 + MIOS32_SYS_RAMSizeGet() - 1)

#elif defined(MIOS32_FAMILY_STM32F4xx)

// sector base addresses

#define MAX_FLASH_SECTOR 12
const u32 flash_sector_map[MAX_FLASH_SECTOR][3] = {
  { 0xffffffff, FLASH_Sector_0 }, /* Base @ of Sector 0, 16 Kbyte */ // TK: actually 0x08000000, ensure that it won't be taken
  { 0x08004000, FLASH_Sector_1 }, /* Base @ of Sector 1, 16 Kbyte */
  { 0x08008000, FLASH_Sector_2 }, /* Base @ of Sector 2, 16 Kbyte */
  { 0x0800C000, FLASH_Sector_3 }, /* Base @ of Sector 3, 16 Kbyte */
  { 0x08010000, FLASH_Sector_4 }, /* Base @ of Sector 4, 64 Kbyte */
  { 0x08020000, FLASH_Sector_5 }, /* Base @ of Sector 5, 128 Kbyte */
  { 0x08040000, FLASH_Sector_6 }, /* Base @ of Sector 6, 128 Kbyte */
  { 0x08060000, FLASH_Sector_7 }, /* Base @ of Sector 7, 128 Kbyte */
  { 0x08080000, FLASH_Sector_8 }, /* Base @ of Sector 8, 128 Kbyte */
  { 0x080A0000, FLASH_Sector_9 }, /* Base @ of Sector 9, 128 Kbyte */
  { 0x080C0000, FLASH_Sector_10 }, /* Base @ of Sector 10, 128 Kbyte */
  { 0x080E0000, FLASH_Sector_11 }, /* Base @ of Sector 11, 128 Kbyte */
};

static u32 flash_erase_done = 0;
#if MAX_FLASH_SECTOR > 32
# error "Please adapt value range of flash_erase_done!"
#endif

  // STM32: flash memory range (16k BSL range excluded)
# define FLASH_START_ADDR  (0x08000000 + 0x4000)
# define FLASH_END_ADDR    (0x08000000 + MIOS32_SYS_FlashSizeGet() - 1)


  // STM32: base address of SRAM
# define SRAM_START_ADDR   (0x20000000)
# define SRAM_END_ADDR     (0x20000000 + MIOS32_SYS_RAMSizeGet() - 1)

#elif defined(MIOS32_FAMILY_LPC17xx)

#include <sbl_iap.h>
#include <sbl_config.h>

# undef  FLASH_BUF_SIZE
# define FLASH_BUF_SIZE    BSL_SYSEX_BUFFER_SIZE
# undef  USER_START_SECTOR
# define USER_START_SECTOR  4
# define MAX_USER_SECTOR   29
# define USER_FLASH_START (sector_start_map[USER_START_SECTOR])
# define USER_FLASH_END   (sector_end_map[MAX_USER_SECTOR])
# define USER_FLASH_SIZE  ((USER_FLASH_END - USER_FLASH_START) + 1)

  // LPC17xx: sectors have different sizes
const unsigned sector_start_map[MAX_FLASH_SECTOR] = {SECTOR_0_START,             \
SECTOR_1_START,SECTOR_2_START,SECTOR_3_START,SECTOR_4_START,SECTOR_5_START,      \
SECTOR_6_START,SECTOR_7_START,SECTOR_8_START,SECTOR_9_START,SECTOR_10_START,     \
SECTOR_11_START,SECTOR_12_START,SECTOR_13_START,SECTOR_14_START,SECTOR_15_START, \
SECTOR_16_START,SECTOR_17_START,SECTOR_18_START,SECTOR_19_START,SECTOR_20_START, \
SECTOR_21_START,SECTOR_22_START,SECTOR_23_START,SECTOR_24_START,SECTOR_25_START, \
SECTOR_26_START,SECTOR_27_START,SECTOR_28_START,SECTOR_29_START                  };

const unsigned sector_end_map[MAX_FLASH_SECTOR] = {SECTOR_0_END,SECTOR_1_END,    \
SECTOR_2_END,SECTOR_3_END,SECTOR_4_END,SECTOR_5_END,SECTOR_6_END,SECTOR_7_END,   \
SECTOR_8_END,SECTOR_9_END,SECTOR_10_END,SECTOR_11_END,SECTOR_12_END,             \
SECTOR_13_END,SECTOR_14_END,SECTOR_15_END,SECTOR_16_END,SECTOR_17_END,           \
SECTOR_18_END,SECTOR_19_END,SECTOR_20_END,SECTOR_21_END,SECTOR_22_END,           \
SECTOR_23_END,SECTOR_24_END,SECTOR_25_END,SECTOR_26_END,                         \
SECTOR_27_END,SECTOR_28_END,SECTOR_29_END                                        };

  // expected by flash programming routine: system core clock (100/120 MHz) in kHz
# define SYSTEM_CORE_CLOCK_KHZ (MIOS32_SYS_CPU_FREQUENCY/1000)

  // LPC17xx: flash memory range (16k BSL range excluded)
# define FLASH_START_ADDR  (0x00000000 + 0x4000)
# define FLASH_END_ADDR    (0x00000000 + MIOS32_SYS_FlashSizeGet() - 1)


  // LPC17xx: base address of SRAM
# define SRAM_START_ADDR   (0x10000000)
# define SRAM_END_ADDR     (0x10000000 + MIOS32_SYS_RAMSizeGet() - 1)

static void iap_entry(unsigned param_tab[], unsigned result_tab[]);
static s32 write_data(unsigned cclk,unsigned flash_address, unsigned *flash_data_buf, unsigned count);
static s32 find_erase_prepare_sector(unsigned cclk, unsigned flash_address);
static s32 erase_sector(unsigned start_sector, unsigned end_sector, unsigned cclk);
static s32 prepare_sector(unsigned start_sector, unsigned end_sector, unsigned cclk);

#else
# error "BSL not prepared for this family"
#endif


/////////////////////////////////////////////////////////////////////////////
// Internal Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 BSL_SYSEX_Cmd_ReadMem(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 BSL_SYSEX_Cmd_WriteMem(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in);

static s32 BSL_SYSEX_RecAddrAndLen(u8 midi_in);

static s32 BSL_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg);
static s32 BSL_SYSEX_SendMem(mios32_midi_port_t port, u32 addr, u32 len);
static s32 BSL_SYSEX_WriteMem(u32 addr, u32 len, u8 *buffer);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static bsl_sysex_rec_state_t sysex_rec_state;

static u32 sysex_addr;
static u32 sysex_len;
static u8 sysex_checksum;
static u8 sysex_received_checksum;
static u32 sysex_receive_ctr;


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

// ensure that the buffer is located at a word boundary (required for LPC17 flash programming routines)
static u8 sysex_buffer[BSL_SYSEX_BUFFER_SIZE] __attribute__ ((aligned (4)));

static u8 halt_state;


/////////////////////////////////////////////////////////////////////////////
// This function initializes the SysEx handler
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // set to one when writing flash to prevent the execution of application code
  // so long flash hasn't been programmed completely
  halt_state = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if BSL is in halt state (e.g. code is uploaded)
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_HaltStateGet(void)
{
  return halt_state;
}


/////////////////////////////////////////////////////////////////////////////
// Used by MIOS32_MIDI to release halt state instead of triggering a reset
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_ReleaseHaltState(void)
{
  // always send upload request (like if we would come out of reset)
  BSL_SYSEX_SendUploadReq(UART0);    
  BSL_SYSEX_SendUploadReq(USB0);

  // clear halt state
  halt_state = 0;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function enhances MIOS32 SysEx commands
// it's called from MIOS32_MIDI_SYSEX_Cmd if the "MIOS32_MIDI_BSL_ENHANCEMENTS"
// switch is set (see code there for details)
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Cmd(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in, u8 sysex_cmd)
{
  // change debug port
  MIOS32_MIDI_DebugPortSet(port);

  // wait 2 additional seconds whenever a SysEx message has been received
  MIOS32_STOPWATCH_Reset();

  // enter the commands here
  switch( sysex_cmd ) {
    // case 0x00: // query command is implemented in MIOS32
    // case 0x0f: // ping command is implemented in MIOS32

    case 0x01:
      BSL_SYSEX_Cmd_ReadMem(port, cmd_state, midi_in);
      break;
    case 0x02:
      BSL_SYSEX_Cmd_WriteMem(port, cmd_state, midi_in);
      break;

    default:
      // unknown command
      return -1; // command not supported
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 01: Read Memory handler
// TODO: we could provide this command also during runtime, as it isn't destructive
// or it could be available as debug command 0D like known from MIOS8
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Cmd_ReadMem(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN:
      // set initial receive state and address/len
      sysex_rec_state = BSL_SYSEX_REC_A3;
      sysex_addr = 0;
      sysex_len = 0;
      break;

    case MIOS32_MIDI_SYSEX_CMD_STATE_CONT:
      if( sysex_rec_state < BSL_SYSEX_REC_PAYLOAD )
	BSL_SYSEX_RecAddrAndLen(midi_in);
      break;

    default: // BSL_SYSEX_CMD_STATE_END
      // TODO: send 0xf7 if merger enabled

      // did we reach payload state?
      if( sysex_rec_state != BSL_SYSEX_REC_PAYLOAD ) {
	// not enough bytes received
	BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else {
	// send dump
	BSL_SYSEX_SendMem(port, sysex_addr, sysex_len);
      }

      break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 02: Write Memory handler
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Cmd_WriteMem(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  static u32 bit_ctr8 = 0;
  static u32 value8 = 0;

  switch( cmd_state ) {

    case MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN:
      // set initial receive state and address/len
      sysex_rec_state = BSL_SYSEX_REC_A3;
      sysex_addr = 0;
      sysex_len = 0;
      // clear checksum and receive counters
      sysex_checksum = 0;
      sysex_received_checksum = 0;

      sysex_receive_ctr = 0;
      bit_ctr8 = 0;
      value8 = 0;
      break;

    case MIOS32_MIDI_SYSEX_CMD_STATE_CONT:
      if( sysex_rec_state < BSL_SYSEX_REC_PAYLOAD ) {
	sysex_checksum += midi_in;
	BSL_SYSEX_RecAddrAndLen(midi_in);
      } else if( sysex_rec_state == BSL_SYSEX_REC_PAYLOAD ) {
	sysex_checksum += midi_in;
	// new byte has been received - descramble and buffer it
	if( sysex_receive_ctr < BSL_SYSEX_MAX_BYTES ) {
	  u8 value7 = midi_in;
	  int bit_ctr7;
	  for(bit_ctr7=0; bit_ctr7<7; ++bit_ctr7) {
	    value8 = (value8 << 1) | ((value7 & 0x40) ? 1 : 0);
	    value7 <<= 1;

	    if( ++bit_ctr8 >= 8 ) {
	      sysex_buffer[sysex_receive_ctr] = value8;
	      bit_ctr8 = 0;
	      if( ++sysex_receive_ctr >= sysex_len )
		sysex_rec_state = BSL_SYSEX_REC_CHECKSUM;
	    }
	  }
	}
      } else if( sysex_rec_state == BSL_SYSEX_REC_CHECKSUM ) {
	// store received checksum
	sysex_received_checksum = midi_in;
      } else {
	// too many bytes... wait for F7
	sysex_rec_state = BSL_SYSEX_REC_INVALID;
      }
      break;

    default: // MIOS32_MIDI_SYSEX_CMD_STATE_END
      // TODO: send 0xf7 if merger enabled

      if( sysex_receive_ctr < sysex_len ) {
	// for remote analysis...
#ifndef MIOS32_MIDI_DISABLE_DEBUG_MESSAGE
	MIOS32_MIDI_SendDebugMessage("[BSL_SYSEX] expected %d, got %d bytes (retry)\n", sysex_len, sysex_receive_ctr);
#endif
	// not enough bytes received
	BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else if( sysex_rec_state == BSL_SYSEX_REC_INVALID ) {
	// too many bytes received
	BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_MORE_BYTES_THAN_EXP);
      } else if( sysex_received_checksum != (-sysex_checksum & 0x7f) ) {
	// notify that wrong checksum has been received
	BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_WRONG_CHECKSUM);
      } else {
	// enter halt state (can only be released via BSL reset)
	halt_state = 1;

	// write received data into memory
	s32 error;
	if( (error = BSL_SYSEX_WriteMem(sysex_addr, sysex_len, sysex_buffer)) ) {
	  // write failed - return negated error status
	  BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, -error);
	} else {
	  // notify that bytes have been received by returning checksum
	  BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, -sysex_checksum & 0x7f);
	}

	// enfore immediate MIDI queue flush
	// (important for retry handling: send ack before next message is processed)
	MIOS32_MIDI_Periodic_mS();
      }
      break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help function to receive address and length
/////////////////////////////////////////////////////////////////////////////
static s32 BSL_SYSEX_RecAddrAndLen(u8 midi_in)
{
  if( sysex_rec_state <= BSL_SYSEX_REC_A0 ) {
    sysex_addr = (sysex_addr << 7) | ((midi_in & 0x7f) << 4);
    if( sysex_rec_state == BSL_SYSEX_REC_A0 )
      sysex_rec_state = BSL_SYSEX_REC_L3;
    else
      ++sysex_rec_state;
  } else if( sysex_rec_state <= BSL_SYSEX_REC_L0 ) {
    sysex_len = (sysex_len << 7) | ((midi_in & 0x7f) << 4);
    if( sysex_rec_state == BSL_SYSEX_REC_L0 ) {
      sysex_rec_state = BSL_SYSEX_REC_PAYLOAD;
    } else {
      ++sysex_rec_state;
    }
  } else {
    return -1; // function shouldn't be called in this state
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx acknowledge to notify the user about the received command
// expects acknowledge code (e.g. 0x0f for good, 0x0e for error) and additional argument
/////////////////////////////////////////////////////////////////////////////
static s32 BSL_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  for(i=0; i<sizeof(mios32_midi_sysex_header); ++i)
    *sysex_buffer_ptr++ = mios32_midi_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = MIOS32_MIDI_DeviceIDGet();

  // send ack code and argument
  *sysex_buffer_ptr++ = ack_code;
  *sysex_buffer_ptr++ = ack_arg;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
}


/////////////////////////////////////////////////////////////////////////////
// This function sends an upload request
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_SendUploadReq(mios32_midi_port_t port)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  for(i=0; i<sizeof(mios32_midi_sysex_header); ++i)
    *sysex_buffer_ptr++ = mios32_midi_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = MIOS32_MIDI_DeviceIDGet();

  // send 0x01 to request code upload
  *sysex_buffer_ptr++ = 0x01;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx dump of the requested memory address range
// We expect that address and length are aligned to 16
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_SendMem(mios32_midi_port_t port, u32 addr, u32 len)
{
  int i;
  u8 checksum = 0;

  // send header
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  for(i=0; i<sizeof(mios32_midi_sysex_header); ++i)
    *sysex_buffer_ptr++ = mios32_midi_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = MIOS32_MIDI_DeviceIDGet();

  // "write mem" command (so that dump could be sent back to overwrite the memory w/o modifications)
  *sysex_buffer_ptr++ = 0x02;

  // send 32bit address (divided by 16) in 7bit format
  checksum += *sysex_buffer_ptr++ = (addr >> 25) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (addr >> 18) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (addr >> 11) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (addr >>  4) & 0x7f;

  // send 32bit range (divided by 16) in 7bit format
  checksum += *sysex_buffer_ptr++ = (len >> 25) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (len >> 18) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (len >> 11) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (len >>  4) & 0x7f;

  // send memory content in scrambled format (8bit values -> 7bit values)
  u8 value7 = 0;
  u8 bit_ctr7 = 0;
  i=0;
  for(i=0; i<len; ++i) {
    u8 value8 = MEM8(addr+i);
    u8 bit_ctr8;
    for(bit_ctr8=0; bit_ctr8<8; ++bit_ctr8) {
      value7 = (value7 << 1) | ((value8 & 0x80) ? 1 : 0);
      value8 <<= 1;

      if( ++bit_ctr7 >= 7 ) {
	checksum += *sysex_buffer_ptr++ = (value7 << (7-bit_ctr7));
	value7 = 0;
	bit_ctr7 = 0;
      }
    }
  }

  if( bit_ctr7 )
    checksum += *sysex_buffer_ptr++ = value7;

  // send checksum
  *sysex_buffer_ptr++ = -checksum & 0x7f;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
}


/////////////////////////////////////////////////////////////////////////////
// This function writes into a memory
// We expect that address and length are aligned to 4
/////////////////////////////////////////////////////////////////////////////
static s32 BSL_SYSEX_WriteMem(u32 addr, u32 len, u8 *buffer)
{
  // check for alignment
  if( (addr % 4) || (len % 4) )
    return -MIOS32_MIDI_SYSEX_DISACK_ADDR_NOT_ALIGNED;

  // check for flash memory range
  if( addr >= FLASH_START_ADDR && addr <= FLASH_END_ADDR ) {
#if defined(MIOS32_FAMILY_STM32F10x)
    // FLASH_* routines are part of the STM32 code library
    FLASH_Unlock();

    FLASH_Status status;
    int i;
    for(i=0; i<len; addr+=2, i+=2) {
      MIOS32_IRQ_Disable();
      if( (addr % FLASH_PAGE_SIZE) == 0 ) {
	if( (status=FLASH_ErasePage(addr)) != FLASH_COMPLETE ) {
	  FLASH_ClearFlag(FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); // clear error flags, otherwise next program attempts will fail
	  MIOS32_IRQ_Enable();
	  return -MIOS32_MIDI_SYSEX_DISACK_WRITE_FAILED;
	}
      }

      if( (status=FLASH_ProgramHalfWord(addr, buffer[i+0] | ((u16)buffer[i+1] << 8))) != FLASH_COMPLETE ) {
	FLASH_ClearFlag(FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); // clear error flags, otherwise next program attempts will fail
	MIOS32_IRQ_Enable();
	return -MIOS32_MIDI_SYSEX_DISACK_WRITE_FAILED;
      }
      MIOS32_IRQ_Enable();

      // TODO: verify programmed code
    }
#elif defined(MIOS32_FAMILY_STM32F4xx)
    // FLASH_* routines are part of the STM32 code library
    FLASH_Unlock();

    // erase if new sector is reached
    {
      int sector;
      for(sector=0; sector<MAX_FLASH_SECTOR; ++sector) {
	if( addr == flash_sector_map[sector][0] ) {
	  // erase only if really required
	  // helps in the case, that an erase takes more than 1 second.
	  // if this happens, MIOS Studio will retry the memory transfer, and in this case the sector will be erased again.
	  // period...
	  u8 erase_required = 0;
	  {
	    if( addr == FLASH_START_ADDR ) { // using app start address as an indicator that all flash sectors have to be erased (again)
	      flash_erase_done = 0;
	    }

	    u32 flash_sector_mask = (1 << sector);
	    if( !(flash_erase_done & flash_sector_mask) ) {
	      erase_required = 1;
	      flash_erase_done |= flash_sector_mask;
	    }
	  }

	  if( erase_required ) {
	    FLASH_Status status = FLASH_EraseSector(flash_sector_map[sector][1], VoltageRange_3);
	    if( status != FLASH_COMPLETE ) {
	      FLASH_ClearFlag(0xffffffff); // clear error flags, otherwise next program attempts will fail
#ifndef MIOS32_MIDI_DISABLE_DEBUG_MESSAGE
	      MIOS32_MIDI_SendDebugMessage("erase failed for 0x%08x: code %d\n", addr, status);
#endif
	      return -MIOS32_MIDI_SYSEX_DISACK_WRITE_FAILED;
	    }
	  }
	  break;
	}
      }
    }

    int i;
    for(i=0; i<len; addr+=4, i+=4) {
      uint32_t data =
	((uint64_t)buffer[i+0] <<  0) |
	((uint64_t)buffer[i+1] <<  8) |
	((uint64_t)buffer[i+2] << 16) |
	((uint64_t)buffer[i+3] << 24);

      FLASH_Status status = FLASH_ProgramWord(addr, data);
      if( status != FLASH_COMPLETE ) {
	FLASH_ClearFlag(0xffffffff); // clear error flags, otherwise next program attempts will fail
#ifndef MIOS32_MIDI_DISABLE_DEBUG_MESSAGE
	MIOS32_MIDI_SendDebugMessage("program flash failed for 0x%08x: code %d\n", addr, status);
#endif
	return -MIOS32_MIDI_SYSEX_DISACK_WRITE_FAILED;
      }

      FLASH_DataCacheReset();
      FLASH_InstructionCacheReset();

      // TODO: verify programmed code
    }

#elif defined(MIOS32_FAMILY_LPC17xx)
    MIOS32_IRQ_Disable();

    s32 status;
    if( (status=find_erase_prepare_sector(SYSTEM_CORE_CLOCK_KHZ, addr)) < 0 ) {
      MIOS32_IRQ_Enable();
#ifndef MIOS32_MIDI_DISABLE_DEBUG_MESSAGE
      MIOS32_MIDI_SendDebugMessage("erase failed for 0x%08x: code %d\n", addr, status);
#endif
      return -MIOS32_MIDI_SYSEX_DISACK_WRITE_FAILED;
    } else if( (status=write_data(SYSTEM_CORE_CLOCK_KHZ, addr, (unsigned *)buffer, len)) < 0 ) {
      MIOS32_IRQ_Enable();
#ifndef MIOS32_MIDI_DISABLE_DEBUG_MESSAGE
      MIOS32_MIDI_SendDebugMessage("write_data failed for 0x%08x: code %d\n", addr, status);
#endif

      return -MIOS32_MIDI_SYSEX_DISACK_WRITE_FAILED;
    }
    MIOS32_IRQ_Enable();

    // TODO: verify programmed code
#else
# error "Flash Programming not prepared for this family"
#endif

    return 0; // no error
  }

  // check for SRAM memory range
  if( addr >= SRAM_START_ADDR && addr <= SRAM_END_ADDR ) {

    // transfer buffer into SRAM
    memcpy((u8 *)addr, (u8 *)buffer, len);

    return 0; // no error
  }

  // invalid address
  return -MIOS32_MIDI_SYSEX_DISACK_WRONG_ADDR_RANGE;
}



#if defined(MIOS32_FAMILY_LPC17xx)

// to enter IAP routines
static void iap_entry(unsigned param_tab[], unsigned result_tab[])
{
  void (*iap)(unsigned [],unsigned []);

  iap = (void (*)(unsigned [],unsigned []))IAP_ADDRESS;
  iap(param_tab,result_tab);
}

s32 find_erase_prepare_sector(unsigned cclk, unsigned flash_address)
{
  unsigned i;
  s32 result = 0;

  for(i=USER_START_SECTOR; i<=MAX_USER_SECTOR; i++) {
    if(flash_address < sector_end_map[i]) {
      if( flash_address == sector_start_map[i]) {
	if( prepare_sector(i, i, cclk) < 0 )
	  result = -1;
	if( erase_sector(i, i ,cclk) < 0 )
	  result = -2;
      }
      if( prepare_sector(i, i, cclk) < 0 )
	result = -3;
      break;
    }
  }

  return result;
}

s32 write_data(unsigned cclk, unsigned flash_address, unsigned *flash_data_buf, unsigned count)
{
  unsigned param_table[5];
  unsigned result_table[5];

  param_table[0] = COPY_RAM_TO_FLASH;
  param_table[1] = flash_address;
  param_table[2] = (unsigned)flash_data_buf;
  param_table[3] = count;
  param_table[4] = cclk;
  iap_entry(param_table, result_table);

  return -(s32)result_table[0];
}

s32 erase_sector(unsigned start_sector, unsigned end_sector, unsigned cclk)
{
  unsigned param_table[5];
  unsigned result_table[5];

  param_table[0] = ERASE_SECTOR;
  param_table[1] = start_sector;
  param_table[2] = end_sector;
  param_table[3] = cclk;
  iap_entry(param_table, result_table);

  return -(s32)result_table[0];
}

s32 prepare_sector(unsigned start_sector, unsigned end_sector, unsigned cclk)
{
  unsigned param_table[5];
  unsigned result_table[5];

  param_table[0] = PREPARE_SECTOR_FOR_WRITE;
  param_table[1] = start_sector;
  param_table[2] = end_sector;
  param_table[3] = cclk;
  iap_entry(param_table, result_table);

  return -(s32)result_table[0];
}

#endif
