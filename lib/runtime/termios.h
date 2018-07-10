//
// Created by BraxtonN on 7/10/2018.
//

#ifndef SHARP_TERMIOS_H
#define SHARP_TERMIOS_H

#ifdef POSIX_
extern char getch(void);

/* Read 1 character with echo */
extern char getche(void);
#endif

#endif //SHARP_TERMIOS_H
