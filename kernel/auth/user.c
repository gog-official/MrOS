#include "user.h"
#include "../crypto/hash.h"
#include "../fs/vfs.h"
#include "../lib/string.h"

// in memmory user table
static user_t users[USER_MAX_COUNT];
static int user_count_val = 0;

// parsing helpers

static int parse_line(const char* line, user_t* u) {
	int pos = 0;
	memset(u, 0, sizeof(user_t));

	// usrname
	int i = 0;
	while (line[pos] && line[pos] != ':' && i < USER_MAX_NAME)
		u->username[i++] = line[pos++];
	if (line[pos] != ':') return 0;
	u->username[i] = '\0';
	pos++;

	for (i = 0; i < 4 && line[pos] && line[pos] != ':'; i ++)
		u->salt_hex[i] = line[pos++];
	u->salt_hex[4] = '\0';
	if (line[pos] != ':') return 0;
	pos++;

	for (i = 0; i < 8 && line[pos] && line[pos] != ':'; i++)
		u->hash_hex[i] = line[pos++];
	u->hash_hex[8] = '\0';
	if (line[pos] != ':') return 0;
	pos++;
	
	u->flags = (line[pos] >= '0' && line[pos] <= '9')
		? (uint8_t)(line[pos] - '0') : 0;
	u->valid = 1;
	return 1;
}

static int serialize_user(const user_t* u, char* buf) {
	int pos = 0;
	for (int i = 0; u->username[i]; i++) buf[pos++] = u->username[i];
	buf[pos++] = ':';

	for (int i = 0; i < 4; i++) buf[pos++] = u->salt_hex[i];
	buf[pos++] = ':';

	for (int i = 0; i < 8; i++) buf[pos++] = u->salt_hex[i];
	buf[pos++] = ':';

	buf[pos++] = '0' + u->flags;
	buf[pos++] = '\n';
	buf[pos] = '\0';
	return pos;
}

// home directory helpers
static void ensure_home_base(void) {
	static vfs_node_t entries[VFS_MAX_DIR_ENTRIED];
	int count = vfs_readdir("/", entries, VFS_MAX_DIR_ENTRIED);
	for (int i = 0; i < count; i++) {
		if (entries[i].type == VFS_TYPE_DIR && strcmp(entries[i].name, USER_HOME_DIR) == 0) return;
	}
	vfs_mkdir(USER_HOME_DIR);
}

static void create_home_dir(const char* username) {
	ensure_home_base();

	//build dir name
	char dirname[9];
	dirname[0] = 'H';
	int i = 1;
	for (int j = 0; username[j] && i < 8; j++, i++) {
		char c = username[j];
		if (c >= 'a' && c <= 'z') c -= 32;
		dirname[i] = c;
	}
	dirname[i] = '\0';
	vfs_mkdir(dirname);
}

// public api
int user_db_exists(void) {
	vfs_file_t f;
	if (vfs_open(USER_DB_FILE, &f) == 0) {
		vfs_close(&f);
		return 1;
	}
	return 0;
}

int user_db_load(void) {
	memset(users, 0, sizeof(users));
	user_count_val = 0;

	if (!user_db_exists()) return 0;

	vfs_file_t file;
	if (vfs_open(USER_DB_FILE, &file) < 0) return -1;

	static uint8_t buf[USER_MAX_COUNT * 40];
	int n = vfs_read(&file, buf, sizeof(buf) - 1);
	vfs_close(&file);
	if (n < 0) return -1;
	buf[n] = '\0';

	char* line = (char*)buf;
	while (*line && user_count_val < USER_MAX_COUNT) {
		char* end = line;
		while (*end && *end != '\n') end++;
		char saved = *end;
		*end = '\0';

		user_t u;
		if (strlen(line) > 4 && parse_line(line, &u)) {
			users[user_count_val++] = u;
		}

		*end = saved;
		if (*end == '\n') end++;
		line = end;
	}
	return user_count_val;
}

int user_db_save(void) {
	static char content[USER_MAX_COUNT * 40];
	int pos = 0;
	for (int i = 0; i < USER_MAX_COUNT; i++) {
		if (!users[i].valid) continue;
		pos += serialize_user(&users[i], content + pos);
	}

	vfs_remove(USER_DB_FILE);
	vfs_create(USER_DB_FILE);

	vfs_file_t file;
	if (vfs_open(USER_DB_FILE, &file) < 0) return -1;
	vfs_write(&file, (uint8_t*)content, (uint32_t)pos);
	vfs_close(&file);
	return 0;
}

user_t* user_find(const char* username) {
	for (int i = 0; i < USER_MAX_COUNT; i++) {
		if (users[i].valid && strcmp(users[i].username, username) == 0)
			return &users[i];
	}
	return 0;
}

int user_create(const char* username, const char* password, uint8_t flags) {
	// validate username
	for (int i = 0; username[i]; i++) {
		char c = username[i];
		int ok = (c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9')|| c=='_';
		if (!ok) return -4;
	}

	if (user_find(username)) return -1;

	int slot = -1;
	for (int i = 0; i < USER_MAX_COUNT; i++) {
		if (!users[i].valid) { slot = i; break; }
	}
	if (slot < 0) return -2;

	// hash password
	uint32_t salt = hash_generate_salt(username);
	char hash_hex[9];
	hash_password(password, salt, hash_hex);

	char salt_hex[5];
	uint16_to_hex((uint16_t)(salt & 0xFFFF), salt_hex);

	char full_salt_hex[9];
	uint32_to_hex(salt, full_salt_hex);

	// notey notey: our format uses 4-char salt. to store 32-bit salt fully, we just store it as 8-char hex in hash field prefix and use a slightly varied layout :}
	// clean solution: just use low 16 bits of salty salt(4 hex chars) and accept sligtly reduced salty space. still prevents rainbow tables for this inspirational/good/motivational/educational/ronnie-colemanational os
	memset(&users[slot], 0, sizeof(user_t));
	strncpy(users[slot].username, username, USER_MAX_NAME);
	memcpy(users[slot].salt_hex, salt_hex, 4);
	users[slot].salt_hex[4] = '\0';
	memcpy(users[slot].hash_hex, hash_hex, 8);
	users[slot].hash_hex[8] = '\0';
	users[slot].flags = flags;
	users[slot].valid = 1;
	user_count_val++;

	// save on di*k
	if (user_db_save() < 0) {
		users[slot].valid = 0;
		user_count_val--;
		return -3;
	}
	
	create_home_dir(username);
	return 0;
}

int user_change_password(const char* username, const char *new_password) {
	user_t* u = user_find(username);
	if (!u) return -1;

	uint32_t salt = hash_generate_salt(username);
	char hash_hex[9];
	hash_password(new_password, salt, hash_hex);
	char salt_hex[5];
	uint16_to_hex((uint16_t)(salt & 0xFFFF), salt_hex);

	memcpy(u->salt_hex, salt_hex, 4);
	u->salt_hex[4] = '\0';
	memcpy(u->hash_hex, hash_hex, 8);
	u->hash_hex[8] = '\0';

	return user_db_save();
}

int user_delete(const char* username) {
	user_t* u = user_find(username);
	if (!u) return -1;
	u->valid = 0;
	user_count_val--;
	return user_db_save();
}

int user_count(void) {
	return user_count_val;
}

int user_get_all(user_t *out, int max) {
	int n = 0;
	for (int i = 0; i < USER_MAX_COUNT && n < max; i++) {
		if (users[i].valid) out[n++] = users[i];
	}
	return n;
}
