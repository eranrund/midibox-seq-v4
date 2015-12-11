// $Id: seq.c 687 2009-08-03 20:48:54Z tk $
/*
 * Sequencer Routines
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
#include <seq_bpm.h>
#include <seq_midi_out.h>
#include <notestack.h>

#include "seq.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NOTESTACK_SIZE 16


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_PlayOffEvents(void);
static s32 SEQ_Tick(u32 bpm_tick);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// the sequence (global: external functions can display/modify the entries)

// note values in hex:
// A-3: 0x45
// G-3: 0x41
// D-2: 0x3e
// A-2: 0x3a
// no note: 0x00
u8 seq_steps_note[16] = {
  0x3c, 0x48, 0x43, 0x00, // C-3  C-4  G-3  ---
  0x3c, 0x48, 0x40, 0x43, // C-3  C-4  E-3  G-3
  0x3c, 0x48, 0x43, 0x00, // C-3  C-4  G-3  ---
  0x3c, 0x40, 0x48, 0x43  // C-3  E-3  C-4  G-3
};

// velocity values in hex 
// (0x01..0x7f - 0x00 won't play the note (since it would result into Note Off event))
u8 seq_steps_velocity[16] = {
  0x60, 0x70, 0x78, 0x00,
  0x60, 0x70, 0x78, 0x70,
  0x60, 0x70, 0x78, 0x00,
  0x60, 0x70, 0x78, 0x70
};

// lengths in decimal
// at 384 ppqn, a 16th note takes 96 ticks, we play 75%
u8 seq_steps_length[16] = {
    72,   72,  168,   0, // |||-|||-|||-|||||||-
    72,   72,   72,  72, // |||-|||-|||-|||-|||-
    72,   72,  168,   0, // |||-|||-|||-|||||||-
    72,   72,   72,  72  // |||-|||-|||-|||-|||-
};


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// the pattern position
static u8 seq_step_pos;

// pause mode (will be controlled from user interface)
static u8 seq_pause = 0;

// transpose incrementer
static s8 transpose; // should be signed!

// notestack
static notestack_t notestack;
static notestack_item_t notestack_items[NOTESTACK_SIZE];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_Init(u32 mode)
{
  // initialize the Notestack
  NOTESTACK_Init(&notestack, NOTESTACK_MODE_PUSH_TOP, &notestack_items[0], NOTESTACK_SIZE);
  transpose = 0;

  // reset sequencer
  SEQ_Reset();

  // init BPM generator
  SEQ_BPM_Init(0);

  SEQ_BPM_PPQN_Set(384);
  SEQ_BPM_Set(120.0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// this sequencer handler is called periodically to check for new requests
// from BPM generator
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_Handler(void)
{
  // handle requests

  u8 num_loops = 0;
  u8 again = 0;
  do {
    ++num_loops;

    // note: don't remove any request check - clocks won't be propagated
    // so long any Stop/Cont/Start/SongPos event hasn't been flagged to the sequencer
    if( SEQ_BPM_ChkReqStop() ) {
      SEQ_PlayOffEvents();
    }

    if( SEQ_BPM_ChkReqCont() ) {
      // release pause mode
      seq_pause = 0;
    }

    if( SEQ_BPM_ChkReqStart() ) {
      SEQ_Reset();
    }

    u16 new_song_pos;
    if( SEQ_BPM_ChkReqSongPos(&new_song_pos) ) {
      SEQ_PlayOffEvents();
    }

    u32 bpm_tick;
    if( SEQ_BPM_ChkReqClk(&bpm_tick) > 0 ) {
      again = 1; // check all requests again after execution of this part

      SEQ_Tick(bpm_tick);
    }
  } while( again && num_loops < 10 );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function plays all "off" events
// Should be called on sequencer reset/restart/pause to avoid hanging notes
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_PlayOffEvents(void)
{
  // play "off events"
  SEQ_MIDI_OUT_FlushQueue();

  // here you could send additional events, e.g. "All Notes Off" CC

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets song position of sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_Reset(void)
{
  // since timebase has been changed, ensure that Off-Events are played 
  // (otherwise they will be played much later...)
  SEQ_PlayOffEvents();

  // release pause mode
  seq_pause = 0;

  // reset BPM tick
  SEQ_BPM_TickSet(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// performs a single bpm tick
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_Tick(u32 bpm_tick)
{
  // whenever we reach a new 16th note (96 ticks @384 ppqn):
  if( (bpm_tick % (SEQ_BPM_PPQN_Get()/4)) == 0 ) {
    // ensure that step number will be reseted on first bpm_tick
    if( bpm_tick == 0 )
      seq_step_pos = 0;
    else {
      // increment step number until it reaches 16
      if( ++seq_step_pos >= 16 )
	seq_step_pos = 0;
    }

    // get note/velocity/length from array
    int note      = seq_steps_note[seq_step_pos];
    u8  velocity  = seq_steps_velocity[seq_step_pos];
    u8  length    = seq_steps_length[seq_step_pos];

    // here you could modify the values, e.g. transposing or dynamic length variation

    // simple transpose:
    note += transpose;
    while( note < 0 )
      note += 12;
    while( note > 128 )
      note -= 12;
      

    // put note into queue if all values are != 0
    if( note && velocity && length ) {
      mios32_midi_package_t midi_package;
      midi_package.type     = NoteOn; // package type must match with event!
      midi_package.event    = NoteOn;
      midi_package.chn      = Chn1;
      midi_package.note     = note;
      midi_package.velocity = velocity;

      SEQ_MIDI_OUT_Send(DEFAULT, midi_package, SEQ_MIDI_OUT_OnOffEvent, bpm_tick, length);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Should be called whenever a Note event has been received.
// We expect, that velocity is 0 on a Note Off event
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_NotifyNoteOn(u8 note, u8 velocity)
{
  if( velocity )
    // push note into note stack
    NOTESTACK_Push(&notestack, note, velocity);
  else
    // remove note from note stack
    NOTESTACK_Pop(&notestack, note);

  // At least one note played?
  if( notestack.len > 0 ) {
    // start sequencer if it isn't already running
    if( !SEQ_BPM_IsRunning() )
      SEQ_BPM_Start();

    // set new transpose note
    transpose = notestack_items[0].note - 0x3c; // C-3 is middle note
  } else {
    // no key is pressed anymore: stop sequencer
    SEQ_BPM_Stop();
  }


#if 1
  // optional debug messages
  NOTESTACK_SendDebugMessage(&notestack);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Should be called whenever a Note Off event has been received.
// forwards to SEQ_NotifyNoteOn with velocity 0
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_NotifyNoteOff(u8 note)
{
  return SEQ_NotifyNoteOn(note, 0);
}
