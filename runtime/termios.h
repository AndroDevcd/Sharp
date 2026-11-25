//
// Created by BraxtonN on 7/10/2018.
//

#ifndef SHARP_TERMIOS_H
#define SHARP_TERMIOS_H

#include "stdimports.h"

#ifdef POSIX_
extern char getch(void);

/* Read 1 character with echo */
extern char getche(void);

extern bool _kbhit(void);

void get_original_console_settings();

void reset_original_console_settings();
#else
#include <conio.h>
#endif

/** One character of the program's input (a key, or the next byte of a file or pipe); -1 at its end. */
long long vm_read_key(bool echo);

#endif //SHARP_TERMIOS_H
