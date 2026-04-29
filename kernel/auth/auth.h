//  login logic  n session management
//  session:
//  	after a succeceful logging in, a session_t is populated, the shell reads session.username for the promp and commands check session.flags for admin ops
// login:
// 	just see and figure out the flow(its way commmon)
//
// of course there is  alockout for FAILURE!
#ifndef AUTH_H
#define AUTH_H

#include "user.h"

#define AUTH_MAX_ATTEMPTS 3
#define AUTH_LOCKOUT_SEC 15

typedef struct {
	int logged_in;
	char username[USER_MAX_NAME + 1];
	uint8_t flags;
	char home_dir[32];
} session_t;

extern session_t current_session;

void auth_init(void);

void auth_login_prompt(void);

int auth_verify(const char* username, const char* password);

void auth_logout(void);

void auth_home_dir(const char* username, char* out);

#endif // !AUTH_H
