//
// Created by bknun on 4/26/2023.
//

#ifndef SHARP_CONSOLE_HELPER_H
#define SHARP_CONSOLE_HELPER_H

#include "stdimports.h"

void dbl_printf(int format, double value, int precision);

/**
 * A decimal as text, the way a person would write it: 5, 0.25, -3.5, 0.1.
 * THE one place the VM turns a number into digits for a program - printing
 * (DPUT), text + number, and to_string() all come here, so they agree.
 */
std::string natural_number(double value);

/**
 * WHAT THE PROGRAM PRINTS, AND WHEN IT REACHES THE SCREEN
 *
 * Everything a program prints goes into one buffer of the VM (the C library's,
 * for stdout, made 64 KB here) and leaves it in large pieces: a program that
 * prints a million lines makes a few hundred calls to the operating system, not
 * a million. Nobody has to flush for that to be seen in time - the VM does:
 *
 *   - a few milliseconds after something was printed (a thread of the VM's own
 *     watches for it), so a line, a prompt, a progress dot is on the screen
 *     before anyone could notice it was not;
 *   - at once before the program waits for a key or a line of input, before it
 *     runs another program (which writes to the same screen), and when it ends
 *     - also after a crash or Ctrl+C;
 *   - when the program says so: flush().
 *
 * It is the same whether the output goes to a terminal, a file or another
 * program (an IDE's console, a pipe).
 *
 *      console_output_start()      once, when the VM starts: before anything is printed
 *      console_output_written()    after the VM wrote something for the program
 *      console_output_flush()      everything printed so far is out when this returns
 */
void console_output_start();
void console_output_written();
void console_output_flush();

#endif //SHARP_CONSOLE_HELPER_H
