//
// System interrupts about the process and the machine it runs on: do nothing,
// run a shell command, end the program, name the OS, find the program's path.
//
// Moved verbatim from the old switch in virtual_machine.cpp (OP_NOP,
// OP_SYSTEM_EXE, OP_EXIT, OP_OS_NAME, OP_PROGRAM_PATH). See interrupt_internal.h
// for what a handler is.
//
#include "interrupt_internal.h"

// For asking the OS the path of THIS binary (see op_runtime_path). Each system
// has its own call; _WIN32 is the real predefined macro (same one sni_runtime.cpp
// uses to pick LoadLibrary over dlopen).
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <climits>
#else
#include <unistd.h>
#include <climits>
#endif

// nop(): do nothing. Kept as a real interrupt so a no-op has somewhere to go.
void op_nop(Int interrupt) {
}

//
// system(command): run a command through the OS shell and wait for it. The
// command string is popped; its exit code goes in CMT. The wait is a gc-safe
// region so the collector may run while the child runs.
//
void op_system(Int interrupt) {
    auto task = thread_self->task;
    auto str = pop_stack_object.o;
    if(str != NULL && str->type <= type_var) {
        string cmd;
        populate_string(cmd, str);
        gc_safe_region waitingForCommand;
        registers[CMT].i = system(cmd.c_str());
    } else
        throw vm_exception(vm.nullptr_except, "");
}

//
// exit(code): end the program with an exit code (popped off the stack). With a
// profiling build it dumps this thread's profile first, then shuts the VM down.
//
void op_exit(Int interrupt) {
    auto task = thread_self->task;
#ifdef SHARP_PROF_
    thread_self->tprof->endtm=Clock::realTimeInNSecs();
    thread_self->tprof->profile();
    thread_self->tprof->dump();

#endif
    task->exitVal = (int)pop_stack_number.i;
    shutdown();
}

//
// os_name(): push the operating system the VM runs on, "win" or "posix". It is
// ASKED FOR, not only handed to the program's start function, because classes
// decide things by it while their static parts are set up (the file class picks
// its file system) - and that runs before the start function has stored anything.
//
void op_os_name(Int interrupt) {
    auto task = thread_self->task;
#ifdef WIN32_
    string name = "win";
#else
    string name = "posix";
#endif
    auto strObject = create_object(name.size(), type_int8);

    copy_object(&push_stack_object, strObject);
    if(strObject) assign_string_field(strObject, name);
}

//
// program_path(): push the whole path of the program file the VM is running, so
// a program can find what is installed beside it (its native libraries:
// load_library(program_folder() + "/foo")) wherever it is started from.
//
void op_program_path(Int interrupt) {
    auto task = thread_self->task;
    string absolute;
    File::resolvePath(executable, absolute);
    auto strObject = create_object(absolute.size(), type_int8);

    copy_object(&push_stack_object, strObject);
    if(strObject) assign_string_field(strObject, absolute);
}

//
// runtime_path(): push the whole path of the `sharp` RUNTIME binary itself -
// not the program it is running (that is program_path()). A library that ships
// WITH the install, like the GUI's native backend, sits beside this binary, so
// std.gui loads it from runtime_folder() no matter where the user's program is
// or where it is started from. We ask the OS for our own path (/proc/self/exe on
// Linux, GetModuleFileName on Windows, _NSGetExecutablePath on macOS); if the OS
// will not say, we fall back to how the program was found.
//
void op_runtime_path(Int interrupt) {
    auto task = thread_self->task;
    string path;
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, buf, MAX_PATH);
    if(n > 0) path.assign(buf, (size_t)n);
#elif defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if(_NSGetExecutablePath(buf, &size) == 0) path = buf;
#else
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if(n > 0) path.assign(buf, (size_t)n);
#endif
    if(path.empty())                       // last resort (very old/odd systems)
        File::resolvePath(executable, path);

    auto strObject = create_object(path.size(), type_int8);
    copy_object(&push_stack_object, strObject);
    if(strObject) assign_string_field(strObject, path);
}
