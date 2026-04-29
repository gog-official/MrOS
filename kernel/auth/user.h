// user acc management
#ifndef USER_H
#define USER_H

#include <stdint.h>

#define USER_MAX_NAME 16
#define USER_MAX_PASS 32
#define USER_MAX_COUNT 16
#define USER_DB_FILE "USER.DB"
#define USER_HOME_DIR "HOME"

// flags
#define USER_FLAG_NORMAL 0
#define USER_FLAG_ADMIN 1

typedef struct {
	char username[USER_MAX_NAME + 1];
	char salt_hex[5];
	char hash_hex[9];
	uint8_t flags;
	int valid;
} user_t;

//das reads user.db from fs into mem
int user_db_load(void);

//das writes all user back to db
int user_db_save(void);

//find user by their usrname
user_t* user_find(const char* username);

//add a new usr account by hashing and writing to db, also makes home dir
int user_create(const char* username, const char* password, uint8_t flags);

//update user passw
int user_change_password(const char* username, const char* new_password);

// delete user but not their home directory
int user_delete(const char* username);

//return numb of valid usrs
int user_count(void);

// fill out[] with copies of all valid users
int user_get_all(user_t* out, int max);

// returns 1 if user.db exists on di*k, 0 otherwise
int user_db_exists(void);

#endif
