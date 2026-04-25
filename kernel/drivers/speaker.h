// pc speaker driver - read below to understand wth is going on
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
#define PIT_CMD_PORT 0x43 // pit command register

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
#define NOTE_D5	587
#define NOTE_E5	659
#define NOTE_F5	698
#define NOTE_G5	784
#define NOTE_A5	880
#define NOTE_B5 988

//octave 3
#define NOTE_C3 131
#define NOTE_G3 196
#define NOTE_A3 220

//special
#define NOTE_REST 0 // every melody requires a silence. -GuruOrGoru(4/25/2026)

// duration constants
// based on 120 BPM
#define DUR_WHOLE 2000
#define DUR_HALF 1000
#define DUR_QUARTER 500
#define DUR_EIGHT 250
#define DUR_SIXTEENTH 125 // had to recheck the spelling lol

//melody note :)
//used to define melodies as arr

typedef struct {
	uint32_t freq_hz; // 0 for peace(silence)
	uint32_t duration_ms;
} melody_note_t;

// usage:
// 	melody_note_t happy[] = {
// 	{NOTE_C4, DUR_Quater},
// 	...,
// 	{0,0}
// 	};
// 	speaker_play_melody();  see this below 

//api
void speaker_init(void);

void speaker_on(uint32_t freq_hz);

void speaker_off(void);

void make_sound(uint32_t freq_hz, uint32_t duration_ms);

void speaker_play_melody(const melody_note_t* notes); // back to back

// prebuilt stuffs
void sfx_boot(void); // start jingling
void sfx_workout_start(void); // workout pump
void sfx_workout_done(void); // muscle soreness
void sfx_rest_start(void); // peace
void sfx_workoutsync_done(void); // in 7th cloud
void sfx_reminder(void); // slurp
void sfx_error(void); //oopsie
void sfx_shell_ready(void); //3..2..1..go


#endif // !SPEAKER_H
