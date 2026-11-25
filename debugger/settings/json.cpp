//
// See json.h.
//

#include "json.h"

// ---------------------------------------------------------------------------------
// A small JSON reader. It reads one value at a time from a text and remembers
// where each value of the outermost object begins and ends, which is what
// replacing one of them in place needs.
// ---------------------------------------------------------------------------------

struct json_reader {
    const string &text;
    size_t at = 0;
    string problem;                             // the first mistake met ("" while all is well)

    explicit json_reader(const string &text) : text(text) {}

    /** say what is wrong, with the line it is on; gives false, for `return fail(...)` */
    bool fail(const string &what) {
        if(problem.empty()) {
            Int line = 1;
            for(size_t i = 0; i < at && i < text.size(); i++) {
                if(text[i] == '\n') line++;
            }
            stringstream words;
            words << "line " << line << ": " << what;
            problem = words.str();
        }
        return false;
    }

    void skip_space() {
        while(at < text.size() && (text[at] == ' ' || text[at] == '\t' || text[at] == '\n' || text[at] == '\r')) at++;
    }

    bool read_text(string &into) {
        at++;                                   // the opening quote
        into.clear();
        while(at < text.size() && text[at] != '"') {
            char c = text[at++];
            if(c == '\\' && at < text.size()) {
                char escaped = text[at++];
                switch(escaped) {
                    case 'n': c = '\n'; break;
                    case 't': c = '\t'; break;
                    case 'r': c = '\r'; break;
                    case 'u': return fail("a \\u.... character in a text: sdb.json takes plain characters only");
                    default:  c = escaped; break;       // \" \\ \/
                }
            }
            into += c;
        }
        if(at >= text.size()) return fail("a text is not closed with a `\"`");
        at++;
        return true;
    }

    bool read_value(json_value &value) {
        skip_space();
        value.from = at;
        if(at >= text.size()) return fail("the file ends where a value should stand");

        char c = text[at];
        if(c == '"') {
            value.kind = JSON_TEXT;
            if(!read_text(value.text)) return false;
        } else if(c == '[') {
            value.kind = JSON_LIST;
            at++;
            skip_space();
            while(at < text.size() && text[at] != ']') {
                json_value item;
                if(!read_value(item)) return false;
                value.items.push_back(item);
                skip_space();
                if(at < text.size() && text[at] == ',') at++;
                else if(at >= text.size() || text[at] != ']') return fail("a `,` or a `]` should follow a value of a list");
                skip_space();
            }
            if(at >= text.size()) return fail("a list is not closed with a `]`");
            at++;
        } else if(c == '{') {
            value.kind = JSON_OBJECT;
            at++;
            skip_space();
            while(at < text.size() && text[at] != '}') {
                string name;
                if(text[at] != '"') return fail("a name in quotes should stand here");
                if(!read_text(name)) return false;
                skip_space();
                if(at >= text.size() || text[at] != ':') return fail("a `:` should follow the name `" + name + "`");
                at++;

                json_value member;
                if(!read_value(member)) return false;
                value.names.push_back(name);
                value.values.push_back(member);
                skip_space();
                if(at < text.size() && text[at] == ',') at++;
                else if(at >= text.size() || text[at] != '}') return fail("a `,` or a `}` should follow the value of `" + name + "`");
                skip_space();
            }
            if(at >= text.size()) return fail("an object is not closed with a `}`");
            at++;
        } else if(text.compare(at, 4, "true") == 0) {
            value.kind = JSON_BOOL; value.truth = true; at += 4;
        } else if(text.compare(at, 5, "false") == 0) {
            value.kind = JSON_BOOL; value.truth = false; at += 5;
        } else if(text.compare(at, 4, "null") == 0) {
            value.kind = JSON_NULL; at += 4;
        } else if(c == '-' || (c >= '0' && c <= '9')) {
            value.kind = JSON_NUMBER;
            char *end = nullptr;
            value.number = strtod(text.c_str() + at, &end);
            at = end - text.c_str();
        } else {
            return fail(string("`") + c + "` is not the start of a value");
        }
        value.to = at;
        return true;
    }
};

/** read `text` as one JSON object; false (and `problem`) when it is not */
bool read_json_object(const string &text, json_value &object, string &problem) {
    json_reader reader(text);
    if(!reader.read_value(object)) {
        problem = reader.problem;
        return false;
    }
    reader.skip_space();
    if(object.kind != JSON_OBJECT || reader.at < text.size()) {
        problem = "the settings are one object: `{ ... }`";
        return false;
    }
    return true;
}

/** a text as JSON writes it, in quotes */
string json_text(const string &text) {
    string made = "\"";
    for(unsigned char c : text) {
        if(c == '"' || c == '\\') { made += '\\'; made += (char)c; }
        else if(c == '\n') made += "\\n";
        else if(c == '\r') made += "\\r";
        else if(c == '\t') made += "\\t";
        else if(c < 32) { char code[8]; snprintf(code, sizeof(code), "\\u%04x", c); made += code; }
        else made += (char)c;
    }
    return made + "\"";
}

const json_value* json_member(const json_value &object, const string &name) {
    for(size_t i = 0; i < object.names.size(); i++) {
        if(object.names[i] == name) return &object.values[i];
    }
    return nullptr;
}

string json_text_of(const json_value &object, const string &name, const string &otherwise) {
    const json_value *member = json_member(object, name);
    return member != nullptr && member->kind == JSON_TEXT ? member->text : otherwise;
}

Int json_number_of(const json_value &object, const string &name, Int otherwise) {
    const json_value *member = json_member(object, name);
    return member != nullptr && member->kind == JSON_NUMBER ? (Int)member->number : otherwise;
}
