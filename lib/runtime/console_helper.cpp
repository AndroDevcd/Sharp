//
// Created by bknun on 4/26/2023.
//

#include "console_helper.h"
#include "memory/sharp_object.h"
#include "../core/opcode/opcode_macros.h"
#include "multitasking/thread/sharp_thread.h"
#include "multitasking/fiber/fiber.h"
#include "reflect/reflect_helpers.h"
#include <cstdint>
#if __has_include(<charconv>)
#include <charconv>
#endif

// std::to_chars for a double (GCC 11 and later): the shortest digits in one go
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
#define SHORTEST_DIGITS_IN_ONE_GO 1
#else
#define SHORTEST_DIGITS_IN_ONE_GO 0
#endif

//
// natural_number(value): a decimal as text, the way a person would write it.
//
//      5.0                     ->  5
//      0.25                    ->  0.25
//      0.1                     ->  0.1          (not 0.1000000000000000055511151231257827)
//      0.1 + 0.2               ->  0.30000000000000004
//      1e21                    ->  1e+21        (very large and very small numbers keep an exponent)
//
// THE RULE: the FEWEST digits that still read back as exactly the same
// number. A double holds 15 to 17 significant digits; printing a fixed 16
// places after the point (what this VM did until November 2026) shows the
// zeros of every whole number and the rounding dust of every fraction. So:
// try 1 digit, then 2, then 3 ... and stop at the first that, read back in,
// gives the number we started with. That is what JavaScript, Python, Kotlin
// and Swift print too.
//
// The digits are then laid out without an exponent as long as that stays
// readable (up to 21 digits before the point, down to 6 zeros after it),
// and with one beyond that.
//
std::string natural_number(double value) {
    if(value != value) return "nan";
    if(value == 0) return "0";                              // (also "minus zero")
    if(value > 1.7976931348623157e308) return "infinity";
    if(value < -1.7976931348623157e308) return "-infinity";

    // a whole number (a count, a pixel, an index - most numbers a program prints): its digits
    // straight away. Below 1e15 every whole number is exact, and comes out the same as below.
    if(value > -1e15 && value < 1e15 && value == (double) (int64_t) value) {
        return std::to_string((int64_t) value);
    }

    // the shortest "d.ddddde+XX" that reads back as the same number
    char text[64];
#if SHORTEST_DIGITS_IN_ONE_GO
    auto made = std::to_chars(text, text + sizeof(text) - 1, value, std::chars_format::scientific);
    *made.ptr = '\0';
#else
    // (an older compiler: try 1 digit, then 2 ... - up to 17 rounds of printing and reading back)
    for(int digits = 1; digits <= 17; digits++) {
        snprintf(text, sizeof(text), "%.*e", digits - 1, value);
        if(strtod(text, nullptr) == value) break;
    }
#endif

    // take it apart: the sign, the digits (without the point), the power of ten
    std::string out;
    const char* at = text;
    if(*at == '-') { out += '-'; at++; }
    std::string digits;
    while(*at != 'e' && *at != '\0') {
        if(*at != '.') digits += *at;
        at++;
    }
    int power = *at == 'e' ? atoi(at + 1) : 0;              // the first digit stands for 10^power
    int count = (int) digits.size();

    if(power >= 0 && power < 21) {
        // an ordinary number: the digits, zeros up to the point if needed, then what follows the point
        if(count <= power + 1) {
            out += digits;
            out.append((size_t) (power + 1 - count), '0');
        } else {
            out.append(digits, 0, (size_t) (power + 1));
            out += '.';
            out.append(digits, (size_t) (power + 1), std::string::npos);
        }
    } else if(power < 0 && power >= -7) {
        // a small fraction: 0.000ddd
        out += "0.";
        out.append((size_t) (-power - 1), '0');
        out += digits;
    } else {
        // too large or too small to read as plain digits: d.ddde+XX
        out += digits[0];
        if(count > 1) {
            out += '.';
            out.append(digits, 1, std::string::npos);
        }
        out += 'e';
        out += power < 0 ? '-' : '+';
        out += std::to_string(power < 0 ? -power : power);
    }
    return out;
}

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
        case 'n': {     // "natural": the fewest digits that read back as the same number
            string str(natural_number(value));
            auto strObject = create_object(str.size(), type_int8);

            copy_object(&push_stack_object, strObject);
            assign_string_field(strObject, str);
            return;
        }
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
            string str(natural_number(value));
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