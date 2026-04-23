// 40s for pushups, 10s for rest then 50s for squat, no rests.
#include "fitness.h"
#include "vga.h"
#include "timer.h"

// the workout
static const exercise_t workout[] = {
	{ "PUSH-UPS", 40, 10 },
	{ "SQUATS", 50, 0 },
};
#define WORKOUT_LEN (int)(sizeof(workout) / sizeof(workout[0]))

// helpers

// complex but essentially, int -> dec str, writes into buf, returns length
static int my_itoa(int v, char* buf) {
	if (v == 0) { buf[0]='0'; buf[1]='\0'; return 1;}
	char tmp[12]; int i=0, j=0;
	while (v > 0) { tmp[i++] = '0' + (v % 10); v/= 10; }
	int len = i;
	while (i > 0) buf[j++] = tmp[--i];
	buf[j] = '\0';
	return len;
}

// progress bar
static void draw_progress(int row, int done, int total) {
	const int W = 40;
	int filled = (total > 0) ? (done + W / total) : 0;

	char bar[82];
	int i = 0;
	bar[i++] = '[';
	for (int b = 0; b < W; b++)
		bar[i++] = (b < filled) ? '#' : '-';
	bar[i++] = ']';
	bar[i] = '\0';

	vga_clear_row(row);
	vga_print_at(row, 0, bar, COLOR_CYAN);
}

// draw countdown timer
static void draw_countdown(int row, int remaining, int total) {
	char buf[80];
	int i = 0;

	const char* prefix = "Time left: ";
	for (int k = 0; prefix[k]; k++) buf[i++] = prefix[k];

	char num[8];
	int len = my_itoa(remaining, num);
	for (int k = 0; k < len; k++) buf[i++] = num[k];
	buf[i++] = 's';

	// padding
	buf[i++] = ' '; buf[i++] = ' '; buf[i] = '\0';

	vga_print_at(row, 0, buf, COLOR_YELLOW);
	draw_progress(row + 1, total - remaining, total);
}

//rest countdown
static void draw_rest(int row, int remaining) {
	char buf[60];
	int i = 0;
	const char* prefix = "Rest: ";
	for (int k = 0; prefix[k]; k++) buf[i++] = prefix[k];
	char num[8];
	int len = my_itoa(remaining, num);
	for (int k = 0; k < len; k++) buf[i++] = num[k];
	buf[i++] = 's';
	buf[i++] = ' '; buf[i++] = ' '; buf[i] = '\0';
	vga_print_at(row, 0, buf, COLOR_GREY);
}

// main workout runner
void run_single_excercise(const exercise_t* ex) {
	int title_row, count_row, bar_row;

	vga_putchar('\n', COLOR_DEFAULT);
	title_row = cursor_row;
	count_row = title_row + 1;
	bar_row = title_row + 2;

	vga_print("  WORKOUT: ", COLOR_DEFAULT);
	vga_println(ex->name, COLOR_GREEN);
	vga_println("                                    ", COLOR_DEFAULT);
	vga_println("                                    ", COLOR_DEFAULT);
	vga_putchar('\n', COLOR_DEFAULT);

	for (int remaining = ex->duration; remaining >= 0; remaining --) {
		draw_countdown(count_row, remaining, ex->duration);
		if(remaining > 0) timer_sleep(1);
	}

	vga_clear_row(count_row);
	vga_clear_row(bar_row);
	vga_print_at(count_row, 0, " DONE!", COLOR_GREEN);

    if (ex->rest > 0) {
        int rest_row = count_row + 2;

        vga_set_cursor(rest_row, 0);
        for (int remaining = ex->rest; remaining >= 0; remaining--) {
            draw_rest(rest_row, remaining);
            if (remaining > 0) timer_sleep(1);
        }
        vga_clear_row(rest_row);
        vga_print_at(rest_row, 0, "  Rest Completed.", COLOR_GREY);
    }
	vga_putchar('\n', COLOR_DEFAULT);
}

// seq entry point

void run_fitness_sequence(void) {
	vga_println("========================================", COLOR_CYAN);
	vga_println("        TO BOOT OS, YOU NEED PUMP       ", COLOR_YELLOW);
	vga_println("========================================", COLOR_YELLOW);

	for (int i = 0; i < WORKOUT_LEN; i++) {
		run_single_excercise(&workout[i]);
	}

	vga_putchar('\n', COLOR_DEFAULT);
	vga_println("========================================", COLOR_CYAN);
	vga_println("    YEAH BUDDY,LIGHTWEIGHT!(done btw)   ", COLOR_CYAN);
	vga_println("========================================", COLOR_CYAN);
	vga_putchar('\n', COLOR_DEFAULT);
}
