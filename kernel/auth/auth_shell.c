//todo:
// 	whoami
// 	users
// 	useradd
// 	userdel
// 	passwd
// 	logout
#include "auth.h"
#include "user.h"
#include "../core/vga.h"
#include "../drivers/keyboard.h"
#include "../lib/string.h"

static void ap_read_password(char* buf, int max) {
	extern int cursor_col;
	int pos = 0;
	while (1) {
		extern char keyboard_getchar(void);
		char c = keyboard_getchar();
		if (c == '\n') { buf[pos] = '\0'; vga_putchar('\n', COLOR_DEFAULT); return; }
		if (c == '\b') {
			if (pos > 0) {
				pos--;
				if (cursor_col > 0) { cursor_col--; vga_putchar(' ', COLOR_DEFAULT); cursor_col--; }
			}
			continue;
		}
		if (c < 0x20 || c > 0x7E || pos >= max - 1) continue;
		buf[pos++] = c;
		vga_putchar('*', COLOR_DEFAULT);
	}
}

//whoami
void cmd_whoami(int argc, char** argv) {
	(void)argc; (void)argv;
	vga_print("  ", COLOR_DEFAULT);
	vga_print(current_session.username, COLOR_YELLOW);
	if (current_session.flags == USER_FLAG_ADMIN) {
		vga_print(" (Admin)", COLOR_CYAN);
	}
	vga_println("", COLOR_DEFAULT);

	char home_display[32];
	strcpy(home_display, "home/");

	char tmp[9];
	strcpy(tmp, current_session.home_dir + 1);
	for (int i = 0; tmp[i]; i++)
		if (tmp[i] >= 'A' && tmp[i] <= 'Z') tmp[i] += 32;
	strcat(home_display, tmp);

	vga_print("  Home: ", COLOR_GREY);
	vga_println(home_display, COLOR_DEFAULT);

	vga_print("  Home: ", COLOR_GREY);
	vga_println(home_display, COLOR_DEFAULT);
}

// users (admin only)
void cmd_users(int argc, char** argv) {
	(void)argc; (void)argv;

	if (current_session.flags != USER_FLAG_ADMIN) {
		vga_println("users: permission denied (admin only), admin needs to be buffffed", COLOR_RED);
		return;
	}

	static user_t list[USER_MAX_COUNT];
	int count = user_get_all(list, USER_MAX_COUNT);

	vga_println("", COLOR_DEFAULT);
	vga_println("  Username		Role 	Home", COLOR_CYAN);
	vga_println("  --------------	------	--------", COLOR_GREY);

	for (int i = 0; i < count; i ++) {
		vga_print("  ", COLOR_DEFAULT);

		int namelen = strlen(list[i].username);
		vga_print(list[i].username, COLOR_YELLOW);
		for (int p = namelen; p < 17; p++) vga_putchar(' ', COLOR_DEFAULT);

		if (list[i].flags == USER_FLAG_ADMIN) {
			vga_print("admim   ", COLOR_CYAN);
		} else {
			vga_print("user    ", COLOR_GREY);
		}

		char home[9];
		auth_home_dir(list[i].username, home);
		char homelower[9];
		for (int j = 0; home[j+1]; j++) {
			homelower[j] = home[j+1];
			if (homelower[j] >= 'A' && homelower[j] <= 'Z') homelower[j] += 32;
		}
		homelower[strlen(home)-1] = '\0';
		vga_println(homelower, COLOR_DEFAULT);
	}
	vga_println("", COLOR_DEFAULT);
}

//useradd - admin
void cmd_useradd(int argc, char** argv) {
	if (current_session.flags != USER_FLAG_ADMIN) {
		vga_println("useradd: permission denied(admin only, they're buff unlike you-skinny fat lol)", COLOR_RED);
		return;
	}
	if (argc < 2) {
		vga_println("usage: useradd <username>", COLOR_YELLOW);
		return;
	}

	char password[USER_MAX_PASS + 1];
	char confirm [USER_MAX_PASS + 1];

	vga_println("", COLOR_DEFAULT);

	while (1) {
		vga_print("  Password for '", COLOR_DEFAULT);
		vga_print(argv[1], COLOR_YELLOW);
		vga_print("': ", COLOR_DEFAULT);
		ap_read_password(password, USER_MAX_PASS + 1);

		if (strlen(password) < 4) {
			vga_println("  Passwords must be at least 4 chars, try iloveglutes as one", COLOR_RED);
			continue;
		}
		vga_print("  Confirm	: ", COLOR_DEFAULT);
		ap_read_password(confirm, USER_MAX_PASS + 1);

		if (strcmp(password, confirm) != 0) {
			vga_println("  Passwords do not match. js try iloveglutes once", COLOR_RED);
			continue;
		}
		break;
	}
	int r = user_create(argv[1], password, USER_FLAG_NORMAL);
	vga_println("", COLOR_DEFAULT);
	switch (r) {
		case 0: {
				vga_print("  User '", COLOR_GREEN);
				vga_print(argv[1], COLOR_YELLOW);
				vga_print("' created. Home: home/", )
			}
	}
}
