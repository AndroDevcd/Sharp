//
// Created by BNunnally on 9/5/2020.
//
#include "fiber.h"
#include "Thread.h"
#include "VirtualMachine.h"

uInt fiber::fibId=0;

fiber* fiber::makeFiber(string name, Method* main) {
    // todo: check if space available before allocation
   fiber *fib = (fiber*)malloc(sizeof(fiber));
   fib->id = fibId++;
   fib->name.init();
   fib->name = name;
   fib->main = main;
   fib->cache=NULL;
   fib->pc=NULL;
   fib->pc=NULL;
   fib->state=FIB_SUSPENDED;
   fib->exitVal=0;
   fib->exceptionObject.object=NULL;
   fib->calls=-1;
   fib->stackLimit = internalStackSize;
   fib->registers = (double*)calloc(REGISTER_SIZE, sizeof(double));
   fib->dataStack = (StackElement *) calloc(internalStackSize, sizeof(StackElement));
   fib->callStack = (Frame*)__calloc(internalStackSize, sizeof(Frame));
   fib->fp = &fib->dataStack[vm.manifest.threadLocals];
   fib->sp = (&fib->dataStack[vm.manifest.threadLocals]) - 1;

    gc.addMemory(sizeof(Frame) * internalStackSize);
    gc.addMemory(sizeof(StackElement) * internalStackSize);
    gc.addMemory(sizeof(double) * REGISTER_SIZE);
    return fib;
}

void fiber::free() {
    if(dataStack != NULL) {
        gc.freeMemory(sizeof(StackElement) * stackLimit);
        StackElement *p = dataStack;
        for(size_t i = 0; i < stackLimit; i++)
        {
            if(p->object.object) {
                DEC_REF(p->object.object);
                p->object.object=NULL;
            }
            p++;
        }

        std::free(this->dataStack); dataStack = NULL;
    }

    if(registers != NULL) {
        gc.freeMemory(sizeof(double) * REGISTER_SIZE);
        std::free(registers); registers = NULL;
    }

    if(callStack != NULL) {
        gc.freeMemory(sizeof(Frame) * internalStackSize);
        std::free(callStack); callStack = NULL;
    }

    fp = NULL;
    sp = NULL;
    name.free();
    id = -1;
}
