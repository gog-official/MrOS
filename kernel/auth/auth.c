#include "auth.h"
#include "user.h"
#include "../crypto/hash.h"
#include "../core/vga.h"
#include "../drivers/keyboard.h"
#include "../drivers/timer.h"
#include "../lib/string.h"
#include "../fs/vfs.h"
#include <stdlib.h>

session_t current_session;

// helpers
static void read_password(char* buf, int max) {
	int pos = 0;
	while (1) {
		char c = keyboard_getchar();
		if (c == '\n') {
			buf[pos] = '\0';
			vga_putchar('\n', COLOR_DEFAULT);
			return;
		}
		if (c == '\b') {
			if (pos > 0) {
				pos--;
				if (cursor_col > 0) {
					cursor_col--;
					vga_putchar(' ', COLOR_DEFAULT);
					cursor_col--;
				}
			}
			continue;
		}
		if (c < 0x20 || c > 0x7E) continue;
		if (pos >= max - 1) continue;
		buf[pos++] = c;
		vga_putchar('*', COLOR_DEFAULT);
	}
}

static void draw_login_banner(void) {
	vga_clear();
	vga_println("", COLOR_DEFAULT);
	vga_println(" ==========================================", COLOR_CYAN);
	vga_println("                 MrOS Login                ", COLOR_YELLOW);
	vga_println(" ==========================================", COLOR_CYAN);
	vga_println("", COLOR_DEFAULT);
}

// home dir calculation
void auth_home_dir(const char* username, char* out) {
	out[0] = 'H';
	int i = 1;
	for (int j = 0; username[j] && i < 8; j++, i++) {
		char c = username[j];
		if (c >= 'a' && c <= 'z') c -= 32;
		out[i] = c;
	}
	out[i] = '\0';
}

// first boot stup
static void first_boot_setup(void) {
	vga_clear();
	vga_println("", COLOR_DEFAULT);
	vga_println(" ==========================================", COLOR_CYAN);
	vga_println("              MrOs - Boot setup            ", COLOR_YELLOW);
	vga_println(" ==========================================", COLOR_CYAN);
	vga_println("", COLOR_DEFAULT);
	vga_println(" No users found. Create the first account", COLOR_GREY);
	vga_println(" This account will have admin privilages", COLOR_GREY);
	vga_println("", COLOR_DEFAULT);

	char username[USER_MAX_NAME + 1];
	char password[USER_MAX_PASS + 1];
	char comfirm[USER_MAX_PASS + 1];

	while (1) {
		vga_print("  Username: ", COLOR_DEFAULT);
		keyboard_readline(username, USER_MAX_NAME + 1);

		if (strlen(username) == 0) {
			vga_println("  Username cannot be empty, try lats as a name lol", COLOR_RED);
			continue;
		}
		if (strlen(username) < 2) {
			vga_println("  Username must be at least 2 characters, biceps might be a nice name", COLOR_RED);
			continue;
		}
		break;
	}
	while (1) {
		vga_print("  Password: ", COLOR_DEFAULT);
		read_password(password, USER_MAX_PASS + 1);

		if (strlen(password) < 4) {
			vga_println("  Password must be 4 characters at least, try ilovebenchpress as a passw", COLOR_RED);
			continue;
		}

		vga_print("  Confirm : ", COLOR_DEFAULT);
		read_password(comfirm, USER_MAX_PASS + 1);
		if (strcmp(password, comfirm) != 0) {
			vga_println("  Passwords do not match. I said ilovebenchpress is a good password", COLOR_RED);
			continue;
		}
		break;
	}

	vga_println("", COLOR_DEFAULT);
	vga_print("  Creating acc '", COLOR_GREY);
	vga_print(username, COLOR_YELLOW);
	vga_println("'...", COLOR_GREY);

	int r = user_create(username, password, USER_FLAG_ADMIN);
	if (r == 0) {
		vga_println("  Account created. Home: home/", COLOR_GREEN);
		char home[9];
		auth_home_dir(username, home);
		char home_ower[9];
		for (int i = 0; home[i]; i++) {
			char c = home[i];
			if (c >= 'A' && c <= 'Z') c += 32;
			home_ower[i] = c;
		}
		home_ower[strlen(home)] = '\0';
		vga_println(home_ower + 1, COLOR_GREEN);
		vga_println("", COLOR_DEFAULT);
		timer_sleep(2);
	} else {
		vga_println("  ERROR: could not create account. prolly because u didnt get enough pump? try rebooting", COLOR_RED);
		timer_sleep(3);
	}
}

// public apis
void auth_init(void) {
	memset(&current_session, 0, sizeof(session_t));
	int loaded = user_db_load();
	if (loaded <= 0) {
		first_boot_setup();
	}
}

int auth_verify(const char* username, const char* password) {
	user_t* u = user_find(username);
	if (!u) return 0;
	return hash_verify(password, u->salt_hex, u->hash_hex);
}

void auth_logout(void) {
	memset(&current_session, 0, sizeof(session_t));
}

void auth_login_prompt(void) {
	int attempts = 0;
	char username[USER_MAX_NAME + 1];
	char password[USER_MAX_PASS + 1];
	
	while (1) {
		draw_login_banner();
		int uc = user_count();
		if (uc == 1) {
			vga_println("  1 user account registered", COLOR_GREY);
		} else {
			char buf[8]; itoa(uc, buf, 10);
			vga_print("  ", COLOR_DEFAULT);
			vga_print(buf, COLOR_GREY);
			vga_println(" user accounts registered", COLOR_GREY);
		}
		vga_println("", COLOR_DEFAULT);

		if (attempts >= AUTH_MAX_ATTEMPTS) {
			vga_print("  Too many failed attempts bruh.", COLOR_RED);
			vga_println("Wating for 15 seconds. Do some situps", COLOR_RED);
			vga_println("", COLOR_DEFAULT);

			for (int i = AUTH_LOCKOUT_SEC; i > 0; i--) {
				char buf[16];
				vga_print_at(cursor_row, 2, "Resuming in:   ", COLOR_GREY);
				itoa(i, buf, 10);
				vga_print_at(cursor_row, 15, buf, COLOR_YELLOW);
				vga_print_at(cursor_row, 15 + strlen(buf), "s  ", COLOR_YELLOW);
				timer_sleep(1);
			}
			attempts = 0;
			continue;
		}

		vga_print("  Username: ", COLOR_DEFAULT);
		keyboard_readline(username, USER_MAX_NAME + 1);

		if (strlen(username) == 0) continue;

		vga_print("  Password: ", COLOR_DEFAULT);
		read_password(password, USER_MAX_PASS + 1);

		vga_println("", COLOR_DEFAULT);

		if (auth_verify(username, password)) {
			user_t* u = user_find(username);

			current_session.logged_in = 1;
			strncpy(current_session.username, username, USER_MAX_NAME);
			current_session.flags = u->flags;
			auth_home_dir(username, current_session.home_dir);

			vga_print("  WELCOME, ", COLOR_GREEN);
			vga_print(username, COLOR_YELLOW);
			if (u->flags == USER_FLAG_ADMIN)
				vga_print("  [admin]", COLOR_CYAN);
			vga_println("!", COLOR_GREEN);
			vga_println("", COLOR_DEFAULT);
			timer_sleep(1);
			attempts = 0;
			return;
		} else {
			attempts++;
			int left = AUTH_MAX_ATTEMPTS - attempts;
			vga_print("  Your shoulders kinda sucks btw. Incorrent username or password. ", COLOR_RED);
			if (left > 0) {
				char buf[4]; itoa(left, buf, 10);
				vga_print(buf, COLOR_YELLOW);
				vga_println(" attempt(s) remaining.", COLOR_YELLOW);
			}
			vga_println("", COLOR_DEFAULT);
			timer_sleep(1);
		}
	}
}
