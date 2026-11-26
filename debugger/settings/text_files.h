//
// READING AND WRITING A WHOLE TEXT FILE, for the few files sdb touches: its
// settings, a name map, a file of commands.
//
// With the C library's files, not C++'s streams: a std::ifstream made in sdb as
// it is linked on Windows (statically, with MinGW) crashed while it set itself
// up - before it had read anything.
//

#ifndef SHARP_DEBUGGER_TEXT_FILES_H
#define SHARP_DEBUGGER_TEXT_FILES_H

#include "../../runtime/stdimports.h"
#include <vector>

/** all of a file, byte for byte; false when it cannot be opened */
bool read_text_file(const string &file, string &text);

/** `text` as the whole of a file (made, or written over); false when it cannot be written */
bool write_text_file(const string &file, const string &text);

/** the lines of a text, without their ends (`\n` or `\r\n`) */
std::vector<string> lines_of(const string &text);

#endif //SHARP_DEBUGGER_TEXT_FILES_H
