// $Id: app.c 2035 2014-08-18 20:16:47Z tk $
//! \defgroup MIOS32_APP
//! MIDIbox NG Application
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

// prints execution time on each received event
#define DEBUG_EVENT_HANDLER_PERFORMANCE 0

/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <msd.h>
#include "app.h"
#include "tasks.h"

#include <glcd_font.h>

#include <ainser.h>
#include <aout.h>
#include <midi_port.h>
#include <midi_router.h>
#include <midimon.h>
#include <keyboard.h>

#include "mbng_sysex.h"
#include "mbng_patch.h"
#include "mbng_event.h"
#include "mbng_lcd.h"
#include "mbng_din.h"
#include "mbng_dout.h"
#include "mbng_dio.h"
#include "mbng_enc.h"
#include "mbng_ain.h"
#include "mbng_ainser.h"
#include "mbng_kb.h"
#include "mbng_matrix.h"
#include "mbng_mf.h"
#include "mbng_cv.h"
#include "mbng_seq.h"

/*
// include source of the SCS
#include <scs.h>
#include <scs_lcd.h>
#include "scs_config.h"
*/

#include "file.h"
#include "mbng_file.h"
#include "mbng_file_c.h"
#include "mbng_file_l.h"
#include "mbng_file_r.h"

#include <seq_bpm.h>
#include <seq_midi_out.h>

#include "terminal.h"

#include "uip_task.h"
#include "osc_client.h"

#include "mbfm.h"

// Make sure only STM32F4
#if !defined(MIOS32_BOARD_STM32F4DISCOVERY) && !defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
#error "MIDIbox FM V2.1 only supported on STM32F4 MCU!"
#endif

/////////////////////////////////////////////////////////////////////////////
//! Local types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  MSD_DISABLED,
  MSD_INIT,
  MSD_READY,
  MSD_SHUTDOWN
} msd_state_t;


/////////////////////////////////////////////////////////////////////////////
//! global variables
/////////////////////////////////////////////////////////////////////////////
u8  hw_enabled;
u8  debug_verbose_level;

/////////////////////////////////////////////////////////////////////////////
//! local variables
/////////////////////////////////////////////////////////////////////////////

// for mutual exclusive SD Card access between different tasks
// The mutex is handled with MUTEX_SDCARD_TAKE and MUTEX_SDCARD_GIVE
// macros inside the application, which contain a different implementation 
// for emulation
xSemaphoreHandle xSDCardSemaphore;

// Mutex for MIDI IN/OUT handler
xSemaphoreHandle xMIDIINSemaphore;
xSemaphoreHandle xMIDIOUTSemaphore;

// Mutex for LCD access
xSemaphoreHandle xLCDSemaphore;

// Mutex for J16 access (SDCard/Ethernet)
xSemaphoreHandle xJ16Semaphore;

static volatile msd_state_t msd_state;


/////////////////////////////////////////////////////////////////////////////
//! Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package);
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port);

static void APP_AINSER_NotifyChange(u32 module, u32 pin, u32 pin_value);


/////////////////////////////////////////////////////////////////////////////
//! This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize stopwatch for measuring delays
  MIOS32_STOPWATCH_Init(100);

  // hardware will be enabled once configuration has been loaded from SD Card
  // (resp. no SD Card is available)
  hw_enabled = 0;

  // only print error messages by default
  debug_verbose_level = DEBUG_VERBOSE_LEVEL_ERROR;

  // disable MSD by default (has to be enabled in SHIFT menu)
  msd_state = MSD_DISABLED;

  // hardware will be enabled once configuration has been loaded from SD Card
  // (resp. no SD Card is available)
  hw_enabled = 0;

  // initialize all J10 pins as inputs with internal Pull-Up
  int pin;
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J10_PinInit(pin, MIOS32_BOARD_PIN_MODE_INPUT_PU);

  // initialize LCDs
  MBNG_LCD_Init(0);

  // initialize the AINSER module(s)
  AINSER_Init(0);

  // initialize the AOUT module(s)
  AOUT_Init(0);

  // create semaphores
  xSDCardSemaphore = xSemaphoreCreateRecursiveMutex();
  xMIDIINSemaphore = xSemaphoreCreateRecursiveMutex();
  xMIDIOUTSemaphore = xSemaphoreCreateRecursiveMutex();
  xLCDSemaphore = xSemaphoreCreateRecursiveMutex();
  xJ16Semaphore = xSemaphoreCreateRecursiveMutex();

  // install SysEx callback
  MIOS32_MIDI_SysExCallback_Init(APP_SYSEX_Parser);

  // install MIDI Rx/Tx callback functions
  MIOS32_MIDI_DirectRxCallback_Init(&NOTIFY_MIDI_Rx);
  MIOS32_MIDI_DirectTxCallback_Init(&NOTIFY_MIDI_Tx);

  // install timeout callback function
  MIOS32_MIDI_TimeOutCallback_Init(&NOTIFY_MIDI_TimeOut);

  // initialize code modules
  MIDI_PORT_Init(0);
  MBNG_SYSEX_Init(0);
  MIDI_ROUTER_Init(0);
  MBNG_EVENT_Init(0);
  MBNG_DIN_Init(0);
  MBNG_DOUT_Init(0);
  MBNG_ENC_Init(0);
  MBNG_MF_Init(0);
  MBNG_AIN_Init(0);
  MBNG_AINSER_Init(0);
  MBNG_CV_Init(0);
  MBNG_KB_Init(0);
  MBNG_MATRIX_Init(0);
  UIP_TASK_Init(0);
  //SCS_Init(0);
  //SCS_CONFIG_Init(0);
  TERMINAL_Init(0);
  MIDIMON_Init(0);
  MBNG_FILE_Init(0);
  MBNG_SEQ_Init(0);
  SEQ_MIDI_OUT_Init(0);

  MBNG_PATCH_Init(0);
  
  MBFM_Init();

  //KEYBOARD_Init(0);
  // done in MBNG_PATCH_Init()

#if MIOS32_DONT_SERVICE_SRIO_SCAN
  //MIOS32_SRIO_ScanNumSet(4);

  // standard SRIO scan has been disabled in programming_models/traditional/main.c via MIOS32_DONT_SERVICE_SRIO_SCAN in mios32_config.h
  // start the scan here - and retrigger it whenever it's finished
  APP_SRIO_ServicePrepare();
  MIOS32_SRIO_ScanStart(APP_SRIO_ServiceFinish);
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  const u16 sdcard_check_delay = 1000;
  u16 sdcard_check_ctr = 0;
  u8 lun_available = 0;
  //static u8 isInMainPage = 1;

  //SCS_DisplayUpdateInMainPage(0);
  MBNG_LCD_SpecialCharsReInit();

  u32 last_timestamp = MIOS32_TIMESTAMP_Get();
  while( 1 ) {
    //vTaskDelay(1 / portTICK_RATE_MS);
    // Background task: use timestamp mechanism to generate delay
    while( MIOS32_TIMESTAMP_Get() == last_timestamp );
    last_timestamp = MIOS32_TIMESTAMP_Get();

    /*
    // call SCS handler
    MUTEX_LCD_TAKE;
    MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
    SCS_Tick();

    SCS_DisplayUpdateInMainPage(MBNG_EVENT_MidiLearnModeGet() ? 1 : 0);

    // LCD output in mainpage
    if( SCS_MenuStateGet() == SCS_MENU_STATE_MAINPAGE && !MBNG_EVENT_MidiLearnModeGet() ) {
      u8 force = isInMainPage == 0;
      if( force ) { // page change
	MBNG_LCD_SpecialCharsReInit();
	MBNG_LCD_CursorSet(SCS_LCD_DeviceGet(), SCS_LCD_OffsetXGet(), SCS_LCD_OffsetYGet() + 0);
	MBNG_LCD_PrintSpaces(SCS_NumMenuItemsGet()*SCS_MENU_ITEM_WIDTH);
	MBNG_LCD_CursorSet(SCS_LCD_DeviceGet(), SCS_LCD_OffsetXGet(), SCS_LCD_OffsetYGet() + 1);
	MBNG_LCD_PrintSpaces(SCS_NumMenuItemsGet()*SCS_MENU_ITEM_WIDTH);
      }

      MBNG_EVENT_UpdateLCD(force);

      // handles .NGR file execution
      MBNG_FILE_R_CheckRequest();

      isInMainPage = 1; // static reminder
    } else {
      if( isInMainPage && MBNG_EVENT_MidiLearnModeGet() ) {
	SCS_LCD_Update(1); // force display update when learn mode is entered in mainpage
      }

      isInMainPage = 0; // static reminder
    }

    // handles .NGR file execution
    MBNG_FILE_R_CheckRequest();

    MUTEX_LCD_GIVE;
    */

    // -> keyboard handler
    KEYBOARD_Periodic_1mS();

    // MIDI In/Out monitor
    MIDI_PORT_Period1mS();
    
    // MBFM handler
    MBFM_BackgroundTick(last_timestamp);
    
    // call MIDI event tick
    MBNG_EVENT_Tick();

    // each second: check if SD Card (still) available
    if( msd_state == MSD_DISABLED && ++sdcard_check_ctr >= sdcard_check_delay ) {
      sdcard_check_ctr = 0;

      MUTEX_SDCARD_TAKE;
      s32 status = FILE_CheckSDCard();

      if( status == 1 ) {
	DEBUG_MSG("SD Card connected: %s\n", FILE_VolumeLabel());

	// stop sequencer
	SEQ_BPM_Stop();

	// load all file infos
	MBNG_FILE_LoadAllFiles(1); // including HW info

	// select the first bank
	MBNG_EVENT_SelectedBankSet(1);

	// refresh display for MBFM
	MBFM_InitAfterFrontPanel();
	
	// immediately go to next step
	sdcard_check_ctr = sdcard_check_delay;
      } else if( status == 2 ) {
	DEBUG_MSG("SD Card disconnected\n");
	// invalidate all file infos
	MBNG_FILE_UnloadAllFiles();

	// stop sequencer
	SEQ_BPM_Stop();

	// change status
	MBNG_FILE_StatusMsgSet("No SD Card");

	MUTEX_LCD_TAKE;
	MBNG_LCD_CursorSet(0, 0, 0);
	MBNG_LCD_PrintString("*** No SD Card *** ");
	MBNG_LCD_ClearScreenOnNextMessage();
	MUTEX_LCD_GIVE;
      } else if( status == 3 ) {
	if( !FILE_SDCardAvailable() ) {
	  DEBUG_MSG("SD Card not found\n");
	  MBNG_FILE_StatusMsgSet("No SD Card");

	  MUTEX_LCD_TAKE;
	  MBNG_LCD_CursorSet(0, 0, 0);
	  MBNG_LCD_PrintString("*** No SD Card *** ");
	  MBNG_LCD_ClearScreenOnNextMessage();
	  MUTEX_LCD_GIVE;
	} else if( !FILE_VolumeAvailable() ) {
	  DEBUG_MSG("ERROR: SD Card contains invalid FAT!\n");
	  MBNG_FILE_StatusMsgSet("No FAT");

	  MUTEX_LCD_TAKE;
	  MBNG_LCD_CursorSet(0, 0, 0);
	  MBNG_LCD_PrintString("* No FAT on SD Card * ");
	  MBNG_LCD_ClearScreenOnNextMessage();
	  MUTEX_LCD_GIVE;
	} else {
	  MBNG_FILE_StatusMsgSet(NULL);

	  // create the default files if they don't exist on SD Card
	  MBNG_FILE_CreateDefaultFiles();
	}

	hw_enabled = 1; // enable hardware after first read...
      }

      MUTEX_SDCARD_GIVE;
    }

    // MSD driver
    if( msd_state != MSD_DISABLED ) {
      MUTEX_SDCARD_TAKE;

      switch( msd_state ) {
      case MSD_SHUTDOWN:
	// switch back to USB MIDI
	MIOS32_USB_Init(1);
	msd_state = MSD_DISABLED;
	break;

      case MSD_INIT:
	// LUN not mounted yet
	lun_available = 0;

	// enable MSD USB driver
	MUTEX_J16_TAKE;
	if( MSD_Init(0) >= 0 )
	  msd_state = MSD_READY;
	else
	  msd_state = MSD_SHUTDOWN;
	MUTEX_J16_GIVE;
	break;

      case MSD_READY:
	// service MSD USB driver
	MSD_Periodic_mS();

	// this mechanism shuts down the MSD driver if SD card has been unmounted by OS
	if( lun_available && !MSD_LUN_AvailableGet(0) )
	  msd_state = MSD_SHUTDOWN;
	else if( !lun_available && MSD_LUN_AvailableGet(0) )
	  lun_available = 1;
	break;
      }

      MUTEX_SDCARD_GIVE;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
//! This hook is called each mS from the main task which also handles DIN, ENC
//! and AIN events. You could add more jobs here, but they shouldn't consume
//! more than 300 uS to ensure the responsiveness of buttons, encoders, pots.
//! Alternatively you could create a dedicated task for application specific
//! jobs as explained in $MIOS32_PATH/apps/tutorials/006_rtos_tasks
/////////////////////////////////////////////////////////////////////////////
void APP_Tick(void)
{
  // toggle the status LED (this is a sign of life)
  MIOS32_BOARD_LED_Set(0x0001, ~MIOS32_BOARD_LED_Get());

//    // execute sequencer handler
//    MUTEX_SDCARD_TAKE;
//    SEQ_Handler();
//    MUTEX_SDCARD_GIVE;

//  // send timestamped MIDI events
//  MUTEX_MIDIOUT_TAKE;
//  SEQ_MIDI_OUT_Handler();
//  MUTEX_MIDIOUT_GIVE;

  if( hw_enabled ) {
    // Scan Matrix button handler
    MBNG_MATRIX_ButtonHandler();

    // update CV outputs
    MBNG_CV_Update();

    // update MAX72xx chain
    MBNG_MATRIX_MAX72xx_Update();

    // handle timed AIN events (sensor mode)
    MBNG_AIN_Periodic();

    // scan AINSER pins
    AINSER_Handler(APP_AINSER_NotifyChange);
    
    // MBFM time-critical things
    MBFM_1msTick(MIOS32_TIMESTAMP_Get());
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_Tick(void)
{
  MBNG_SEQ_Handler();
}


/////////////////////////////////////////////////////////////////////////////
//! This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // filter SysEx which is handled by separate parser
  if( !(midi_package.evnt0 < 0xf8 &&
       (midi_package.cin == 0xf ||
       (midi_package.cin >= 0x4 && midi_package.cin <= 0x7))) ) {

#if DEBUG_EVENT_HANDLER_PERFORMANCE
    MIOS32_STOPWATCH_Reset();
#endif

    // -> Event Handler
    MBNG_EVENT_MIDI_NotifyPackage(port, midi_package);

#if DEBUG_EVENT_HANDLER_PERFORMANCE
    u32 cycles = MIOS32_STOPWATCH_ValueGet();
    if( cycles == 0xffffffff )
      DEBUG_MSG("[PERF PCK:%08x] overrun!\n", midi_package.ALL);
    else
      DEBUG_MSG("[PERF PCK:%08x] %5d.%d mS\n", midi_package.ALL, cycles/10, cycles%10);
#endif
  }
    
  // -> MBFM Handler
  MBFM_ReceiveMIDIMessage(port, midi_package);

  // -> Motorfader Handler
  MBNG_MF_MIDI_NotifyPackage(port, midi_package);

  // -> MIDI Router
  MIDI_ROUTER_Receive(port, midi_package);

  // -> MIDI Port Handler (used for MIDI monitor function)
  MIDI_PORT_NotifyMIDIRx(port, midi_package);

  // forward to MIDI Monitor
  // SysEx messages have to be filtered for USB0 and UART0 to avoid data corruption
  // (the SysEx stream would interfere with monitor messages)
  u8 filter_sysex_message = (port == USB0) || (port == UART0);
  MIDIMON_Receive(port, midi_package, filter_sysex_message);
}

/////////////////////////////////////////////////////////////////////////////
//! This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // -> MBNG
  MBNG_SYSEX_Parser(port, midi_in);

#if DEBUG_EVENT_HANDLER_PERFORMANCE
    MIOS32_STOPWATCH_Reset();
#endif

  // -> Event router
  MBNG_EVENT_ReceiveSysEx(port, midi_in);

#if DEBUG_EVENT_HANDLER_PERFORMANCE
  u32 cycles = MIOS32_STOPWATCH_ValueGet();
  if( cycles == 0xffffffff )
    DEBUG_MSG("[PERF SYX:%02x] overrun!\n", midi_in);
  else
    DEBUG_MSG("[PERF SYX:%02x] %5d.%d mS\n", midi_in, cycles/10, cycles%10);
#endif

  // -> MF handler
  MBNG_MF_ReceiveSysEx(port, midi_in);

  // -> MIDI Router
  MIDI_ROUTER_ReceiveSysEx(port, midi_in);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
  // Matrix handler
  //MBNG_MATRIX_PrepareCol();
  // obsolete with MIOS32_SRIO based pages

  // keyboard handler
  KEYBOARD_SRIO_ServicePrepare();
}


/////////////////////////////////////////////////////////////////////////////
//! This hook is called after the shift register chain has been scanned
//! and before the DIN registers will be compared
//! (hook enabled via #define MIOS32_SRIO_CALLBACK_BEFORE_DIN_COMPARE in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinishBeforeDINCompare(void)
{
#if MBNG_PATCH_NUM_DIO > 0
  if( hw_enabled ) {
    u8 num_sr = MIOS32_SRIO_ScanNumGet();

    int port;
    for(port=0; port<MBNG_PATCH_NUM_DIO; ++port) {
      u8 sr;
      if( (sr=mbng_patch_dio_cfg[port].emu_sr) > 0 && sr <= num_sr ) {
	switch( mbng_patch_dio_cfg[port].mode ) {
	case MBNG_PATCH_DIO_CFG_MODE_DIN: {
	  // copy port value to specified DIN buffer
	  u8 value = MBNG_DIO_PortGet(port);
	  mios32_srio_din_buffer[sr-1] = value;
	} break;

	case MBNG_PATCH_DIO_CFG_MODE_DOUT: {
	  // copy specified DOUT to port
#if MIOS32_SRIO_NUM_DOUT_PAGES < 2
	  u8 value = mios32_dout_reverse_tab[mios32_srio_dout[0][num_sr-sr]];
#else
	  u8 value = mios32_dout_reverse_tab[mios32_srio_dout[mios32_srio_dout_page_ctr][num_sr-sr]];
#endif
	  MBNG_DIO_PortSet(port, value);
	} break;
	}
      }
    }
  }
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
  /*
  u8 skip_scs = 0;
#if MBNG_PATCH_NUM_DIO > 0
  {
    u8 sr;
    skip_scs = mbng_patch_dio_cfg[0].mode != MBNG_PATCH_DIO_CFG_MODE_Off && (sr=mbng_patch_dio_cfg[0].emu_sr) > 0 && sr <= MIOS32_SRIO_NUM_SR;
  }
#endif

  if( !skip_scs ) {
    // pass current pin state of J10
    // only available for LPC17xx, all others (like STM32) default to SRIO
    SCS_AllPinsSet(0xff00 | MIOS32_BOARD_J10_Get());

    // update encoders/buttons of SCS
    SCS_EncButtonUpdate_Tick();
  }
  */

  // Matrix handler
  MBNG_MATRIX_GetRow();

  // keyboard handler
  KEYBOARD_SRIO_ServiceFinish();

#if MIOS32_DONT_SERVICE_SRIO_SCAN
  // update encoder states
  MIOS32_ENC_UpdateStates();

  // standard SRIO scan has been disabled in programming_models/traditional/main.c via MIOS32_DONT_SERVICE_SRIO_SCAN in mios32_config.h
  // start the scan here - and retrigger it whenever it's finished
  APP_SRIO_ServicePrepare();
  MIOS32_SRIO_ScanStart(APP_SRIO_ServiceFinish);
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This hook is called when a button has been toggled
//! pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
  // -> MBNG_DIN once enabled
  if( hw_enabled )
    MBNG_DIN_NotifyToggle(pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
//! This hook is called when an encoder has been moved
//! incrementer is positive when encoder has been turned clockwise, else
//! it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  // pass encoder event to SCS handler
  //if( encoder == SCS_ENC_MENU_ID ) // == 0 (assigned by SCS)
  //  SCS_ENC_MENU_NotifyChange(incrementer);
  //else {
    // -> ENC handler
    MBNG_ENC_NotifyChange(encoder-1, incrementer);
  //}
}


/////////////////////////////////////////////////////////////////////////////
//! This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  // -> MBNG_AIN once enabled
  if( hw_enabled ) {
    MBNG_AIN_NotifyChange(pin, pin_value, 0); // no_midi==0
  }
}


/////////////////////////////////////////////////////////////////////////////
//! This hook is called when an AINSER pot has been moved
/////////////////////////////////////////////////////////////////////////////
static void APP_AINSER_NotifyChange(u32 module, u32 pin, u32 pin_value)
{
  // -> MBNG_AIN once enabled
  if( hw_enabled ) {
    MBNG_AINSER_NotifyChange(module, pin, pin_value, 0); // no_midi==0
  }
}


/////////////////////////////////////////////////////////////////////////////
//! Installed via MIOS32_MIDI_DirectRxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte)
{
  // filter MIDI In port which controls the MIDI clock
  if( MIDI_ROUTER_MIDIClockInGet(port) == 1 )
    SEQ_BPM_NotifyMIDIRx(midi_byte);

  return 0; // no error, no filtering
}

/////////////////////////////////////////////////////////////////////////////
//! Installed via MIOS32_MIDI_DirectTxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  return MIDI_PORT_NotifyMIDITx(port, package);
}

/////////////////////////////////////////////////////////////////////////////
//! Installed via MIOS32_MIDI_TimeoutCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port)
{  
  // forward to SysEx parser
  MBNG_SYSEX_TimeOut(port);

  // print message on screen
  //SCS_Msg(SCS_MSG_L, 2000, "MIDI Protocol", "TIMEOUT !!!");

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! MSD access functions
/////////////////////////////////////////////////////////////////////////////
s32 TASK_MSD_EnableSet(u8 enable)
{
  MIOS32_IRQ_Disable();
  if( msd_state == MSD_DISABLED && enable ) {
    msd_state = MSD_INIT;
  } else if( msd_state == MSD_READY && !enable )
    msd_state = MSD_SHUTDOWN;
  MIOS32_IRQ_Enable();

  return 0; // no error
}

s32 TASK_MSD_EnableGet()
{
  return (msd_state == MSD_READY) ? 1 : 0;
}

s32 TASK_MSD_FlagStrGet(char str[5])
{
  str[0] = MSD_CheckAvailable() ? 'U' : '-';
  str[1] = MSD_LUN_AvailableGet(0) ? 'M' : '-';
  str[2] = MSD_RdLEDGet(250) ? 'R' : '-';
  str[3] = MSD_WrLEDGet(250) ? 'W' : '-';
  str[4] = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! functions to access J16 semaphore
//! see also mios32_config.h
/////////////////////////////////////////////////////////////////////////////
void APP_J16SemaphoreTake(void)
{
  if( xJ16Semaphore != NULL && msd_state == MSD_DISABLED )
    MUTEX_J16_TAKE;
}

void APP_J16SemaphoreGive(void)
{
  if( xJ16Semaphore != NULL && msd_state == MSD_DISABLED )
    MUTEX_J16_GIVE;
}


/////////////////////////////////////////////////////////////////////////////
//! functions to access MIDI IN/Out Mutex
//! see also mios32_config.h
/////////////////////////////////////////////////////////////////////////////
void APP_MUTEX_MIDIOUT_Take(void) { MUTEX_MIDIOUT_TAKE; }
void APP_MUTEX_MIDIOUT_Give(void) { MUTEX_MIDIOUT_GIVE; }
void APP_MUTEX_MIDIIN_Take(void) { MUTEX_MIDIIN_TAKE; }
void APP_MUTEX_MIDIIN_Give(void) { MUTEX_MIDIIN_GIVE; }

//! \}
