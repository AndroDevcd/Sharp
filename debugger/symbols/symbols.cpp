//
// See symbols.h.
//

#include "symbols.h"
#include "../../runtime/virtual_machine.h"
#include <unordered_map>
#include <mutex>
#include <algorithm>

// ---------------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------------

#define GLOBAL_CLASS "__srt_global."

/** a function's whole name with the compiler's class for functions outside any class taken out */
static string without_global_class(string name) {
    size_t at = name.find("#" GLOBAL_CLASS);
    if(at != string::npos) name.erase(at + 1, strlen(GLOBAL_CLASS));
    return name;
}

string name_of(sharp_function *function) {
    return without_global_class(written_name(function));
}

string carried_note(const string &written, const string &carried) {
    return written == carried ? "" : " (" + carried + ")";
}

string carried_note(sharp_function *function) {
    if(!has_name_map()) return "";
    return carried_note(written_name(function), function->fullName);
}

string carried_note(sharp_class *of) {
    if(!has_name_map()) return "";
    return carried_note(written_name(of), of->fullName);
}

/** does `full` end with `name`, with a `.` or `#` (or nothing) in front of it? */
static bool ends_with_name(const string &full, const string &name) {
    if(name.empty() || name.size() > full.size()) return false;
    if(full.compare(full.size() - name.size(), name.size(), name) != 0) return false;
    if(name.size() == full.size()) return true;

    char before = full[full.size() - name.size() - 1];
    return before == '.' || before == '#';
}

std::vector<sharp_function*> functions_named(const string &name, bool &ambiguous) {
    std::vector<sharp_function*> found;
    ambiguous = false;

    // the whole name first: `app#main` must not also find `other.app#main`
    for(Int i = 0; i < vm.mf.methods; i++) {
        if(name_of(&vm.methods[i]) == name) found.push_back(&vm.methods[i]);
    }
    if(!found.empty()) return found;

    for(Int i = 0; i < vm.mf.methods; i++) {
        if(ends_with_name(name_of(&vm.methods[i]), name)) {
            if(!found.empty() && name_of(found[0]) != name_of(&vm.methods[i])) ambiguous = true;
            found.push_back(&vm.methods[i]);
        }
    }
    return found;
}

bool is_library_function(sharp_function *function) {
    string name = written_name(function);
    size_t hash = name.find('#');
    string module = hash == string::npos ? name : name.substr(0, hash);

    for(const char *library : { "std", "platform" }) {
        size_t length = strlen(library);
        if(module.compare(0, length, library) == 0 && (module.size() == length || module[length] == '.')) return true;
    }
    return false;
}

std::vector<sharp_function*> main_functions() {
    std::vector<sharp_function*> found;
    const string ending = "#" GLOBAL_CLASS "main";

    for(Int i = 0; i < vm.mf.methods; i++) {
        string full = written_name(&vm.methods[i]);
        if(full.size() >= ending.size() && full.compare(full.size() - ending.size(), ending.size(), ending) == 0) {
            found.push_back(&vm.methods[i]);
        }
    }
    return found;
}

// ---------------------------------------------------------------------------------
// Lines
// ---------------------------------------------------------------------------------

/**
 * A function's line table as an array, made the first time it is asked for.
 * (The VM keeps it as a linked list, which is slow to search; a step asks for
 * a line before every instruction.) Asked from the program's threads and from
 * the prompt's, hence the lock.
 */
static const std::vector<line_data>& lines_of(sharp_function *function) {
    static std::mutex &tables_mut = *new std::mutex();
    static auto &tables = *new std::unordered_map<sharp_function*, std::vector<line_data>*>();

    std::lock_guard<std::mutex> lock(tables_mut);
    auto found = tables.find(function);
    if(found != tables.end()) return *found->second;

    auto *table = new std::vector<line_data>();
    for(Int i = 0; i < function->lineTable.size; i++) {
        table->push_back(function->lineTable.node_at(i)->data);
    }
    tables[function] = table;
    return *table;
}

Int line_of(sharp_function *function, Int pc) {
    // the line table says where each line BEGINS, in ascending order: the line
    // of an instruction is the last one that began at or before it
    const std::vector<line_data> &table = lines_of(function);
    auto after = std::upper_bound(table.begin(), table.end(), pc,
                                  [](Int wanted, const line_data &entry) { return wanted < entry.pc; });
    if(after == table.begin()) return table.empty() ? -1 : table.front().line_number;
    return (after - 1)->line_number;
}

/** `path` with every `\` written as `/` */
static string with_slashes(string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

/**
 * A path as two paths are COMPARED: with `/` for `\` - and on Windows in small
 * letters, where `C:\Code\App.sharp` and `c:/code/app.sharp` are one file.
 * (Only for comparing: a file is shown as it is written.)
 */
static string with_forward_slashes(string path) {
    path = with_slashes(path);
#ifdef _WIN32
    std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) { return (char)tolower(c); });
#endif
    return path;
}

string shown_file(Int file) {
    if(file < 0 || file >= (Int)vm.mdata.files.size) return "";
    string path = with_slashes(written_file(file));

    // the name alone, then with one more folder in front each time, until no other file ends the same way
    size_t cut = path.find_last_of('/');
    while(cut != string::npos) {
        if(files_named(path.substr(cut + 1)).size() <= 1) return path.substr(cut + 1);
        if(cut == 0) break;
        cut = path.find_last_of('/', cut - 1);
    }
    return path;
}

string describe_place(const code_place &place) {
    stringstream words;
    words << name_of(place.function) << carried_note(place.function) << "  ";

    Int line = line_of(place.function, place.pc);
    string file = shown_file(place.function->sourceFile);
    if(line >= 0 && !file.empty()) words << file << ":" << line;
    else words << "instruction " << place.pc;
    return words.str();
}

std::vector<Int> files_named(const string &name) {
    std::vector<Int> found;
    string wanted = with_forward_slashes(name);

    for(Int i = 0; i < (Int)vm.mdata.files.size; i++) {
        string file = with_forward_slashes(written_file(i));
        if(file == wanted) {
            // the whole path: that file and no other
            found.clear();
            found.push_back(i);
            return found;
        }
        if(wanted.size() < file.size() && file[file.size() - wanted.size() - 1] == '/'
           && file.compare(file.size() - wanted.size(), wanted.size(), wanted) == 0) {
            found.push_back(i);
        }
    }
    return found;
}

std::vector<code_place> places_at_line(Int file, Int line, Int &nextLine) {
    std::vector<code_place> places;
    nextLine = -1;

    for(Int i = 0; i < vm.mf.methods; i++) {
        sharp_function *function = &vm.methods[i];
        if(function->sourceFile != file) continue;

        // (a line may begin more than once in a function - a loop's head does;
        //  the table is in ascending order, so the first entry is the first instruction)
        bool taken = false;
        for(const line_data &entry : lines_of(function)) {
            if(entry.line_number == line && !taken) {
                code_place place;
                place.function = function;
                place.pc = entry.pc;
                places.push_back(place);
                taken = true;
            }
            if(entry.line_number > line && (nextLine == -1 || entry.line_number < nextLine)) {
                nextLine = entry.line_number;
            }
        }
    }
    return places;
}

// ---------------------------------------------------------------------------------
// Source text
// ---------------------------------------------------------------------------------

Int source_line_count(Int file) {
    if(file < 0 || file >= (Int)vm.mdata.files.size) return 0;

    // counted once per file, by walking the list: the count the list keeps of itself may be more
    // than the lines it holds
    static std::mutex &counts_mut = *new std::mutex();
    static auto &counts = *new std::unordered_map<Int, Int>();
    std::lock_guard<std::mutex> lock(counts_mut);
    auto known = counts.find(file);
    if(known != counts.end()) return known->second;

    auto &lines = vm.mdata.files.node_at(file)->data->lines;
    Int count = 0;
    for(auto *line = lines.node_at(0); line != nullptr; line = line->next) count++;
    counts[file] = count;
    return count;
}

string source_line(Int file, Int line) {
    if(line < 1 || line > source_line_count(file)) return "";
    auto *found = vm.mdata.files.node_at(file)->data->lines.node_at(line - 1);
    return found == nullptr ? "" : found->data;         // (the list may hold fewer lines than it counts)
}

// ---------------------------------------------------------------------------------
// Classes
// ---------------------------------------------------------------------------------

/** is this the class the compiler keeps a module's own functions and variables in? */
static bool holds_module_globals(sharp_class *of) {
    const string ending = "#__srt_global";
    string full = written_name(of);
    return full.size() >= ending.size() && full.compare(full.size() - ending.size(), ending.size(), ending) == 0;
}

std::vector<sharp_class*> classes_named(const string &name, bool &ambiguous) {
    std::vector<sharp_class*> found;
    ambiguous = false;

    for(Int i = 0; i < vm.mf.classes; i++) {
        if(written_name(&vm.classes[i]) == name && !holds_module_globals(&vm.classes[i])) found.push_back(&vm.classes[i]);
    }
    if(!found.empty()) return found;

    for(Int i = 0; i < vm.mf.classes; i++) {
        if(!holds_module_globals(&vm.classes[i]) && ends_with_name(written_name(&vm.classes[i]), name)) found.push_back(&vm.classes[i]);
    }
    ambiguous = found.size() > 1;
    return found;
}

sharp_class* globals_of_module(const string &module) {
    const string wanted = module + "#__srt_global";
    for(Int i = 0; i < vm.mf.classes; i++) {
        if(written_name(&vm.classes[i]) == wanted) return &vm.classes[i];
    }
    return nullptr;
}

string name_of(sharp_class *of) {
    string full = written_name(of);
    if(holds_module_globals(of)) return "module " + full.substr(0, full.size() - strlen("#__srt_global"));
    return full;
}
