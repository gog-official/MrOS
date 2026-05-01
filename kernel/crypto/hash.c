#include "hash.h"
#include "../drivers/timer.h"
#include "../lib/string.h"

// hex conversions 
static char hex_digit(uint8_t v) {
	return v < 10 ? '0' + v : 'A' + (v - 10);
}

void uint32_to_hex(uint32_t val, char* out) {
	for (int i = 7; i >= 0; i--) {
		out[i] = hex_digit(val & 0xF);
		val >>= 4;
	}
	out[8] = '\0';
}

void uint16_to_hex(uint16_t val, char* out) {
	for (int i = 3; i >= 0; i--) {
		out[i] = hex_digit(val & 0xF);
		val >>= 4;
	}
	out[4] = '\0';
}

uint32_t hex_to_uint32(const char* hex) {
	uint32_t result = 0;
	for (int i = 0; i < 8 && hex[i]; i++) {
		result <<= 4;
		char c = hex[i];
		if (c >= '0' && c <= '9') result |= (uint32_t)(c - '0');
		else if (c >= 'A' && c <= 'F') result |= (uint32_t)(c - 'A' + 10);
		else if (c >= 'a' && c <= 'f') result |= (uint32_t)(c - 'a' + 10);
	}
	return result;
}

// djb2 hash

static uint32_t djb2(const char* str, uint32_t seed) {
	uint32_t hash = seed ^ 5381;
	int c;
	while ((c = (unsigned char)*str++)) {
		hash = ((hash << 5) + hash) ^ (uint32_t)c;
	}
	return hash;
}

// public api

void hash_password(const char* password, uint32_t salt, char* out_hex) {
	char salted[128];
	salted[0] = '\0';
	char salt_str[9];
	uint32_to_hex(salt, salt_str);
	strcat(salted, salt_str);
	strcat(salted, ":");
	strcat(salted, password);

	uint32_t hash = djb2(salted, salt);
	// second pass for extra masala
	hash = djb2(salted, hash);
	uint32_to_hex(hash, out_hex);
}

uint32_t hash_generate_salt(const char* username) {
	uint32_t t = timer_get_ticks();
	uint32_t base = djb2(username, 5381);
	return base ^ (t * 2654435761u); // knuth multiplicative hash mix
}

int hash_verify(const char* password, const char *stored_salt_hex, const char *stored_hash_hex) {
	// pad salthex to 8 chars if its only 4
	char full_salt_hex[9];
	memset(full_salt_hex, '0', 8);
	full_salt_hex[8] = '\0';
	int slen = strlen(stored_salt_hex);
	for (int i = 0; i < slen && i < 8; i++)
		full_salt_hex[8 - slen + i] = stored_salt_hex[i];

	uint32_t salt = hex_to_uint32(full_salt_hex);

	char computed[9];
	hash_password(password, salt, computed);

	return strcmp(computed, stored_hash_hex) == 0;
}
