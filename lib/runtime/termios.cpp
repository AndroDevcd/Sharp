//
// Created by BraxtonN on 7/10/2018.
//
#include "../../stdimports.h"
#ifdef POSIX_
#include <termios.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include <unistd.h>

struct termios original;
struct termios old;
struct termios _new;

// THE CONSOLE AS THE VM FOUND IT.
//
// Reading one key (getch) turns the console's line mode and echo off for a moment. A program that
// is stopped in that moment - Ctrl+C, a crash - would leave the console that way: nothing typed is
// shown, lines do not start at the left. So the settings are SAVED when the VM starts and PUT BACK
// when it ends, however it ends (sig_handler.cpp).
//
// `have_original` is the point of the fix this comment came with: the settings used to be put back
// without ever having been saved, which wrote an EMPTY set of settings into the console - the
// very damage it was there to prevent. Nothing is put back unless something was saved, and nothing
// is saved when the input is not a console at all (a file, a pipe, /dev/null).
static bool have_original = false;

void get_original_console_settings() {
    if(have_original) return;
    if(isatty(0) && tcgetattr(0, &original) == 0) have_original = true;
}

void reset_original_console_settings() {
    // Only a program the console belongs to right now may set it. Any other - one started by `timeout`,
    // or with & - is STOPPED by the system for trying (SIGTTOU), and then never ends; there is also
    // nothing of its own to put back. (tcsetattr, tcgetpgrp and getpgrp may all be called from a signal handler.)
    if(have_original && tcgetpgrp(0) == getpgrp()) tcsetattr(0, TCSANOW, &original);
}

/* Initialize new terminal i/o settings */
void initTermios(int echo)
{
    tcgetattr(0, &old); /* grab old terminal i/o settings */
    _new = old; /* make new settings same as old settings */
    _new.c_lflag &= ~ICANON; /* disable buffered i/o */
    if (echo) {
        _new.c_lflag |= ECHO; /* set echo mode */
    } else {
        _new.c_lflag &= ~ECHO; /* set no echo mode */
    }
    tcsetattr(0, TCSANOW, &_new); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void)
{
    tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo)
{
    // The key is read straight from the console (read), not through C's buffered input (getchar).
    // getchar takes everything that is there into a buffer of its own and hands out one key; the
    // rest then waits in that buffer, where kbhit() - which asks the CONSOLE how much is waiting -
    // cannot see it. "q, Enter" typed quickly was one key and then "nothing waiting".
    // Nothing more to read (the input has ended): the key 0.
    unsigned char ch = 0;
    initTermios(echo);
    if(read(0, &ch, 1) != 1) ch = 0;
    resetTermios();
    return (char)ch;
}
void enable_raw_mode()
{
    termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO); // Disable echo as well
    tcsetattr(0, TCSANOW, &term);
}

void disable_raw_mode()
{
    termios term;
    tcgetattr(0, &term);
    term.c_lflag |= ICANON | ECHO;
    tcsetattr(0, TCSANOW, &term);
}

bool kbhit()
{
    // (the count starts at 0 and the answer of ioctl is checked: when the input is not a console
    //  - /dev/null, a closed pipe - the call fails and used to leave the count as whatever was in
    //  memory, so "a key is waiting" could be true forever)
    int byteswaiting = 0;
    if(ioctl(0, FIONREAD, &byteswaiting) != 0) return false;
    return byteswaiting > 0;
}

bool _kbhit()
{
    bool hit;
    initTermios(false);
    hit = kbhit();
    resetTermios();
    return hit;
}

/* Read 1 character without echo */
char getch(void)
{
    return getch_(0);
}

/* Read 1 character with echo */
char getche(void)
{
    return getch_(1);
}
#endif