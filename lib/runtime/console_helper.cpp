//
// Created by bknun on 4/26/2023.
//

#include "console_helper.h"
#include "memory/sharp_object.h"
#include "../core/opcode/opcode_macros.h"
#include "multitasking/thread/sharp_thread.h"
#include "multitasking/fiber/fiber.h"
#include "reflect/reflect_helpers.h"

thread_local char buf[256];
void dbl_printf(int format, double value, int precision) {
    memset(buf, 0, sizeof(char)*256);
    auto task = thread_self->task;

    switch(format) {
        case 'd':
        case 'i':
            snprintf(buf, 256, "%d", (int)value);
            break;
        case 'u':
            snprintf(buf, 256, "%u", (unsigned int)value);
            break;
        case 'o':
            snprintf(buf, 256, "%o", (unsigned int)value);
            break;
        case 'x':
            snprintf(buf, 256, "%x", (unsigned int)value);
            break;
        case 'X':
            snprintf(buf, 256, "%X", (unsigned int)value);
            break;
        case 'f':
            if(precision) {
                stringstream ss;
                ss << "%." << precision << 'f';
                snprintf(buf, 256, ss.str().c_str(), value);
            } else
                snprintf(buf, 256, "%f", value);
            break;
        case 'F':
            if(precision) {
                stringstream ss;
                ss << "%." << precision << 'F';
                snprintf(buf, 256, ss.str().c_str(), value);
            } else
                snprintf(buf, 256, "%F", value);
            break;
        case 'e':
            snprintf(buf, 256, "%e", value);
            break;
        case 'E':
            snprintf(buf, 256, "%E", value);
            break;
        case 'g':
            snprintf(buf, 256, "%g", value);
            break;
        case 'G':
            snprintf(buf, 256, "%G", value);
            break;
        case 'a':
            snprintf(buf, 256, "%a", value);
            break;
        case 'A':
            snprintf(buf, 256, "%A", value);
            break;
        case 'c':
            snprintf(buf, 256, "%c", (unsigned int)value);
            break;
        case '%':
            snprintf(buf, 256, "%%");
            break;
        case 'l': {
            string str(to_string((int64_t)value));
            auto strObject = create_object(str.size(), type_int8);

            copy_object(&push_stack_object, strObject);
            assign_string_field(strObject, str);
            return;
        }
        case 'L': {
            string str(to_string((uint64_t)value));
            auto strObject = create_object(str.size(), type_int8);

            copy_object(&push_stack_object, strObject);
            assign_string_field(strObject, str);
            return;
        }
        default: {
            string str(to_string(value));
            auto strObject = create_object(str.size(), type_int8);

            copy_object(&push_stack_object, strObject);
            assign_string_field(strObject, str);
            return;
        }
    }

    string str(buf);
    auto strObject = create_object(str.size(), type_int8);

    copy_object(&push_stack_object, strObject);
    assign_string_field(strObject, str);
}