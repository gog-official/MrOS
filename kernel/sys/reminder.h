// passive water reminder - it'll fire every 90 minutes of os-uptime
// Design of it:
// 	reminder_tick() is called from timer_irq_handler() every irq0
// 	It checks elapsed ticks against WATER_INTERVAL_TICKS.
// 	when the threshold is crossed it calls statusbar_set_msg().
// 	No thread, no separate timer, just a countrer check tbh
// 	existing 100Hz ticks : )
//
// Deisgn coice: why call IRQ context and not the shell loop?
// 	bcz the shell loop only runs when the user is actively typing.
// 	if the usr leaves the os in the idle state, the shell loop never iterates
// 	and a loop-based reminder would never fire, which we don't want.
// 	IRQ) firres without a condition. That's why we chose it.

#ifndef REMINDER_H
#define REMINDER_H

// 90 min x 60 sec x 100 ticks/sec
#define WATER_INTERVAL_TICKS (90 * 60 * 100)

// Uncomment this line for ovveriding interval to 10 seconds, for testing
// #define WATER_INTERVAL_TICKS (10 * 100)

void reminder_init(void);

//reminder_tick:
//called from timer_irq_handler() every tick.
//keep this FAST - it runs 100 times per seconds inside an IRQ.
//one integer comparision and a branch- that's it bruh

void reminder_tick(void);
void reminder_set_interval(unsigned int ticks);

#endif // !REMINDER_H
