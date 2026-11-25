//
// See values.h.
//

#include "values.h"
#include "../control/control.h"
#include "../../runtime/virtual_machine.h"
#include "../../runtime/multitasking/fiber/fiber.h"
#include "../../runtime/memory/garbage_collector.h"
#include "../../runtime/types/access_flag.h"
#include "../../runtime/types/sharp_field.h"
#include "../../runtime/types/sharp_type.h"
#include "../../runtime/reflect/reflect_helpers.h"
#include "../../runtime/error/vm_exception.h"
#include <cmath>
#include <iomanip>

#define MOST_ELEMENTS 15        // how many elements of an array or a list text_of() shows before `...`
#define MOST_CHARACTERS 200     // how many characters of a text text_of() shows

// ---------------------------------------------------------------------------------
// Frames
// ---------------------------------------------------------------------------------

bool frame_at(fiber *task, Int index, frame_view &frame) {
    std::vector<code_place> calls = call_stack_of(task);
    if(index < 0 || index >= (Int)calls.size()) return false;

    frame.task = task;
    frame.index = index;
    frame.place = calls[index];

    // the innermost frame's slots start at task->fp; an outer one's start was put
    // away in frames[] when it made its call (virtual_machine.cpp, return_method)
    if(index == 0) frame.slots = task->fp;
    else frame.slots = task->stack + task->frames[task->callFramePtr - (index - 1)].fp;
    return true;
}

// ---------------------------------------------------------------------------------
// Making a held_value from the place it lives in
// ---------------------------------------------------------------------------------

static held_value reference_at(object *slot) {
    held_value value;
    value.kind = VALUE_OBJECT;
    value.reference = slot->o;
    value.slot = slot;
    return value;
}

/** a number in a cell; `type` is how the VM knows it (data_type: type_int8 ... type_var) */
static held_value number_in(vm_cell *cell, int type) {
    held_value value;
    value.cell = cell;
    value.type = type;
    if(type == type_var) {
        value.kind = VALUE_DECIMAL;
        value.decimal = cell->d;
    } else {
        value.kind = type == type_uint64 ? VALUE_UNSIGNED : VALUE_INTEGER;
        value.integer = cell->i;
    }
    return value;
}

/** is this field a plain number (its slot holds the number, not a reference)? */
static bool is_number_field(sharp_field &field) {
    return field.type->type <= type_var && !field.isArray;
}

/**
 * A value of `kind` (a local_kind: what the program's debugger section says a
 * variable holds) that lives in `cell` when it is a number and in `slot` when
 * it is a reference.
 */
static held_value value_of_kind(int kind, vm_cell *cell, object *slot) {
    held_value value;
    switch(kind) {
        case LOCAL_OBJECT:
            return reference_at(slot);
        case LOCAL_TEXT:
            value = reference_at(slot);
            value.holdsText = true;
            return value;
        case LOCAL_BOOL_ARRAY:
            value = reference_at(slot);
            value.boolElements = true;
            return value;
        case LOCAL_DECIMAL:
            return number_in(cell, type_var);
        case LOCAL_BOOL:
            value = number_in(cell, type_uint8);
            value.kind = VALUE_BOOL;
            return value;
        case LOCAL_UINT64:
            return number_in(cell, type_uint64);
        case LOCAL_INT8:   return number_in(cell, type_int8);
        case LOCAL_INT16:  return number_in(cell, type_int16);
        case LOCAL_INT32:  return number_in(cell, type_int32);
        case LOCAL_UINT8:  return number_in(cell, type_uint8);
        case LOCAL_UINT16: return number_in(cell, type_uint16);
        case LOCAL_UINT32: return number_in(cell, type_uint32);
        default:
            return number_in(cell, type_int64);
    }
}

// the fiber whose values are being looked at: where a `localized` variable is read from (each
// fiber has its own). Set by whatever starts from a frame: value_of_path(), value_of_local().
static fiber *viewed_fiber = nullptr;

static held_value field_value(sharp_object *owner, sharp_field &field) {
    // where the field is: in the object - or, for a `localized` one, at the bottom of the fiber's stack
    vm_cell *cell = &number_slot(owner, field.address);
    object *slot = &owner->node[field.address];
    sharp_object *holder = owner;
    if(field.threadLocal && viewed_fiber != nullptr) {
        cell = &viewed_fiber->stack[field.address].var;
        slot = &viewed_fiber->stack[field.address].obj;
        holder = nullptr;
    }

    held_value value;
    if(field.debugKind == LOCAL_BOOL || field.debugKind == LOCAL_BOOL_ARRAY) {
        // a bool: to the VM a one-byte number, said to be a bool by a program built with -dbg
        value = value_of_kind(field.debugKind, cell, slot);
    } else if(is_number_field(field)) {
        value = number_in(cell, field.type->type);
    } else {
        value = reference_at(slot);
        value.holdsText = !field.isArray && field.type->type == type_class && field.type->sc == vm.string_class;
    }
    value.owner = holder;
    value.carried = field.name;
    return value;
}

/** the class an object is an instance of (null for an array and for null) */
static sharp_class* class_of(sharp_object *o) {
    if(o == nullptr || !IS_CLASS(o) || o->arrayFlag) return nullptr;
    return &vm.classes[CLASS(o->info)];
}

/** the field `name` of an object's class - a static one for a class's static object, an instance one otherwise */
static sharp_field* find_field(sharp_object *o, const string &name) {
    sharp_class *of = class_of(o);
    if(of == nullptr) return nullptr;

    for(Int i = 0; i < of->totalFieldCount; i++) {
        sharp_field &field = of->fields[i];
        if((bool)o->staticObject != check_flag(field.flags, flag_static)) continue;
        if(written_name(field, of) == name) return &field;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------------
// Locals
// ---------------------------------------------------------------------------------

std::vector<const local_data*> locals_in_view(const frame_view &frame) {
    std::vector<const local_data*> seen;
    sharp_function *function = frame.place.function;
    Int line = line_of(function, frame.place.pc);

    for(int32_t i = 0; i < function->localCount; i++) {
        const local_data &local = function->locals[i];
        if(line < local.firstLine || (local.lastLine != 0 && line > local.lastLine)) continue;

        // a variable of an inner block hides one of the same name around it: it is declared later
        bool replaced = false;
        for(const local_data *&earlier : seen) {
            if(written_name(*earlier) == written_name(local)) {
                earlier = &local;
                replaced = true;
            }
        }
        if(!replaced) seen.push_back(&local);
    }
    return seen;
}

bool is_being_declared(const frame_view &frame, const local_data &local) {
    // (a loop head's variable is on a line that runs on every turn: it has its value there)
    if((local.kind & LOCAL_OF_LOOP_HEAD) != 0) return false;
    return local.firstLine != 0 && line_of(frame.place.function, frame.place.pc) == local.firstLine;
}

bool value_of_local(const frame_view &frame, const local_data &local, held_value &value, string &problem) {
    viewed_fiber = frame.task;
    if(local.slot < 0) {
        problem = "the program does not say where `" + written_name(local) + "` is kept";
        return false;
    }

    if(local.field == -2) {
        // `localized`: this fiber's own, at the bottom of its stack
        stack_item &own = frame.task->stack[local.slot];
        value = value_of_kind(local.kind & LOCAL_KIND_MASK, &own.var, &own.obj);
        value.carried = local.name;
        return true;
    }
    stack_item &item = frame.slots[local.slot];

    if(local.field < 0) {
        // in the frame
        value = value_of_kind(local.kind & LOCAL_KIND_MASK, &item.var, &item.obj);
    } else {
        // in a closure record: the frame's slot holds the record, or the record to start from
        sharp_object *record = item.obj.o;
        for(int32_t step = 0; step < local.stepCount && record != nullptr; step++) {
            record = local.steps[step] < (Int)record->size ? record->node[local.steps[step]].o : nullptr;
        }
        if(record == nullptr || local.field >= (Int)record->size) {
            problem = "`" + written_name(local) + "` is kept for a lambda, in a record this call has not made yet";
            return false;
        }
        value = value_of_kind(local.kind & LOCAL_KIND_MASK, &number_slot(record, local.field), &record->node[local.field]);
        value.owner = record;
    }
    value.carried = local.name;
    return true;
}

// ---------------------------------------------------------------------------------
// The library's list (std#list<...>): an array with room to spare (`data`) and
// how many of its elements are in use (`n_elements`). sdb shows and indexes it
// as the list it is, not as those two fields.
// ---------------------------------------------------------------------------------

static bool is_list(sharp_object *o) {
    sharp_class *of = class_of(o);
    if(of == nullptr || o->staticObject || written_name(of).compare(0, 9, "std#list<") != 0) return false;

    // (The compiler leaves out a field no code uses: a kind of list the program never adds to
    //  has no `n_elements`. Such a list is empty. What is there must be what it is expected to be.)
    sharp_field *count = find_field(o, "n_elements");
    sharp_field *data = find_field(o, "data");
    if(count != nullptr && !is_number_field(*count)) return false;
    if(data != nullptr && is_number_field(*data)) return false;
    return true;
}

/** the array a list keeps its elements in (null: none yet) and how many of them are in use */
static sharp_object* array_of_list(sharp_object *o, Int &count) {
    count = 0;
    if(!is_list(o)) return nullptr;

    sharp_field *counted = find_field(o, "n_elements");
    sharp_field *kept = find_field(o, "data");
    if(counted == nullptr || kept == nullptr) return nullptr;

    sharp_object *data = o->node[kept->address].o;
    count = number_slot(o, counted->address).i;
    if(data == nullptr || count < 0) count = 0;
    else if(count > (Int)data->size) count = data->size;
    return data;
}

// ---------------------------------------------------------------------------------
// Paths
// ---------------------------------------------------------------------------------

/** `text` as an index of at most 9 digits, or -1 */
static Int index_in(const string &text) {
    if(text.empty() || text.size() > 9 || text.find_first_not_of("0123456789") != string::npos) return -1;
    return atoll(text.c_str());
}

/** element `index` of an array */
static bool element_of(const held_value &owner, Int index, const string &written, held_value &value, string &problem) {
    sharp_object *o = owner.kind == VALUE_OBJECT ? owner.reference : nullptr;
    if(o == nullptr) {
        problem = owner.kind == VALUE_OBJECT ? "`" + written + "` is null" : "`" + written + "` is a number, not an array";
        return false;
    }

    // a list: element n of the list is element n of the array it keeps
    Int listed = 0;
    sharp_object *kept = array_of_list(o, listed);
    if(is_list(o)) {
        if(index >= listed || kept == nullptr) {
            stringstream words;
            if(listed == 0) words << "`" << written << "` is an empty list";
            else words << "`" << written << "` has " << listed << " elements: the last is [" << listed - 1 << "]";
            problem = words.str();
            return false;
        }
        held_value inside;
        inside.reference = kept;
        inside.boolElements = owner.boolElements;
        return element_of(inside, index, written, value, problem);
    }

    if(class_of(o) != nullptr) {
        problem = "`" + written + "` is an object (" + name_of(class_of(o)) + "), not an array: name one of its fields (`print " + written + "` lists them)";
        return false;
    }
    if(index >= (Int)o->size) {
        stringstream words;
        words << "`" << written << "` has " << o->size << " elements: the last is [" << (Int)o->size - 1 << "]";
        problem = words.str();
        return false;
    }

    if(o->type <= type_var) {
        // a number array: its elements are packed, there is no cell to point at
        value = held_value();
        value.array = o;
        value.element = index;
        if(holds_decimals(o)) {
            value.kind = VALUE_DECIMAL;
            value.decimal = number_at(o, index);
        } else {
            value.kind = o->type == type_uint64 ? VALUE_UNSIGNED : VALUE_INTEGER;
            if(owner.boolElements) value.kind = VALUE_BOOL;
            value.integer = integer_at(o, index);
        }
        value.type = o->type;
    } else {
        value = reference_at(&o->node[index]);
        value.owner = o;
        value.holdsText = IS_CLASS(o) && &vm.classes[CLASS(o->info)] == vm.string_class;    // an array of texts
    }
    return true;
}

/** field `name` of an object */
static bool field_of(const held_value &owner, const string &name, const string &written, held_value &value, string &problem) {
    sharp_object *o = owner.kind == VALUE_OBJECT ? owner.reference : nullptr;
    if(o == nullptr) {
        problem = owner.kind == VALUE_OBJECT ? "`" + written + "` is null" : "`" + written + "` is a number: it has no fields";
        return false;
    }
    if(class_of(o) == nullptr) {
        problem = "`" + written + "` is an array: name an element, as in `" + written + "[0]`";
        return false;
    }

    sharp_field *field = find_field(o, name);
    if(field == nullptr) {
        problem = "`" + written + "` (" + name_of(class_of(o)) + ") has no field `" + name + "`: `print " + written + "` lists its fields";
        return false;
    }
    value = field_value(o, *field);
    return true;
}

/** the object that holds a class's static fields, as a value (its `object` is null when the class has none) */
static held_value statics_of(sharp_class *of) {
    return reference_at(&vm.staticHeap[of->address]);
}

/**
 * What the first name of a path means, seen from a frame - asked in the order
 * the language itself looks a name up: a local, a field of `self`, a static
 * field of the function's class (the module's variables, for a function
 * written outside any class), and last a class or module by name.
 */
static bool first_of_path(const frame_view &frame, const string &name, held_value &value, string &problem) {
    std::vector<const local_data*> locals = locals_in_view(frame);
    const local_data *self = nullptr;

    for(const local_data *local : locals) {
        if(written_name(*local) == name) return value_of_local(frame, *local, value, problem);
        if(local->name == "self") self = local;
    }

    // a field of `self`
    held_value owner;
    string unused;
    if(self != nullptr && value_of_local(frame, *self, owner, unused) && owner.reference != nullptr) {
        sharp_field *field = find_field(owner.reference, name);
        if(field != nullptr) {
            value = field_value(owner.reference, *field);
            return true;
        }
    }

    // a static field of the function's own class
    sharp_class *own = frame.place.function->owner;
    if(own != nullptr && vm.staticHeap[own->address].o != nullptr) {
        sharp_field *field = find_field(vm.staticHeap[own->address].o, name);
        if(field != nullptr) {
            value = field_value(vm.staticHeap[own->address].o, *field);
            return true;
        }
    }

    // a class by name: its static fields are then named with `.field`
    bool ambiguous = false;
    std::vector<sharp_class*> classes = classes_named(name, ambiguous);
    if(ambiguous) {
        problem = "`" + name + "` fits more than one class - say which, with its module:";
        for(sharp_class *one : classes) problem += "\n  " + written_name(one);
        return false;
    }
    if(!classes.empty()) {
        value = statics_of(classes[0]);
        value.carried = classes[0]->fullName;
        if(value.reference == nullptr) {
            problem = "the class " + written_name(classes[0]) + " has no static fields";
            return false;
        }
        return true;
    }

    // `module#variable`: a variable of a named module
    size_t hash = name.find_last_of('#');
    if(hash != string::npos) {
        sharp_class *globals = globals_of_module(name.substr(0, hash));
        if(globals != nullptr && vm.staticHeap[globals->address].o != nullptr) {
            sharp_field *field = find_field(vm.staticHeap[globals->address].o, name.substr(hash + 1));
            if(field != nullptr) {
                value = field_value(vm.staticHeap[globals->address].o, *field);
                value.carried = globals->fullName + "." + value.carried;
                return true;
            }
        }
    }

    if(frame.place.function->localCount == 0) {
        problem = "nothing named `" + name + "` is known here. The names of this function's variables are not in the program: "
                  "build it with -dbg and without -O";
    } else {
        problem = "nothing named `" + name + "` can be seen here (`locals` lists what can)";
    }
    return false;
}

/**
 * Where the first name of a path ends: at the first `.` or `[` - but a name
 * with a module in front (`std.io#file`, `std#list<std#string>`) has dots and
 * brackets of its own, so with a `#` the name runs from the LAST `#` on.
 */
static size_t end_of_first_name(const string &path) {
    size_t from = path.find_last_of('#');
    from = from == string::npos ? 0 : from + 1;
    size_t end = path.find_first_of(".[", from);
    return end == string::npos ? path.size() : end;
}

bool value_of_path(const frame_view &frame, const string &path, held_value &value, string &problem) {
    viewed_fiber = frame.task;
    size_t at = end_of_first_name(path);
    string written = path.substr(0, at);
    if(written.empty()) {
        problem = "a path starts with a name: `count`, `book.title`, `notes[3]`";
        return false;
    }
    if(!first_of_path(frame, written, value, problem)) return false;
    string carried = value.carried.empty() ? written : value.carried;

    while(at < path.size()) {
        held_value owner = value;

        if(path[at] == '.') {
            size_t end = path.find_first_of(".[", at + 1);
            if(end == string::npos) end = path.size();
            string name = path.substr(at + 1, end - at - 1);
            if(name.empty()) {
                problem = "a name is missing after the `.` in `" + path + "`";
                return false;
            }
            if(!field_of(owner, name, written, value, problem)) return false;
            carried += "." + value.carried;
            at = end;
        } else {
            size_t close = path.find(']', at);
            string inside = close == string::npos ? "" : path.substr(at + 1, close - at - 1);
            Int index = index_in(inside);
            if(index < 0 && !inside.empty() && inside.find_first_of("[]") == string::npos) {
                // not a number: a variable that holds one (`numbers[i]`)
                held_value counter;
                if(!value_of_path(frame, inside, counter, problem)) return false;
                if(counter.kind != VALUE_INTEGER && counter.kind != VALUE_UNSIGNED) {
                    problem = "`" + inside + "` is not a whole number, so it cannot say which element";
                    return false;
                }
                if(counter.integer < 0) {
                    problem = "`" + inside + "` is " + text_of(counter) + ": elements are counted from 0";
                    return false;
                }
                index = counter.integer;
            }
            if(index < 0) {
                problem = "an element is named by its number, or by a variable that holds it: `" + written + "[0]`, `" + written + "[i]`";
                return false;
            }
            if(!element_of(owner, index, written, value, problem)) return false;
            carried += "[" + inside + "]";
            at = close + 1;
            if(at < path.size() && path[at] != '.' && path[at] != '[') {
                problem = "after `]` comes `.`, `[` or the end, in `" + path + "`";
                return false;
            }
        }
        written = path.substr(0, at);
    }
    value.carried = carried;
    return true;
}

// ---------------------------------------------------------------------------------
// Setting a value
// ---------------------------------------------------------------------------------

/** a whole number as a variable of this type keeps it: one too large wraps round */
static int64_t narrowed(int64_t number, int type) {
    switch(type) {
        case type_int8:   return (int8_t)number;
        case type_int16:  return (int16_t)number;
        case type_int32:  return (int32_t)number;
        case type_uint8:  return (uint8_t)number;
        case type_uint16: return (uint16_t)number;
        case type_uint32: return (uint32_t)number;
        default:          return number;
    }
}

/** what stands between the quotes of `"..."`, with \n \t \r \" \\ read as in a program; false when it is not a text in quotes */
static bool text_in_quotes(const string &written, string &characters) {
    if(written.size() < 2 || written[0] != '"' || written[written.size() - 1] != '"') return false;

    characters.clear();
    for(size_t i = 1; i + 1 < written.size(); i++) {
        char c = written[i];
        if(c == '\\' && i + 2 < written.size()) {
            char escaped = written[++i];
            switch(escaped) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                default:  c = escaped; break;
            }
        }
        characters += c;
    }
    return true;
}

/**
 * Give a variable that holds a text a new one. The text is a new object, and
 * only a thread of the program can make one: one of the held threads does.
 */
static bool set_text(held_value &value, const string &characters, string &problem) {
    if(value.slot == nullptr) {
        problem = "this is not a variable, a field or an element: there is nothing to set";
        return false;
    }

    // what kind of text the variable holds: a `string`, or the plain `_int8[]` - told by what is
    // there now. When it is null, a `string`: that is what a text variable nearly always is.
    sharp_object *now = value.reference;
    bool plainArray = now != nullptr && class_of(now) == nullptr && now->type == type_int8;
    bool isText = value.holdsText || plainArray || (now != nullptr && class_of(now) == vm.string_class);
    if(!isText) {
        // (a text put where the program expects another kind of object would break the program)
        if(now != nullptr) problem = "it holds " + text_of(value) + ", not a text";
        else problem = "it is null, and nothing says that it is a variable for a text";
        return false;
    }

    string failure;
    bool ran = sdb_run_on_held_thread([&] {
        try {
            sharp_object *array = create_object((Int)characters.size(), type_int8);
            for(size_t i = 0; i < characters.size(); i++) set_integer_at(array, i, characters[i]);

            sharp_object *made = array;
            if(!plainArray) {
                // (until the text is stored below, nothing but this thread knows the two new
                //  objects: it runs no instruction in between, so no collection can come by)
                made = create_object(vm.string_class);
                object *data = resolve_field("data", made);
                if(data != nullptr) {
                    store_object(data, array);
                    write_barrier(made, array, data);
                }
            }
            store_object(value.slot, made);
            write_barrier(value.owner, made, value.slot);
            value.reference = made;
        } catch(vm_exception &err) {
            failure = "the text could not be made (out of memory?)";
        }
    });

    if(!ran) {
        problem = "no thread of the program is stopped at an instruction, and a text is a new object that one of them must make";
        return false;
    }
    if(!failure.empty()) {
        problem = failure;
        return false;
    }
    return true;
}

bool set_value(held_value &value, const string &text, string &problem) {
    if(value.kind == VALUE_OBJECT) {
        string characters;
        if(text_in_quotes(text, characters)) return set_text(value, characters, problem);

        if(text != "null") {
            problem = "an object can be set to null, and a text to a text in quotes (\"...\"): sdb copies no objects and makes no others";
            return false;
        }
        if(value.slot == nullptr) {
            problem = "this is not a variable, a field or an element: there is nothing to set";
            return false;
        }
        store_object(value.slot, (sharp_object*)nullptr);
        value.reference = nullptr;
        return true;
    }

    // the number that is wanted
    double decimal = 0;
    int64_t whole = 0;
    bool isWhole = true;
    if(value.kind == VALUE_BOOL) {
        if(text != "true" && text != "false") {
            problem = "a bool is set to true or false";
            return false;
        }
        whole = text == "true" ? 1 : 0;
    } else {
        char *end = nullptr;
        decimal = strtod(text.c_str(), &end);
        if(text.empty() || *end != 0) {
            problem = "`" + text + "` is not a number";
            return false;
        }
        isWhole = text.find_first_of(".eE") == string::npos;
        whole = isWhole ? strtoll(text.c_str(), nullptr, 10) : (int64_t)decimal;
        if(value.kind != VALUE_DECIMAL && !isWhole) {
            problem = "this is a whole number: it cannot take `" + text + "`";
            return false;
        }
    }

    // ... written where the value lives
    if(value.kind == VALUE_DECIMAL) {
        if(value.cell != nullptr) value.cell->d = decimal;
        else if(value.array != nullptr) set_number_at(value.array, value.element, decimal);
        value.decimal = decimal;
    } else {
        whole = narrowed(whole, value.type);
        if(value.cell != nullptr) value.cell->i = whole;
        else if(value.array != nullptr) set_integer_at(value.array, value.element, whole);
        value.integer = whole;
    }
    return true;
}

// ---------------------------------------------------------------------------------
// Saying what a value holds
// ---------------------------------------------------------------------------------

/** a decimal the way a Sharp program prints it: whole numbers without a point */
static string decimal_text(double number) {
    if(std::isnan(number)) return "NaN";
    if(std::isinf(number)) return number > 0 ? "Infinity" : "-Infinity";

    // a whole number: its digits, no point and no exponent (400, not 4e+02)
    if(number == std::floor(number) && std::fabs(number) < 1e15) {
        stringstream whole;
        whole << (int64_t)number;
        return whole.str();
    }

    // the shortest of 15, 16 and 17 digits that reads back as the same number
    for(int digits = 15; digits <= 17; digits++) {
        stringstream words;
        words << std::setprecision(digits) << number;
        if(digits == 17 || strtod(words.str().c_str(), nullptr) == number) return words.str();
    }
    return "";
}

/** the characters of a text object (`std#string`), in quotes; what cannot be printed is written as `\n`, `\t`, `\x..` */
static string quoted_text(sharp_object *text) {
    sharp_field *data = find_field(text, "data");
    sharp_object *characters = data == nullptr ? nullptr : text->node[data->address].o;
    if(characters == nullptr || characters->type > type_var) return "\"\"";

    stringstream words;
    words << "\"";
    Int shown = characters->size > MOST_CHARACTERS ? MOST_CHARACTERS : characters->size;
    for(Int i = 0; i < shown; i++) {
        unsigned char c = (unsigned char)integer_at(characters, i);
        if(c == '\n') words << "\\n";
        else if(c == '\t') words << "\\t";
        else if(c == '\r') words << "\\r";
        else if(c == '"') words << "\\\"";
        else if(c == '\\') words << "\\\\";
        else if(c < 32) words << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)c << std::dec;
        else words << c;
    }
    words << "\"";
    if(shown < (Int)characters->size) words << "... (" << characters->size << " characters)";
    return words.str();
}

/** the name of a number array's element type */
static const char* number_type_name(int type) {
    switch(type) {
        case type_int8: return "_int8";
        case type_int16: return "_int16";
        case type_int32: return "_int32";
        case type_int64: return "_int64";
        case type_uint8: return "_uint8";
        case type_uint16: return "_uint16";
        case type_uint32: return "_uint32";
        case type_uint64: return "_uint64";
        default: return "decimal";
    }
}

/** is this one of the library's number objects (int, long, double ...: a class around one `value`)? */
static bool is_number_object(sharp_class *of) {
    return of == vm.int_class || of == vm.char_class || of == vm.short_class || of == vm.long_class
        || of == vm.byte_class || of == vm.ushort_class || of == vm.double_class || of == vm.uint_class
        || of == vm.uchar_class || of == vm.ulong_class;
}

static string value_text(const held_value &value, bool inside);

/**
 * The first `count` elements of an array, on one line: `[10, 20, 30]`, cut
 * after MOST_ELEMENTS with `...`. An element that is itself an array or a
 * list is named, not opened (`_int64[4]`): one level is what fits a line.
 */
static string elements_text(sharp_object *array, Int count, bool boolElements) {
    held_value owner;
    owner.reference = array;
    owner.boolElements = boolElements;

    stringstream words;
    words << "[";
    for(Int i = 0; i < count && i < MOST_ELEMENTS; i++) {
        held_value element;
        string unused;
        if(i > 0) words << ", ";
        element_of(owner, i, "", element, unused);
        words << value_text(element, true);
    }
    if(count > MOST_ELEMENTS) words << ", ...";
    words << "]";
    return words.str();
}

string text_of(const held_value &value) {
    return value_text(value, false);
}

/** text_of(); `inside`: the value is an element being shown on its array's line */
static string value_text(const held_value &value, bool inside) {
    stringstream words;

    switch(value.kind) {
        case VALUE_INTEGER:  words << value.integer; return words.str();
        case VALUE_UNSIGNED: words << (uint64_t)value.integer; return words.str();
        case VALUE_DECIMAL:  return decimal_text(value.decimal);
        case VALUE_BOOL:     return value.integer != 0 ? "true" : "false";
        default: break;
    }

    sharp_object *o = value.reference;
    if(o == nullptr) return "null";

    sharp_class *of = class_of(o);
    if(of != nullptr) {
        if(of == vm.string_class) return quoted_text(o);
        if(is_number_object(of)) {
            sharp_field *held = find_field(o, "value");
            if(held != nullptr && is_number_field(*held)) return name_of(of) + " " + text_of(field_value(o, *held));
        }
        if(is_list(o)) {
            Int count = 0;
            sharp_object *kept = array_of_list(o, count);
            words << name_of(of) << " (" << count << ")";
            if(!inside && count > 0) words << " " << elements_text(kept, count, false);
            return words.str();
        }
        return o->staticObject ? "the static fields of " + name_of(of) + carried_note(of) : name_of(of) + carried_note(of);
    }

    // an array
    if(o->type <= type_var) words << (value.boolElements ? "bool" : number_type_name(o->type));
    else if(IS_CLASS(o)) words << name_of(&vm.classes[CLASS(o->info)]);
    else words << "object";
    words << "[" << o->size << "]";
    if(!inside && o->size > 0) words << " " << elements_text(o, o->size, value.boolElements);
    return words.str();
}

std::vector<value_part> parts_between(const held_value &value, Int from, Int count, Int &total) {
    std::vector<value_part> parts;
    total = 0;
    sharp_object *o = value.kind == VALUE_OBJECT ? value.reference : nullptr;
    if(o == nullptr) return parts;

    sharp_class *of = class_of(o);
    if(of != nullptr && !is_list(o)) {
        if(of == vm.string_class || is_number_object(of)) return parts;
        for(Int i = 0; i < of->totalFieldCount; i++) {
            sharp_field &field = of->fields[i];
            if((bool)o->staticObject != check_flag(field.flags, flag_static)) continue;
            if(!field.name.empty() && field.name[0] == '@') continue;      // the compiler's own
            if(total >= from && total < from + count) {
                value_part part;
                part.shown = written_name(field, of);
                part.name = "." + part.shown;
                part.value = field_value(o, field);
                parts.push_back(part);
            }
            total++;
        }
        return parts;
    }

    // an array, or a list: its elements
    if(of != nullptr) array_of_list(o, total);
    else total = o->size;
    for(Int i = from; i < total && i < from + count; i++) {
        value_part part;
        string unused;
        stringstream name;
        name << "[" << i << "]";
        part.name = name.str();
        part.shown = part.name;
        if(element_of(value, i, "", part.value, unused)) parts.push_back(part);
    }
    return parts;
}

bool has_parts(const held_value &value) {
    Int total = 0;
    parts_between(value, 0, 0, total);
    return total > 0;
}

string kind_of(const held_value &value) {
    switch(value.kind) {
        case VALUE_DECIMAL:  return "decimal";
        case VALUE_BOOL:     return "bool";
        case VALUE_INTEGER:
        case VALUE_UNSIGNED: return number_type_name(value.type);
        default: break;
    }

    sharp_object *o = value.reference;
    if(o == nullptr) return value.holdsText ? "std#string" : "null";
    sharp_class *of = class_of(o);
    if(of != nullptr) return name_of(of);

    // an array: what its elements are, and how many
    stringstream words;
    if(o->type <= type_var) words << (value.boolElements ? "bool" : number_type_name(o->type));
    else if(IS_CLASS(o)) words << name_of(&vm.classes[CLASS(o->info)]);
    else words << "object";
    words << "[" << o->size << "]";
    return words.str();
}

std::vector<string> parts_of(const held_value &value) {
    std::vector<string> lines;
    sharp_object *o = value.kind == VALUE_OBJECT ? value.reference : nullptr;
    if(o == nullptr) return lines;

    sharp_class *of = class_of(o);
    if(of != nullptr) {
        if(of == vm.string_class || is_number_object(of) || is_list(o)) return lines;
        for(Int i = 0; i < of->totalFieldCount; i++) {
            sharp_field &field = of->fields[i];
            if((bool)o->staticObject != check_flag(field.flags, flag_static)) continue;
            if(!field.name.empty() && field.name[0] == '@') continue;      // the compiler's own (`@static_init_flag`)
            string name = written_name(field, of);
            lines.push_back(name + carried_note(name, field.name) + " = " + text_of(field_value(o, field)));
        }
        return lines;
    }

    // an array: its elements are on its own line (text_of)
    return lines;
}
