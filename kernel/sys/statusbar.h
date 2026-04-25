// the status bar will livw permanenltly on row 24(last row).
#ifndef STATUSBAR_H
#define STATUSBAR_H

// row assignments, shared with vga.c scroll guard
#define STATUSBAR_DIVIDER_ROW 23
#define STATUSBAR_ROW 24

// amount of time the reminder stays on screen
#define STATUSBAR_MSG_TIMEOUT_SEC 8 // we will change as per the checkings

void statusbar_init(void);
void statusbar_update(void);
void statusbar_set_msg(const char* msg, unsigned char color);
void statusbar_clear_msg(void);

#endif
