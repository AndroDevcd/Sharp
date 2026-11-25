//
// See settings.h.
//

#include "settings.h"
#include "text_files.h"
#include "json.h"

sdb_settings& settings() {
    static sdb_settings &running = *new sdb_settings();
    return running;
}

// ---------------------------------------------------------------------------------
// Reading the settings
// ---------------------------------------------------------------------------------

/** `name` as a path seen from the folder of the settings file (a path from the root stays as it is) */
static string beside(const string &settingsFile, const string &name) {
    if(name.empty() || name[0] == '/' || name[0] == '\\' || (name.size() > 1 && name[1] == ':')) return name;
    size_t cut = settingsFile.find_last_of("/\\");
    return cut == string::npos ? name : settingsFile.substr(0, cut + 1) + name;
}

/** every item of a list as a text; false when the value is not a list of texts */
static bool texts_of(const json_value &value, std::vector<string> &into) {
    if(value.kind != JSON_LIST) return false;
    for(const json_value &item : value.items) {
        if(item.kind != JSON_TEXT) return false;
        into.push_back(item.text);
    }
    return true;
}

bool read_settings(const string &file, sdb_settings &settings, string &problem) {
    settings.file = file;

    string text;
    if(!read_text_file(file, text)) return true;        // not there: the defaults
    settings.found = true;

    json_value object;
    if(!read_json_object(text, object, problem)) {
        problem = file + ": " + problem;
        return false;
    }

    for(size_t i = 0; i < object.names.size(); i++) {
        const string &name = object.names[i];
        const json_value &value = object.values[i];
        string wanted;                              // what the value should have been, when it is not

        if(name == "program") {
            if(value.kind == JSON_TEXT) settings.program = beside(file, value.text); else wanted = "a text";
        } else if(name == "arguments") {
            if(!texts_of(value, settings.arguments)) wanted = "a list of texts";
        } else if(name == "run") {
            if(value.kind == JSON_BOOL) settings.run = value.truth; else wanted = "true or false";
        } else if(name == "map_file") {
            if(value.kind == JSON_TEXT) settings.mapFile = beside(file, value.text); else wanted = "a text";
        } else if(name == "view") {
            if(value.kind == JSON_TEXT && (value.text == "full" || value.text == "plain")) settings.plainView = value.text == "plain";
            else wanted = "\"full\" or \"plain\"";
        } else if(name == "list_lines") {
            if(value.kind == JSON_NUMBER && value.number >= 0 && value.number <= 1000) settings.listLines = (Int)value.number;
            else wanted = "a number from 0 to 1000";
        } else if(name == "save_breakpoints") {
            if(value.kind == JSON_BOOL) settings.saveBreakpoints = value.truth; else wanted = "true or false";
        } else if(name == "breakpoints") {
            if(!texts_of(value, settings.breakpoints)) wanted = "a list of texts";
        } else {
            problem = file + ": `" + name + "` is not a setting. The settings are: program, arguments, run, map_file, "
                      "view, list_lines, save_breakpoints, breakpoints";
            return false;
        }

        if(!wanted.empty()) {
            problem = file + ": `" + name + "` should be " + wanted;
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------------
// Saving the breakpoints
// ---------------------------------------------------------------------------------

/** the list as it is written into the file: on one line when short, a line each otherwise */
static string list_text(const std::vector<string> &breakpoints, const string &indent) {
    if(breakpoints.empty()) return "[]";

    string made = "[";
    for(size_t i = 0; i < breakpoints.size(); i++) {
        made += "\n" + indent + indent + json_text(breakpoints[i]);
        if(i + 1 < breakpoints.size()) made += ",";
    }
    return made + "\n" + indent + "]";
}

bool save_breakpoints(const string &file, const std::vector<string> &breakpoints, string &problem) {
    const string indent = "    ";
    string text;
    string made;

    if(!read_text_file(file, text) || text.find_first_not_of(" \t\r\n") == string::npos) {
        // no file yet (or an empty one): one is made
        made = "{\n" + indent + "\"breakpoints\": " + list_text(breakpoints, indent) + "\n}\n";
    } else {
        json_value object;
        if(!read_json_object(text, object, problem)) {
            problem = file + " was not written: it cannot be read (" + problem + ")";
            return false;
        }

        size_t found = object.names.size();
        for(size_t i = 0; i < object.names.size(); i++) {
            if(object.names[i] == "breakpoints") found = i;
        }

        if(found < object.names.size()) {
            // the list that is there is replaced, and nothing else
            const json_value &old = object.values[found];
            made = text.substr(0, old.from) + list_text(breakpoints, indent) + text.substr(old.to);
        } else {
            // there is none: it is added before the object's closing `}`
            size_t close = text.find_last_of('}');
            size_t last = text.find_last_not_of(" \t\r\n", close - 1);
            bool empty = object.names.empty();
            made = text.substr(0, last + 1) + (empty ? "" : ",") + "\n" + indent + "\"breakpoints\": "
                   + list_text(breakpoints, indent) + "\n" + text.substr(close);
        }
    }

    if(!write_text_file(file, made)) {
        problem = file + " cannot be written";
        return false;
    }
    return true;
}
