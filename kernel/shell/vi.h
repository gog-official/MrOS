// minimal vi like editor for MrOS
//
// possible commands:
// N = hjkl, wb, 0$, gg F, x, dd, o O, A, i, a, :w, :q, :q!, :wq, /<text>, u
// I = any printable character, backspace, return, del, esc
// 
// Layout: rows 0-21(text area), row 22 (vi tatusbar), row 23 and beyond, mros stuff

#ifndef VI_H
#define VI_H

#define VI_MAX_LINES 50
#define VI_MAX_LINE_LEN 160
#define VI_TEXT_ROWS 22
#define VI_TEXT_COLS 80

int vi_open(const char* filename);

#endif // !VI_H
