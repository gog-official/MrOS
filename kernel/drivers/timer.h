#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43

#define PIT_BASE_FREQ 1193180
#define TIMER_FREQ_HZ 100
#define PIT_DIVISOR (PIT_BASE_FREQ/TIMER_FREQ_HZ)

#define TICKS_PER_SEC TIMER_FREQ_HZ
#define TICKS_PER_MIN (TICKS_PER_SEC * 60)

void timer_init(void);
uint32_t timer_get_ticks(void);
void timer_sleep(uint32_t seconds);
void timer_sleep_ticks(uint32_t ticks);

void  timer_irq_handler(void);

#endif // !TIMER_H
