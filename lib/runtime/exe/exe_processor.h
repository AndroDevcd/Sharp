//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_EXE_PROCESSOR_H
#define SHARP_EXE_PROCESSOR_H

#include "../../../stdimports.h"
#include "../../util/KeyPair.h"

struct sharp_type;

KeyPair<int, string> process_exe();
char next_char();
char current_char();
void next_chars(string &str, size_t len);
int32_t next_int32();
void next_string(string &str);
Int parse_int();
sharp_type *next_type();
void check_section(int section_type, Int error_code, KeyPair<int, string> &result);

#define PROCESS_SECTION(section_end, code, section_end_code) \
    for(;;) {\
        switch(next_char()) {\
            case 0x0:\
            case 0x0a:\
            case 0x0d:\
            case (section_end):\
                break;\
            \
            code \
                   \
            default: { \
                result.with(CORRUPT_MANIFEST, "file `" + executable + "` may be corrupt.\n"); \
                throw runtime_error(""); \
            } \
        } \
           \
        if(current_char() == section_end)  {   \
            section_end_code \
        } \
    }

extern string exeData;
extern int cursor;

#endif //SHARP_EXE_PROCESSOR_H
