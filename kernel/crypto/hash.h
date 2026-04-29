// password hashing for our newbie auth
#ifndef HASH_H
#define HASH_H

#include <stdint.h>

void hash_password(const char* password, uint32_t salt, char* out_hex);
uint32_t hash_generate_salt(const char* username);

int hash_verify(const char* password,
		const char* stored_salt_hex,
		const char* stored_hash_hex);

//hex helpers
void uint32_to_hex(uint32_t val, char* out);
void uint16_to_hex(uint16_t val, char* out);
uint32_t hex_to_uint32(const char* hex);

#endif // !DEBUG
