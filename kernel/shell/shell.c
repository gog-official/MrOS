// interactive command shell
#include "shell.h"
#include "../drivers/keyboard.h"
#include "../interrupts/pic.h"
#include "../core/vga.h"
#include "../drivers/timer.h"
#include "../fitness/fitness.h"

//str helpers

static int k_strcmp(const char* a, const char* b) {
	while (*a && (*a == *b)) { a++; b++; }
	return (unsigned char) *a - (unsigned char)*b;
}

static int k_strlen(const char* s) {
	int n = 0;
	while (s[n]) n++;
	return n;
}

static void k_itoa(int v, char* buf) {
	if ( v == 0 ) { buf[0]='0'; buf[1]='\0'; return; }
	char tmp[12]; int i=0, j=0;
	int neg = 0;
	if ( v < 0) {neg=1; v=-v;}
	while (v > 0) { tmp[i++]='0'+(v%10);v/=10;}
	if (neg) buf[j++]='-';
	while (i>0) buf[j++]= tmp[--i];
	buf[j]='\0';
}

// command parser
static void cmd_parse(char* line, char* argv[], int* argc) {
	*argc = 0;
	char* p = line;

	while (*p) {
		while (*p == ' ') p++;
		if (*p == '\0') break;

		if (*argc < SHELL_ARGS_MAX) {
			argv[(*argc)++] = p;
		}

		while (*p && *p != ' ') p++;

		if (*p == ' ') { *p = '\0'; p++; }
	}
}

//command handlers
static void cmd_help(int argc, char** argv);

static void cmd_clear(int argc, char** argv) {
	(void)argc; (void)argv;
	vga_clear();
}

// echo
static void cmd_echo(int argc, char** argv) {
	for (int i = 1; i < argc; i++) {
		vga_print(argv[i], COLOR_DEFAULT);
		if (i < argc - 1) vga_putchar(' ', COLOR_DEFAULT);
	}
	vga_putchar('\n', COLOR_DEFAULT);
}

// uptime
static void cmd_uptime(int argc, char** argv) {
	(void)argc; (void)argv;

	uint32_t ticks = timer_get_ticks();
	uint32_t seconds = ticks / 100;
	uint32_t minutes = seconds / 60;
	uint32_t hours = minutes / 60;
	seconds %= 60;
	minutes %= 60;

	char buf[12];

	vga_print("Uptime: ", COLOR_CYAN);
	if (hours > 0) {
		k_itoa((int)hours, buf);
		vga_print(buf, COLOR_DEFAULT);
		vga_print("h ", COLOR_DEFAULT);
	}

	k_itoa((int)minutes, buf);
	vga_print(buf, COLOR_DEFAULT);
	vga_print("m ", COLOR_DEFAULT);

	k_itoa((int)seconds, buf);
	vga_print(buf, COLOR_DEFAULT);
	vga_println("s", COLOR_DEFAULT);
}

// about
static void cmd_about(int argc, char** argv) {
	(void)argc; (void)argv;
	vga_println("", COLOR_DEFAULT);
	vga_println("   MrOS - OS but also a motivation", COLOR_CYAN);
	vga_println("   Built with NASM + GCC (no libc)", COLOR_GREY);
	vga_println("   By a beginner trying to learn low level programming", COLOR_GREY);
	vga_println("", COLOR_DEFAULT);
}

// reboot
static void cmd_reboot(int argc, char** argv) {
	(void)argc; (void)argv;
	vga_println("Initiating a reboot for you:)....", COLOR_YELLOW);

	uint8_t status;
	do { status = inb(0x64); } while (status & 0x02);
	outb(0x64, 0xFE);

	// if outb didnt reboot uus
	for (;;) __asm__ volatile ("hlt");
}

//workout, lezgoo
static void cmd_workout(int argc, char**argv) {
	(void)argc; (void)argv;
	vga_clear();
	run_fitness_sequence();
	vga_clear();
}

//custom workout
static void cmd_exercise(int argc, char** argv) {
	if (argc < 3) {
		vga_println("usage: exercise <name> <time in seconds>", COLOR_YELLOW);
		vga_println("   e.g.: exercise BURPEE 30", COLOR_GREY);
		return;
	}

	int secs = 0;
	for (int i = 0; argv[2][i] >= '0' && argv[2][i] <= '9'; i++) {
		secs = secs * 10 + (argv[2][i] - '0');
	}
	if (secs <= 0 || secs > 3600) {
		vga_println("Seconds must be 1-3600", COLOR_RED);
		return;
	}

	exercise_t ex;
	ex.name = argv[1];
	ex.duration = secs;
	ex.rest = 0;
	
	vga_clear();
	run_single_excercise(&ex);
	vga_clear();
}

// stats
static int workouts_completed = 0;
static void cmd_stats(int argc, char** argv) {
	(void)argc; (void)argv;

	uint32_t ticks = timer_get_ticks();
	uint32_t seconds = ticks / 100;

	char buf[12];
	vga_println("", COLOR_DEFAULT);
	vga_println("  -- Session stats --", COLOR_CYAN);

	vga_print("  Workouts this session : ", COLOR_DEFAULT);
	k_itoa(workouts_completed, buf);
	vga_println(buf, COLOR_YELLOW);

	vga_print("  OS uptime             : ", COLOR_DEFAULT);
	k_itoa((int)seconds, buf);
	vga_print(buf, COLOR_YELLOW);
	vga_println("s", COLOR_YELLOW);

	vga_println("", COLOR_DEFAULT);
}

// command table
typedef struct {
	const char* name;
	void (*fn)(int argc, char** argv);
	const char* desc;
} command_t;

static const command_t commands[] = {
	// normal
	{ "help", cmd_help, "list all commands" },
	{ "clear", cmd_clear, "clear the screen" },
	{ "echo", cmd_echo, "echo [text]  - print text" },
	{ "uptime", cmd_uptime, "show time since boot" },
	{ "about", cmd_about, "shows OS info" },
	{ "reboot", cmd_reboot, "reboots the machine" },
	//fitness
	{ "workout", cmd_workout, "run the full workout sequence" },
	{ "exercise", cmd_exercise, "exercose <name> <secs> - one set" },
	{ "stats", cmd_stats, "show session stats" },
	//sentinel
	{ 0, 0, 0 }
};

// help
static void cmd_help(int argc, char** argv) {
	(void)argc; (void)argv;
	vga_println("", COLOR_DEFAULT);
	vga_println("  Available commands:", COLOR_CYAN);
	vga_println("  -------------------", COLOR_GREY);
	for (int i = 0; commands[i].name != 0; i ++) {
		vga_print ("  ", COLOR_DEFAULT);
		int namelen = k_strlen(commands[i].name);
		vga_print(commands[i].name, COLOR_YELLOW);
		for (int p = namelen; p < 12; p++) vga_putchar(' ', COLOR_DEFAULT);
		vga_println(commands[i].desc, COLOR_GREY);
	}
	vga_println("", COLOR_DEFAULT);
}

// dispatcher
static void dispatch(int argc, char** argv) {
	if (argc == 0) return;

	for (int i = 0; commands[i].name != 0; i++) {
		if (k_strcmp(argv[0], commands[i].name)==0) {
			commands[i].fn(argc, argv);
			return;
		}
	}

	// unknown command
	vga_print("Oopsie :(, Unknown command: ", COLOR_RED);
	vga_println(argv[0], COLOR_DEFAULT);
	vga_println("Respectfully type 'help' for a list of commands", COLOR_GREY);
}

// finnaly main loop

void shell_run(void) {
	char line[SHELL_LINE_MAX];
	char* argv[SHELL_ARGS_MAX];
	int argc;

	vga_println("****************************************************", COLOR_YELLOW);
	vga_println("     Welcome to MrOs, aware of water reminders??    ", COLOR_CYAN);
	vga_println("            Type 'help' for commands ;)             ", COLOR_GREY);
	vga_println("****************************************************", COLOR_YELLOW);
	vga_putchar('\n', COLOR_DEFAULT);

	while (1) {
		vga_print("#Mr> ", COLOR_GREEN);

		keyboard_readline(line, SHELL_LINE_MAX);
		cmd_parse(line, argv, &argc);
		dispatch(argc, argv);
	}
	// no one reaches here
}
