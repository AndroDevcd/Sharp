//
// See names.h.
//

#include "names.h"
#include "../../runtime/virtual_machine.h"
#include "../settings/text_files.h"
#include <unordered_map>

static bool loaded = false;

// the map's lines by kind: the new name -> the old one
typedef std::unordered_map<string, string> name_lines;
static name_lines classes, fields, functions, locals, modules, files;

// what was worked out from them, once, when the map was loaded
static std::unordered_map<sharp_class*, string> class_names;
static std::unordered_map<sharp_function*, string> function_names;

bool has_name_map() {
    return loaded;
}

/** does `text` end with `ending`? */
static bool ends_with(const string &text, const string &ending) {
    return text.size() >= ending.size() && text.compare(text.size() - ending.size(), ending.size(), ending) == 0;
}

/**
 * The old name of something carried as `carried`, when the map has a line for
 * it AND that line's old name is `front` followed by one more name - "" when
 * it has none (the thing was not renamed).
 */
static string old_name_after(const name_lines &lines, const string &carried, const string &front) {
    auto found = lines.find(carried);
    if(found == lines.end()) return "";

    const string &old = found->second;
    if(old.size() <= front.size() || old.compare(0, front.size(), front) != 0) return "";
    return old;
}

/** (the map is loaded) a class's name as written; worked out from its module or the class around it */
static string work_out(sharp_class *of) {
    auto known = class_names.find(of);
    if(known != class_names.end()) return known->second;

    string front;
    if(of->owner != nullptr && ends_with(of->fullName, "." + of->name)) {
        front = work_out(of->owner) + ".";
    } else if(ends_with(of->fullName, "#" + of->name)) {
        string module = of->fullName.substr(0, of->fullName.size() - of->name.size() - 1);
        auto renamed = modules.find(module);
        front = (renamed == modules.end() ? module : renamed->second) + "#";
    } else {
        // not a name of the two shapes above: left as it is
        class_names[of] = of->fullName;
        return of->fullName;
    }

    string old = old_name_after(classes, of->name, front);
    string written = old.empty() ? front + of->name : old;
    class_names[of] = written;
    return written;
}

int load_name_map(const string &file) {
    string text;
    if(!read_text_file(file, text)) return -1;

    int count = 0;
    for(const string &line : lines_of(text)) {
        if(line.empty() || line[0] == '#') continue;

        // kind, two spaces, the new name, two spaces, the old name (which may hold spaces: a file's path)
        size_t first = line.find("  ");
        size_t second = first == string::npos ? string::npos : line.find("  ", first + 2);
        if(second == string::npos) continue;

        string kind = line.substr(0, first);
        string carried = line.substr(first + 2, second - first - 2);
        string old = line.substr(second + 2);

        name_lines *into = nullptr;
        if(kind == "class") into = &classes;
        else if(kind == "field") into = &fields;
        else if(kind == "function") into = &functions;
        else if(kind == "local") into = &locals;
        else if(kind == "module") into = &modules;
        else if(kind == "file") into = &files;
        if(into == nullptr) continue;

        (*into)[carried] = old;
        count++;
    }

    // every class and function once, now: names are asked for all the time
    for(Int i = 0; i < vm.mf.classes; i++) work_out(&vm.classes[i]);
    for(Int i = 0; i < vm.mf.methods; i++) {
        sharp_function *function = &vm.methods[i];
        if(function->owner == nullptr || !ends_with(function->fullName, "." + function->name)) {
            function_names[function] = function->fullName;
            continue;
        }
        string front = class_names[function->owner] + ".";
        string old = old_name_after(functions, function->name, front);
        function_names[function] = old.empty() ? front + function->name : old;
    }

    loaded = true;
    return count;
}

string written_name(sharp_class *of) {
    if(!loaded) return of->fullName;
    auto known = class_names.find(of);
    return known == class_names.end() ? of->fullName : known->second;
}

string written_name(sharp_function *function) {
    if(!loaded) return function->fullName;
    auto known = function_names.find(function);
    return known == function_names.end() ? function->fullName : known->second;
}

string written_name(sharp_field &field, sharp_class *of) {
    if(!loaded) return field.name;

    // a field a class got from a class above is named in that class's lines
    for(sharp_class *holder = of; holder != nullptr; holder = holder->base) {
        string front = written_name(holder) + ".";
        string old = old_name_after(fields, field.name, front);
        if(!old.empty()) return old.substr(front.size());
    }
    return field.name;
}

string written_name(const local_data &local) {
    if(!loaded) return local.name;

    // (not checked against its function: a lambda's table also lists the variables of the
    //  functions around it, whose lines name THOSE functions)
    auto found = locals.find(local.name);
    if(found == locals.end()) return local.name;

    size_t dot = found->second.find_last_of('.');
    return dot == string::npos ? found->second : found->second.substr(dot + 1);
}

string written_file(Int file) {
    if(file < 0 || file >= (Int)vm.mdata.files.size) return "";
    const string &carried = vm.mdata.files.node_at(file)->data->name;
    if(!loaded) return carried;

    auto found = files.find(carried);
    return found == files.end() ? carried : found->second;
}
