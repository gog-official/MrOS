#include "reminder.h"
#include "statusbar.h"
#include "../drivers/timer.h"
#include "../core/vga.h"

//last_reminder_tick:
//Initialised to Water_interva_ticks so the FIRST reminder booms
//exactly 90 mins after the boot, not immediately.
// 
// Aight, so if we initialised to 0, the first fire would happen at tick
// water_interval_ticks which is still 90 min - essssentially same thing.
// setting it this way makes the logic easier to reason aout:
// "fire when (now - last) >= interval".

static unsigned int last_reminder_tick = 0;

// track whether we showed the initial boot value
static int initialised = 0;

void reminder_init(void) {
	// set last_reminder_tick to 'now' so the first reminder fires
	// 90 mins froom boot, not from time 0
	last_reminder_tick = timer_get_ticks();
	initialised = 1;
}

// changes the reminder interval to user defined time
static unsigned int water_interval_ticks;
void reminder_set_interval(unsigned int ticks) {
	if (ticks > 0) {
		water_interval_ticks = ticks;
	}

	if (water_interval_ticks <= 0) {
		water_interval_ticks = WATER_INTERVAL_TICKS;
	}
}

//reminder_tick;
//its called e very irq0 (100hz) from timer_irq_handler()
// we do not call timer_get_ticks() here, it exists but
// calling it from irq context is fine since it just reads the volatile counter
// However, hold up, since we r inside the handler when ticks is being incremented
// we use the value directly via the extern.
//
// ackshually we'll just call timer_get_ticks(), it is a simple read of a volatile uint32_t, perfectly safe from irq context.

void reminder_tick(void) {
	if (!initialised) return;

	unsigned int now = timer_get_ticks();

	if ((now - last_reminder_tick) >= water_interval_ticks) {
		last_reminder_tick = now;

		// statusbar_set_msg writes directly to vga memory. safe from IRQ
		// context- no malloc, no blocking, no scroll. what a pride :D
		statusbar_set_msg("DRINK WATER! Stay hydrated.", COLOR_CYAN);
	}
}
