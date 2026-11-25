//
// System interrupts for files, directories and paths.
//
// Moved verbatim from the old switch in virtual_machine.cpp. Two of these
// handlers serve a FAMILY of interrupts through one body, told apart by the
// flag - that is why `interrupt` is used inside them:
//   op_file_path_op   -> OP_FILE_ACCESS .. OP_READ_FILE (one string argument)
//   op_file_two_paths -> OP_RENAME_FILE, OP_WRITE_FILE  (two string arguments)
//
// See interrupt_internal.h for what a handler is.
//
#include "interrupt_internal.h"

//
// abs_path(relative): resolve a relative path to an absolute one, in place on
// the stack (the top slot is both the input string and the output string).
//
void op_abs_path(Int interrupt) {
    auto task = thread_self->task;
    auto *relPath = &task->sp->obj;

    if(relPath->o != NULL && relPath->o->type <= type_var) {
        string path, absolute;
        populate_string(path, relPath->o);

        File::resolvePath(path, absolute);
        auto strObject = create_object(absolute.size(), type_int8);

        copy_object(relPath, strObject);
        if(strObject) assign_string_field(strObject, absolute);
    } else {
        throw vm_exception(vm.nullptr_except, "");
    }
}

//
// current_directory(): push the process's working directory as a string.
//
void op_current_directory(Int interrupt) {
    auto task = thread_self->task;
    string path;
    current_directory(path);
    auto strObject = create_object(path.size(), type_int8);

    copy_object(&push_stack_object, strObject);
    assign_string_field(strObject, path);
}

//
// The one-path file operations. All take a path string on the stack; which
// operation runs is decided by the flag. Depending on the flag, EBX carries an
// extra argument in and/or the result out, or a string/array is pushed.
//
void op_file_path_op(Int interrupt) {
    auto task = thread_self->task;
    auto arry = &pop_stack_object;
    if(arry->o != NULL && arry->o->size > 0 && arry->o->type <= type_var) {
        string path;
        populate_string(path, arry->o);

        if(interrupt==OP_FILE_ACCESS)
            registers[EBX].i = check_access(path, (int)registers[EBX].i);
        else if(interrupt==OP_FILE_ATTRS)
            registers[EBX].i = get_file_attrs(path);
        else if(interrupt==OP_FILE_UPDATE_TM)
            registers[EBX].i = last_update(path, (int)registers[EBX].i);
        else if(interrupt==OP_FILE_SIZE)
            registers[EBX].i = file_size(path);
        else if(interrupt==OP_FILE_CREATE)
            create_file(path);
        else if(interrupt==OP_FILE_DELETE)
            registers[EBX].i = delete_file(path);
        else if(interrupt==OP_GET_FILES) {
            std::list<string> files;
            get_file_list(path, files);
            arry = &push_stack_object;

            if(files.size()>0) {
                copy_object(arry, create_object(vm.string_class, (Int)files.size()));

                Int iter = 0;
                for(string fileName : files) {
                    copy_object(arry->o->node + (iter), create_object(vm.string_class));
                    auto dataField = resolve_field("data", arry->o->node[iter].o);
                    auto strObject = create_object(fileName.size(), type_int8);

                    copy_object(dataField, strObject);
                    assign_string_field(strObject, fileName);
                    iter++;
                }
            } else {
                copy_object(arry, (sharp_object*) nullptr);
            }
        }
        else if(interrupt==OP_CREATE_DIR)
            registers[EBX].i = File::makeDir(path);
        else if(interrupt==OP_DELETE_DIR)
            registers[EBX].i = delete_dir(path);
        else if(interrupt==OP_UPDATE_FILE_TM)
            registers[EBX].i = update_time(path, (time_t)registers[EBX].i);
        else if(interrupt==OP_CHMOD)
            registers[EBX].i = __chmod(path, (mode_t)registers[EBX].i, (bool)registers[EGX].i, (bool)registers[ECX].i);
        else if(interrupt==OP_READ_FILE) {
            arry = &push_stack_object;
            string str;
            read_file(path, str);

            if(str.size() > 0) {
                copy_object(arry, create_object(vm.string_class));
                auto dataField = resolve_field("data", arry->o);
                auto strObject = create_object(str.size(), type_int8);

                copy_object(dataField, strObject);
                assign_string_field(strObject, str);
            } else {
                copy_object(arry, (sharp_object*) nullptr);
            }
        }
    } else
        throw vm_exception(vm.nullptr_except, "");
}

//
// The two-path file operations: rename (old -> new) and write (path + the bytes
// to write). Both strings are popped; the result goes in EBX.
//
void op_file_two_paths(Int interrupt) {
    auto task = thread_self->task;
    auto pathObj = pop_stack_object.o;
    auto newNameObj = pop_stack_object.o;

    if (pathObj != NULL && pathObj->type <= type_var && newNameObj != NULL
        && newNameObj->type <= type_var) {
        string path, rename;

        populate_string(path, pathObj);
        populate_string(rename, newNameObj);

        if(interrupt==OP_RENAME_FILE)
            registers[EBX].i = rename_file(path, rename);
        else if(interrupt==OP_WRITE_FILE) {
            File::buffer buf;
            buf.operator<<(rename); // rename will contain our actual unicode data
            registers[EBX].i = File::write(path.c_str(), buf);
            buf.end();
        }
    } else {
        throw vm_exception(vm.nullptr_except, "");
    }
}

// disk_space(which): free/total space for a disk (EBX selects which), result in EBX.
void op_disk_space(Int interrupt) {
    registers[EBX].i=disk_space((int32_t )registers[EBX].i);
}
