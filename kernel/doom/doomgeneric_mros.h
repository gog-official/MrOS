#ifndef DOOMGENERIC_MROS_H
#define DOOMGENERIC_MROS_H

#include <stdint.h>
#include <stddef.h>

void doom_launch(void);
void doom_quit(void);
void doom_key_event(unsigned char ascii, int pressed);

// stdio stubs: override libc symbols for doomgeneric
void* fopen (const char* path, const char* mode);
int fclose (void* f);
size_t fread (void* buf, size_t size, size_t nmemb, void* f);
int fseek (void* f, long offset, int whence);
long ftell (void* f);
int feof(void *f);
int ferror(void *f);

#endif // DOOMGENERIC_MROS_H
