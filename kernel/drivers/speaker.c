// timing note:
// 	make_sound uses timer_sleep_ticks.
// 	1 tick = 10ms at 100Hz
// 	duration_ms / 10 = ticks
// 	minimum resolution is 10ms - duratioons below that are rounded to 0(little quickie)

#include "speaker.h"
#include "timer.h"
#include "../interrupts/pic.h" // for outb/inb

// low level hardware

// here's a revision for you
// we will program PIT channel 2 to generate a square wave at freq_hz.
//
// PIT command byte for channel 2:
// 	bits 7-6 = 10  - channel 2
// 	bits 5-4 = 11  - access mode lobyte/hibyte
// 	bits 3-1 = 011 - mode 3 (square wave)
// 	bit 0 = 0 - binary not BCD
// 	= 1011 0110 = 0xB6, so yeah we'll use it

static void pit_set_channel2(uint32_t freq_hz) {
	if (freq_hz == 0) return;

	uint32_t divisor = 1193180 / freq_hz;

	// clamp: divisor must tightly fit in 16 bits(max ~18hz), min 1
	if (divisor > 0xFFFF) divisor = 0xFFFF;
	if (divisor < 1) divisor = 1;

	// send command then divisor lo/hi
	outb(PIT_CMD_PORT, 0xB6);
	outb(PIT_CHANNEL2_PORT, (uint8_t)(divisor & 0xFF));
	outb(PIT_CHANNEL2_PORT, (uint8_t)((divisor >> 8) & 0xFF));

}

// public api
void speaker_init(void) {
	speaker_off();
}

void speaker_on(uint32_t freq_hz) {
	if (freq_hz == 0) {
		speaker_off();
		return;
	}

	pit_set_channel2(freq_hz);
	// enable speaker:
	// 	read port 0x61, set bits 0 and 1, write back
	// 	we use read-modify-write so that we won't mess with other bits
	// 	in port 0x61
	uint8_t tmp = inb(SPEAKER_PORT);
	outb(SPEAKER_PORT, tmp | 0x03); // essentially what i was talking about, set bits 0 and 1
}

void speaker_off(void) {
	// again, clear bits 0 and 1, read-modify-write again
	uint8_t tmp = inb(SPEAKER_PORT);
	outb(SPEAKER_PORT, tmp & ~0x03);
}

void make_sound(uint32_t freq_hz, uint32_t duration_ms) {
	// firstly, convert ms to ticks, minimum 1 tick so very short sounds still produce a clickky
	uint32_t ticks = duration_ms / 10;
	if (ticks == 0) ticks = 1;
	if (freq_hz == 0) {
		speaker_off();  // ensure speaker is off for silence
		timer_sleep_ticks(ticks); // waiting
		return;
	}

	speaker_on(freq_hz);
	timer_sleep_ticks(ticks);
	speaker_off();
}

void speaker_play_melody(const melody_note_t* notes) {
	for (int i = 0; notes[i].duration_ms != 0 || notes[i].freq_hz != 0; i++) {
		make_sound(notes[i].freq_hz, notes[i].duration_ms);

		// without the gap below, consecutive same-frequency notes blur together
		// and sound like one long tone.
		if (notes[i+1].freq_hz != 0 || notes[i+1].duration_ms != 0) {
			speaker_off();
			timer_sleep_ticks(2);
		}
	}
	speaker_off();
}

// sound effects: each short melody array is played v,ia speakers play function or direct sequence of making sound calls.

//this will play at startup after timer init
void sfx_boot(void) {
	static const melody_note_t seq[] = {
		{ NOTE_C4, DUR_EIGHT },
		{ NOTE_E4, DUR_EIGHT },
		{ NOTE_G4, DUR_EIGHT },
		{ NOTE_C5, DUR_QUARTER }, // ascending arpeggio = C E G C5
		{ 0, 0 }
	};
	speaker_play_melody(seq);
}

void sfx_workout_start(void) {
	make_sound(NOTE_G4, DUR_EIGHT);
	make_sound(NOTE_C5, DUR_EIGHT); // quick ascending beeps
}

void sfx_workout_done(void) {
	make_sound(NOTE_E5, DUR_EIGHT);
	make_sound(NOTE_C5, DUR_QUARTER);
}

void sfx_rest_start(void) {
	make_sound(NOTE_G3, DUR_QUARTER); // calmness
}

void sfx_workoutsync_done(void) {
	static const melody_note_t seq[] = {
		{ NOTE_C4, DUR_EIGHT },
		{ NOTE_E4, DUR_EIGHT },
		{ NOTE_G4, DUR_EIGHT },
		{ NOTE_E4, DUR_EIGHT },
		{ NOTE_C5, DUR_QUARTER },
		{ NOTE_REST, DUR_EIGHT },
		{ NOTE_G5, DUR_HALF }, // victory fanfare
		{ 0, 0 }
	};
	speaker_play_melody(seq);
}

void sfx_reminder(void) {
	make_sound(NOTE_A5, DUR_EIGHT);
	make_sound(NOTE_REST, 50);
	make_sound(NOTE_A5, DUR_EIGHT);
	make_sound(NOTE_REST, 50);
	make_sound(NOTE_A5, DUR_EIGHT);
}

void sfx_error(void) {
	make_sound(NOTE_A3, DUR_EIGHT);
	make_sound(NOTE_G3, DUR_EIGHT);
}

void sfx_shell_ready(void) {
	make_sound(NOTE_C5, DUR_SIXTEENTH);
}
