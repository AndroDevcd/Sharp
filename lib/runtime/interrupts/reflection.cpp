//
// System interrupts for reflection: look at the call stack, and what std.reflect
// (lib/support std/reflect) is made of.
//
// THE VM KNOWS NO CLASS OF std.reflect. It answers questions about the program
// with plain numbers and texts - "class 12: its base, its flags, how many fields"
// - and the library builds its class_info, field_info and function_info objects
// from the answers, the first time each is asked for. Nothing is built when a
// program starts.
//
//      reflect_facts       numbers about the program, a class, a field, a function
//      reflect_text        a name
//      reflect_class_of    what an object is
//      reflect_find        the class with a full name
//      reflect_new         an object or an array of a class given by its number
//      reflect_get         a field's value (also a static field's)
//      reflect_put         give a field a value, checked against its type
//      reflect_spread      a list of arguments laid out on the stack for a call
//      reflect_box         the result of that call as an object
//
// A class is named by its number (its place in vm.classes, sharp_class.address),
// a function by its place in vm.methods, a field by the class's number and its
// place in the class's field list (the instance fields first, then the static
// ones).
//
// See interrupt_internal.h for what a handler is.
//
#include "interrupt_internal.h"
#include "../memory/type_check.h"

//
// get_frame_info(): build and push a StackState object - two arrays, one of the
// method ids and one of the program counters, one entry per live call frame.
//
void op_get_frame_info(Int interrupt) {
    auto task = thread_self->task;
    grow_stack
    stack_overflow_check
    auto info = get_frame_info(task);
    auto stackState = create_object(vm.stack_sate);
    auto methodsField = resolve_field("methods", stackState);
    auto pcField = resolve_field("pc", stackState);
    Int iter = 0;

    copy_object(methodsField, create_object(info.size(), type_int64));
    copy_object(pcField, create_object(info.size(), type_int64));
    for(KeyPair<Int, Int> &frameInfo : info) {
        set_integer_at(methodsField->o, iter, frameInfo.key);
        set_integer_at(pcField->o, iter++, frameInfo.value);
    }

    copy_object(&push_stack_object, stackState);
}

//
// get_stack_trace(): fill in the current exception's stack trace from the live
// frames (used when an error is being raised).
//
void op_get_stack_trace(Int interrupt) {
    fill_stack_trace_from_frame_info();
}

//
// the class with a number; an exception for a number that is no class's
//
static sharp_class* reflected_class(Int id) {
    if(id < 0 || id >= (Int)vm.mf.classes)
        throw vm_exception(vm.ill_state_except, "reflection: there is no class number " + std::to_string(id));
    return vm.classes + id;
}

static sharp_function* reflected_function(Int address) {
    if(address < 0 || address >= (Int)vm.mf.methods)
        throw vm_exception(vm.ill_state_except, "reflection: there is no function number " + std::to_string(address));
    return vm.methods + address;
}

static sharp_field& reflected_field(sharp_class *sc, Int index) {
    if(index < 0 || index >= sc->totalFieldCount)
        throw vm_exception(vm.ill_state_except, "reflection: `" + sc->fullName + "` has no field number " + std::to_string(index));
    return sc->fields[index];
}

/** the three numbers that say what a type is: its kind (data_type), the class's number or -1, array or not */
static void type_facts(std::vector<Int> &out, sharp_type *type, bool isArray) {
    out.push_back(type ? (Int)type->type : (Int)type_nil);
    out.push_back(type && type->type == type_class && type->sc ? (Int)type->sc->address : -1);
    out.push_back(isArray ? 1 : 0);
}

//
// reflect_facts(kind, a, b): numbers about the program, as an array of whole
// numbers. What `a` and `b` mean and what comes back, by `kind`:
//
//      0 the program               [ classes, functions ]
//      1 class a                   [ base, owner, flags, instance fields, static fields, functions, interfaces,
//                                    then the number of each interface ]             (-1: no base, no owner)
//      2 field b of class a        [ flags, kind, class, array, static, the class that declares it ]
//      3 the functions of class a  [ the number of each ]
//      4 function a                [ flags, what it is (function_type), owner, parameters,
//                                    result kind, result class, result array, the family it overrides or -1,
//                                    then kind, class, array for each parameter ]
//
void op_reflect_facts(Int interrupt) {
    auto task = thread_self->task;
    Int b = (Int)(task->sp--)->var.i;
    Int a = (Int)(task->sp--)->var.i;
    Int kind = (Int)(task->sp)->var.i;
    std::vector<Int> out;

    switch(kind) {
        case 0:
            out.push_back((Int)vm.mf.classes);
            out.push_back((Int)vm.mf.methods);
            break;
        case 1: {
            sharp_class *sc = reflected_class(a);
            out.push_back(sc->base && sc->base != sc ? (Int)sc->base->address : -1);
            out.push_back(sc->owner ? (Int)sc->owner->address : -1);
            out.push_back((Int)sc->flags);
            out.push_back(sc->instanceFields);
            out.push_back(sc->staticFields);
            out.push_back(sc->methodCount);
            out.push_back(sc->interfaceCount);
            for(Int i = 0; i < sc->interfaceCount; i++) out.push_back((Int)sc->interfaces[i]->address);
            break;
        }
        case 2: {
            sharp_class *sc = reflected_class(a);
            sharp_field &field = reflected_field(sc, b);
            out.push_back((Int)field.flags);
            type_facts(out, field.type, field.isArray);
            out.push_back(b >= sc->instanceFields ? 1 : 0);
            out.push_back(field.owner ? (Int)field.owner->address : a);
            break;
        }
        case 3: {
            sharp_class *sc = reflected_class(a);
            for(Int i = 0; i < sc->methodCount; i++) out.push_back((Int)sc->methods[i]->address);
            break;
        }
        case 4: {
            sharp_function *fn = reflected_function(a);
            out.push_back((Int)fn->flags);
            out.push_back((Int)fn->fnType);
            out.push_back(fn->owner ? (Int)fn->owner->address : -1);
            out.push_back(fn->paramSize);
            type_facts(out, fn->returnType, fn->arrayReturn);
            out.push_back((Int)fn->delegateAddress);
            for(Int i = 0; i < fn->paramSize; i++) type_facts(out, fn->params[i].type, fn->params[i].isArray);
            break;
        }
        default:
            throw vm_exception(vm.ill_state_except, "reflection: there is no kind of facts number " + std::to_string(kind));
    }

    auto made = create_object((Int)out.size(), type_int64);
    for(size_t i = 0; i < out.size(); i++) set_integer_at(made, (Int)i, out[i]);
    copy_object(&task->sp->obj, made);
}

//
// reflect_text(kind, a, b): a name, as characters.
//
//      0 class a: its name             1 class a: its full name (module#name)
//      2 field b of class a: its name  3 function a: its name      4 function a: its full name
//
void op_reflect_text(Int interrupt) {
    auto task = thread_self->task;
    Int b = (Int)(task->sp--)->var.i;
    Int a = (Int)(task->sp--)->var.i;
    Int kind = (Int)(task->sp)->var.i;
    const string *text;

    switch(kind) {
        case 0: text = &reflected_class(a)->name; break;
        case 1: text = &reflected_class(a)->fullName; break;
        case 2: text = &reflected_field(reflected_class(a), b).name; break;
        case 3: text = &reflected_function(a)->name; break;
        case 4: text = &reflected_function(a)->fullName; break;
        default:
            throw vm_exception(vm.ill_state_except, "reflection: there is no kind of text number " + std::to_string(kind));
    }

    auto made = create_object((Int)text->size(), type_int8);
    for(size_t i = 0; i < text->size(); i++) set_integer_at(made, (Int)i, (*text)[i]);
    copy_object(&task->sp->obj, made);
}

//
// reflect_class_of(object): what an object is, as [ kind (data_type), the class's
// number or -1, array or not, its size ]. For null: [ -1, -1, 0, 0 ].
//
void op_reflect_class_of(Int interrupt) {
    auto task = thread_self->task;
    auto o = task->sp->obj.o;
    auto made = create_object(4, type_int64);

    set_integer_at(made, 0, o ? (Int)o->type : -1);
    set_integer_at(made, 1, o && IS_CLASS(o) ? (Int)CLASS(o->info) : -1);
    set_integer_at(made, 2, o && o->arrayFlag ? 1 : 0);
    set_integer_at(made, 3, o ? (Int)o->size : 0);
    copy_object(&task->sp->obj, made);
}

//
// reflect_find(name): the number of the class with this full name ("std#string",
// "app.models#user"), as [ number ]; [ -1 ] when the program has no such class.
//
void op_reflect_find(Int interrupt) {
    auto task = thread_self->task;
    string name = read_string_value(task->sp->obj.o);
    sharp_class *found = locate_class(name.c_str());
    auto made = create_object(1, type_int64);

    set_integer_at(made, 0, found ? (Int)found->address : -1);
    copy_object(&task->sp->obj, made);
}

//
// reflect_new(class, count): make what a program can only name at run time.
//
//      count < 0       one object of the class: its fields empty, NO constructor
//                      run - the caller runs one, as `new` would
//      count >= 0      an array with room for `count` objects of the class
//
void op_reflect_new(Int interrupt) {
    auto task = thread_self->task;
    Int count = (Int)(task->sp--)->var.i;
    sharp_class *made = reflected_class((Int)(task->sp)->var.i);

    copy_object(&task->sp->obj, count >= 0 ? create_object(made, count) : create_object(made));
}

/** a number as an object of the library's number class for its kind (int, long, double ...): what a program can hold as `object` */
static sharp_object* boxed_number(data_type kind, vm_cell cell) {
    sharp_class *box;
    double value = kind == type_var ? cell.d : (double)cell.i;

    switch(kind) {
        case type_int8:   box = vm.char_class; break;
        case type_int16:  box = vm.short_class; break;
        case type_int32:  box = vm.int_class; break;
        case type_uint8:  box = vm.uchar_class; break;
        case type_uint16: box = vm.ushort_class; break;
        case type_uint32: box = vm.uint_class; break;
        case type_uint64: box = vm.ulong_class; break;
        case type_var:    box = vm.double_class; break;
        default:          box = vm.long_class; break;
    }
    if(box == nullptr)
        throw vm_exception(vm.ill_state_except, "reflection: this program was built without the number classes of the library (std#int, std#double ...)");

    auto made = create_object(box);
    assign_numeric_class_field(made, value);
    return made;
}

/** the number in what a program hands over for a number: an object of a number class (its `value`), or a number array (its first item) */
static double unboxed_number(sharp_object *value, const string &what) {
    if(value != nullptr && value->type <= type_var && value->size >= 1) return number_at(value, 0);
    if(value != nullptr && IS_CLASS(value) && !value->arrayFlag) {
        sharp_class *sc = vm.classes + CLASS(value->info);
        for(Int i = 0; i < sc->instanceFields; i++) {
            if(sc->fields[i].name == "value" && sc->fields[i].type->type <= type_var && !sc->fields[i].isArray) {
                vm_cell cell = number_slot(value, i);
                return sc->fields[i].type->type == type_var ? cell.d : (double)cell.i;
            }
        }
    }
    throw vm_exception(vm.ill_state_except, "reflection: " + what + " takes a number and was given "
        + (value == nullptr ? string("null") : IS_CLASS(value) ? "a `" + vm.classes[CLASS(value->info)].fullName + "`" : string("an array")));
}

/** a number cut to the size of what holds it, as a store from bytecode cuts it */
static vm_cell sized_number(data_type kind, double number) {
    vm_cell cell;
    if(kind == type_var) { cell.d = number; return cell; }

    int64_t whole = decimal_to_integer(number);
    switch(kind) {
        case type_int8:   cell.i = (int8_t) whole; break;
        case type_uint8:  cell.i = (uint8_t) whole; break;
        case type_int16:  cell.i = (int16_t) whole; break;
        case type_uint16: cell.i = (uint16_t) whole; break;
        case type_int32:  cell.i = (int32_t) whole; break;
        case type_uint32: cell.i = (uint32_t) whole; break;
        default:          cell.i = whole; break;
    }
    return cell;
}

/** where a field's value is: the object that holds it (the class's static object for a static field) and the slot in it */
static sharp_object* field_holder(sharp_class *sc, Int index, sharp_object *instance, Int &slot) {
    sharp_field &declared = sc->fields[index];
    if(index >= sc->instanceFields) {
        slot = index - sc->instanceFields;
        sharp_object *statics = vm.staticHeap[sc->address].o;
        if(statics == nullptr)
            throw vm_exception(vm.ill_state_except, "reflection: the static fields of `" + sc->fullName + "` are not set up");
        return statics;
    }

    slot = index;
    if(instance == nullptr)
        throw vm_exception(vm.nullptr_except, "reflection: the field `" + declared.name + "` of null");
    if(!IS_CLASS(instance) || instance->arrayFlag || !are_classes_related(vm.classes + CLASS(instance->info), sc))
        throw vm_exception(vm.ill_state_except, "reflection: the field `" + declared.name + "` is one of `" + sc->fullName
            + "`, and the object is " + (IS_CLASS(instance) && !instance->arrayFlag ? "a `" + vm.classes[CLASS(instance->info)].fullName + "`" : string("not an object of a class")));
    return instance;
}

//
// reflect_get(class, field, object): the value of a field. `object` is the
// object to read it from, and is not looked at for a static field. A number
// comes as an object of its number class (int, double ...); everything else as
// what the field refers to.
//
void op_reflect_get(Int interrupt) {
    auto task = thread_self->task;
    auto instance = (task->sp--)->obj.o;
    Int index = (Int)(task->sp--)->var.i;
    sharp_class *sc = reflected_class((Int)(task->sp)->var.i);
    sharp_field &declared = reflected_field(sc, index);
    Int slot;
    sharp_object *holder = field_holder(sc, index, instance, slot);

    if(declared.type->type <= type_var && !declared.isArray)
        copy_object(&task->sp->obj, boxed_number(declared.type->type, number_slot(holder, slot)));
    else
        copy_object(&task->sp->obj, holder->node[slot].o);
}

//
// reflect_put(class, field, object, value): give a field a value.
//
//      a number field      `value` is an object of a number class or a number
//                          array; the number is cut to the field's size
//      a field of a class  `value` is null or an object of that class (or of one
//                          below it, or one that implements it)
//      an array field      `value` is null or an array: of numbers for a number
//                          array, of references for every other
//      an `object` field   anything
//
// Anything else is refused here, with the field's name: a wrong reference stored
// now is a crash somewhere else later.
//
void op_reflect_put(Int interrupt) {
    auto task = thread_self->task;
    auto value = (task->sp--)->obj.o;
    auto instance = (task->sp--)->obj.o;
    Int index = (Int)(task->sp--)->var.i;
    sharp_class *sc = reflected_class((Int)(task->sp--)->var.i);
    sharp_field &declared = reflected_field(sc, index);
    Int slot;
    sharp_object *holder = field_holder(sc, index, instance, slot);
    string what = "the field `" + sc->fullName + "." + declared.name + "`";

    if(declared.type->type <= type_var && !declared.isArray) {
        number_slot(holder, slot) = sized_number(declared.type->type, unboxed_number(value, what));
        return;
    }

    if(value != nullptr) {
        bool numbers = value->type <= type_var;
        if(declared.isArray) {
            bool wanted = declared.type->type <= type_var;
            if(numbers != wanted || (!numbers && !value->arrayFlag))
                throw vm_exception(vm.ill_state_except, "reflection: " + what + " holds an array of "
                    + (wanted ? "numbers" : "objects") + " and was given something else");
        } else if(declared.type->type == type_class) {
            if(numbers)
                throw vm_exception(vm.ill_state_except, "reflection: " + what
                    + " holds an object and was given an array of numbers (a text in quotes is one: pass a `string`)");
            // (the values of an enum are objects of std#_enum_, the class the enum is based on)
            sharp_class *wanted = declared.type->sc;
            bool enumValue = IS_CLASS(value) && !value->arrayFlag && wanted->base != nullptr
                && wanted->base->fullName == "std#_enum_" && vm.classes + CLASS(value->info) == wanted->base;
            if(!enumValue && (!IS_CLASS(value) || value->arrayFlag || !are_classes_related(vm.classes + CLASS(value->info), wanted)))
                throw vm_exception(vm.ill_state_except, "reflection: " + what + " holds a `" + declared.type->sc->fullName + "` and was given "
                    + (IS_CLASS(value) && !value->arrayFlag ? "a `" + vm.classes[CLASS(value->info)].fullName + "`" : string("an array")));
        }
    }
    holder->node[slot].o = value;
    write_barrier(holder, value, holder->node + slot);
}

//
// reflect_spread(arguments, function): lay a list of arguments out on the stack
// as a call of the function expects them, in place of the list.
//
//      stack before:   ... [ arguments : object[] or null ][ function's number ]
//      stack after:    ... [ argument 1 ] ... [ argument n ]
//
// The list has one item for each parameter, and before them the object the
// function is called on when it is a function of an object (a constructor
// too). A number parameter takes an object of a number class or a number
// array, as reflect_put does. `calld n` follows (std/reflect/calling.sharp),
// with the function value that sits under the list.
//
void op_reflect_spread(Int interrupt) {
    auto task = thread_self->task;
    sharp_function *fn = reflected_function((Int)(task->sp--)->var.i);
    sharp_object *arguments = (task->sp--)->obj.o;
    Int given = arguments ? (Int)arguments->size : 0;
    Int before = given - fn->paramSize;         // 1: the object the function is called on comes first

    if(arguments != nullptr && (arguments->type <= type_var || !arguments->arrayFlag))
        throw vm_exception(vm.ill_state_except, "reflection: the arguments of a call have to be an array of objects");
    if(before < 0 || before > 1)
        throw vm_exception(vm.ill_state_except, "reflection: `" + fn->fullName + "` takes " + std::to_string(fn->paramSize)
            + " argument(s) and was given " + std::to_string(given));

    grow_stack_for(given + 1)
    stack_overflow_check_for(given + 1)
    for(Int i = 0; i < given; i++) {
        sharp_object *argument = arguments->node[i].o;
        function_param *param = i < before ? nullptr : fn->params + (i - before);

        task->sp++;
        if(param != nullptr && param->type->type <= type_var && !param->isArray) {
            task->sp->var = sized_number(param->type->type,
                unboxed_number(argument, "parameter " + std::to_string(i - before + 1) + " of `" + fn->fullName + "`"));
            task->sp->obj.o = nullptr;
            TC(TC_ST(task->sp) = param->type->type == type_var ? tk_dec : tk_int;)
        } else {
            task->sp->obj.o = argument;
            TC(TC_ST(task->sp) = tk_any;)
        }
    }
}

//
// reflect_box(function): the result of the call that has just returned, as an
// object, on the stack.
//
//      the function gives a number     the number on the stack becomes an object of its number class
//      it gives a reference            it stays as it is
//      it gives nothing                null is pushed
//
void op_reflect_box(Int interrupt) {
    auto task = thread_self->task;
    sharp_function *fn = reflected_function((Int)(task->sp--)->var.i);
    data_type kind = fn->returnType ? fn->returnType->type : type_nil;

    if(kind <= type_var && !fn->arrayReturn) {
        copy_object(&task->sp->obj, boxed_number(kind, task->sp->var));
    } else if(kind != type_object && kind != type_class && kind != type_function_ptr && !fn->arrayReturn) {
        grow_stack
        stack_overflow_check
        copy_object(&push_stack_object, (sharp_object*)nullptr);
    }
}
