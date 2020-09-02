//
// Created by BraxtonN on 7/10/2018.
//
#include "../../stdimports.h"
#ifdef POSIX_
#include <termios.h>
#include <sys/ioctl.h>
#include <stdio.h>

struct termios original;
struct termios old;
struct termios _new;

void get_original_console_settings() {
    tcgetattr(0, &original); /* grab old terminal i/o settings */
}

void reset_original_console_settings() {
    tcsetattr(0, TCSANOW, &original);
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
    char ch;
    initTermios(echo);
    ch = getchar();
    resetTermios();
    return ch;
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
    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);
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