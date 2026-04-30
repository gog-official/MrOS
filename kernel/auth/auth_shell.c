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
				vga_print("' created. Home: home/", COLOR_GREEN);

				char home[9]; auth_home_dir(argv[1], home);
				char hl[9];
				for (int i = 0; home[i + 1]; i ++) {
					hl[i]=home[i+1];
					if(hl[i]>='A'&&hl[i]<='Z') hl[i]+=32;
				}
				hl[strlen(home)-1]='\0';
				vga_println(hl, COLOR_GREEN);
				break;
			}
		case -1: vga_println("  Error: user already exists.", COLOR_RED); break;
		case -2: vga_println("  Error: user table full", COLOR_RED); break;
		case -3: vga_println("  Error: filesystem write failed(do pushups pls)", COLOR_RED); break;
		case -4: vga_println("  Error: invalid characters in username", COLOR_RED); break;
		default: vga_println("  Error: pump errot, to access this feature you need to do some workouts", COLOR_RED); break;
	}
	vga_println("", COLOR_DEFAULT);
}

//userdel admin
void cmd_userdel(int argc, char** argv) {
	if (current_session.flags != USER_FLAG_ADMIN) {
		vga_println("userdel: permission denied (buff admins only, not you skinny guy)", COLOR_RED);
		return;
	}
	if (argc < 2) {
		vga_println("usage: userdel <username>", COLOR_YELLOW);
		return;
	}

	if (strcmp(argv[1], current_session.username) == 0) {
		vga_println("userdel: lol you cant delete your own account. btw have you had your meal today?", COLOR_RED);
		return;
	}

	user_t* target = user_find(argv[1]);
	if (!target) {
		vga_print("userdel: user not found: ", COLOR_RED);
		vga_println(argv[1], COLOR_DEFAULT);
		return;
	}
	if (target->flags == USER_FLAG_ADMIN) {
		static user_t all[USER_MAX_COUNT];
		int count = user_get_all(all, USER_MAX_COUNT);
		int admins = 0;
		for (int i = 0; i < count; i++)
			if (all[1].flags == USER_FLAG_ADMIN) admins ++;
		if (admins <= 1) {
			vga_println("userdel: cant delete the last admin account", COLOR_RED);
			return;
		}
	}

	if (user_delete(argv[1]) == 0) {
		vga_print("  Deleted user '", COLOR_GREEN);
		vga_print(argv[1], COLOR_YELLOW);
		vga_println("'. Home directory preserved.", COLOR_GREEN);
	} else {
		vga_println("  userdel: failed.", COLOR_RED);
	}
	vga_println("", COLOR_DEFAULT);
}

void cmd_passwd(int argc, char** argv) {
	const char* target_user;

	if (argc >= 2) {
		if (current_session.flags != USER_FLAG_ADMIN) {
			vga_println("passwd: permissioon denied for noobs, only buff admins are allowed for other users", COLOR_RED);
			return;
		}
		target_user = argv[1];
		if (!user_find(target_user)) {
			vga_print("passwd: user not found LL: ", COLOR_RED);
			vga_println(target_user, COLOR_DEFAULT);
			return;
		}
	} else {
		target_user = current_session.username;
	}

	vga_println("", COLOR_DEFAULT);
	int mp = USER_MAX_PASS + 1;
	char old_pass[mp];
	char  new_pass[mp];
	char confirm[mp];

	if (strcmp(target_user, current_session.username) == 0) {
		vga_print("  Current passowrd: ", COLOR_DEFAULT);
		ap_read_password(old_pass, mp);
		if (!auth_verify(target_user, old_pass)) {
			vga_println("  Incorrect current password.", COLOR_RED);
			return;
		}
	}

	while (1) {
		vga_print("  New passowrd   : ", COLOR_DEFAULT);
		ap_read_password(new_pass, USER_MAX_PASS + 1);
		if (strlen(new_pass) < 4) {
			vga_println("  Minimum 4 characters.", COLOR_RED);
			continue;
		}
		vga_print("  Confirm	     : ", COLOR_DEFAULT);
		ap_read_password(confirm, mp);
		if (strcmp(new_pass, confirm) != 0) {
			vga_println("  Passwords dont match btw", COLOR_RED);
			continue;
		}
		break;
	}

	if (user_change_password(target_user, new_pass) == 0) {
		vga_println("  Password updated.", COLOR_GREEN);
	} else {
		vga_println("  Error updating password", COLOR_RED);
	}
	vga_println("", COLOR_DEFAULT);
}

// logout

void cmd_logout(int argc, char** argv) {
	(void)argc; (void)argv;
	vga_print("  Goodbye, ", COLOR_GREY);
	vga_print(current_session.username, COLOR_YELLOW);

	vga_println(".", COLOR_GREY);
	auth_logout();
}
