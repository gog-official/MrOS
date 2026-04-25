// pc speaker driver
// hardware path:
// PIT channel 2 (port 0x42, command 0x43) -> and gate -> speaker
// and gate < port 0x61 bits 0-1
//
// port 0x61 (system control port B):
//  bit 0 = pit channel 2 gate enable (1 = let pit drive speaker)
//  bit 1 = speaker data enable (1 = connect pit to speaker)
//  Both bits must be 1 for sound to come out.
//  Clear both to silence.
//
//  hold up, pit channel 2 works exactly like channel 0(our timmie) except for these stuffs
//  1. not triggering irq
//  2. output driving the speaker AND gate
//  3. we program it independently on port 0x42
//
//  frequency math :(
//  pit base clock = 1,193,180 hz
//  divisor = 1193180 / desired_hz
//  actual hz = 1193180 / divisor
//
//  eg: 440hz(C-A) -> divisor = 1193180 / 440 = 2712
//  	actual = 1193180 /2712 = 439.98(0.02 diff) ultra close
//
//  musical note frequencies are provided as constant so callers can play melodies without looking up values :)
#ifndef SPEAKER_H
#define SPEAKER_H

#include <stdint.h>

// IO PORTs
#define SPEAKER_PORT 0x61 // sys control port B
#define PIT_CHANNEL2_PORT 0x42 // PIT channel 2 data
#define PIT_CMD_PORT // pit command register

//now, notes(hz)

//octave 4
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494

//octave 5
#define NOTE_C5 523
#define NOTE_D5
#define NOTE_E5
#define NOTE_F5
#define NOTE_G5
#define NOTE_A5
#define NOTE_B5 988

//octave 3

#endif // !SPEAKER_H
