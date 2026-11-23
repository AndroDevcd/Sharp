//
// A SMALL JSON READER AND WRITER, for what sdb reads and writes itself: its
// settings (settings.h) and the lines it exchanges with an editor (machine/).
//
// It reads objects, lists, texts, numbers, true, false and null, and remembers
// where each value begins and ends in the text - which is what replacing one
// value of a file in place needs.
//

#ifndef SHARP_DEBUGGER_JSON_H
#define SHARP_DEBUGGER_JSON_H

#include "../../../stdimports.h"
#include <vector>

enum json_kind { JSON_NULL, JSON_BOOL, JSON_NUMBER, JSON_TEXT, JSON_LIST, JSON_OBJECT };

struct json_value {
    json_kind kind = JSON_NULL;
    bool truth = false;
    double number = 0;
    string text;
    std::vector<json_value> items;              // JSON_LIST
    std::vector<string> names;                  // JSON_OBJECT: the names, and under the same number ...
    std::vector<json_value> values;             // ... their values
    size_t from = 0, to = 0;                    // where it stands in the text: [from, to)
};

/** read `text` as one JSON object; false (and `problem` says what and on which line) when it is not */
bool read_json_object(const string &text, json_value &object, string &problem);

/** the value of an object's member; null when the object has none of that name */
const json_value* json_member(const json_value &object, const string &name);

/** a member that is a text / a whole number, or what is given when it is not there */
string json_text_of(const json_value &object, const string &name, const string &otherwise = "");
Int json_number_of(const json_value &object, const string &name, Int otherwise = 0);

/** a text as JSON writes it, in quotes, with what must be escaped escaped */
string json_text(const string &text);

#endif //SHARP_DEBUGGER_JSON_H
