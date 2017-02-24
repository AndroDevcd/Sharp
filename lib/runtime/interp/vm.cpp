//
// Created by BraxtonN on 2/17/2017.
//

#include "../internal/Environment.h"
#include "vm.h"
#include "../internal/Exe.h"
#include "../internal/Thread.h"

SharpVM* vm;
Environment* env;

int CreateSharpVM(SharpVM** pVM, Environment** pEnv, std::string exe, std::list<string> pArgs)
{
    updateStackFile("Creating virtual machine:");
    vm = new SharpVM();
    env = new Environment();

    *pVM = vm;
    *pEnv = env;

    if(Process_Exe(exe) != 0)
        return 1;

    Thread::Startup();

    return 0;
}

void SharpVM::DestroySharpVM() {

}

void SharpVM::DetatchCurrentThread() {

}
