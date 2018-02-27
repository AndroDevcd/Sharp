//
// Created by BraxtonN on 2/15/2018.
//

#include "VirtualMachine.h"
#include "Exe.h"
#include "Thread.h"
#include "memory/GarbageCollector.h"
#include "register.h"
#include "Environment.h"
#include "../util/time.h"
#include "Opcode.h"

VirtualMachine* vm;
Environment* env;

int CreateVirtualMachine(std::string exe)
{
    vm = (VirtualMachine*)__malloc(sizeof(VirtualMachine)*1);
    env = (Environment*)__malloc(sizeof(Environment)*1);

    if(Process_Exe(exe) != 0)
        return 1;

    Thread::Startup();
    GarbageCollector::startup();

    /**
     * Aux classes
     */
    Field* fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, &env->classes[manifest.classes]);
    fields[1].init("stackTrace", 0, VAR, false, false, &env->classes[manifest.classes]);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#Throwable",
            fields,
            2,
            NULL,
            manifest.classes
    );
    env->Throwable = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, false, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#RuntimeErr",
            fields,
            2,
            env->Throwable,
            manifest.classes
    );
    env->RuntimeErr = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, false, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#StackOverflowErr",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->StackOverflowErr = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, false, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#ThreadStackException",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->ThreadStackException = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, false, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#IndexOutOfBoundsException",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->IndexOutOfBoundsException = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, false, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#NullptrException",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->NullptrException = &env->classes[manifest.classes++];

    fields=(Field*)malloc(sizeof(Field)*2);
    fields[0].init("message", 0, VAR, false, false, env->Throwable);
    fields[1].init("stackTrace", 0, VAR, false, false, env->Throwable);

    env->classes[manifest.classes].init();
    env->classes[manifest.classes] = ClassObject(
            "std.err#ClassCastException",
            fields,
            2,
            env->RuntimeErr,
            manifest.classes
    );
    env->ClassCastException = &env->classes[manifest.classes++];

    cout.precision(16);

    for(unsigned long i = 0; i < manifest.classes; i++) {
        env->globalHeap[i].object = NULL;
    }

    return 0;
}

void VirtualMachine::destroy() {
    if(thread_self != NULL) {
        exitVal = thread_self->exitVal;
    } else
        exitVal = 1;
    Thread::shutdown();
    GarbageCollector::self->shutdown();
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
VirtualMachine::InterpreterThreadStart(void *arg) {
    thread_self = (Thread*)arg;
    thread_self->state = THREAD_RUNNING;

    try {
        /*
         * Call main method
         */
        vm->executeMethod(thread_self->main->address);

        thread_self->exec();
    } catch (Exception &e) {
        //    if(thread_self->exceptionThrown) {
        //        cout << thread_self->throwable.stackTrace.str();
        //    }
        thread_self->throwable = e.getThrowable();
        thread_self->exceptionThrown = true;
    }

    /*
     * Check for uncaught exception in thread before exit
     */
    thread_self->exit();

    if (thread_self->id == main_threadid)
    {
        /*
        * Shutdown all running threads
        * and de-allocate all allocated
        * memory. If we do not call join()
        * to wait for all other threads
        * regardless of what they are doing, we
        * stop them.
        */
        vm->shutdown();
    }

#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return NULL;
#endif
}

void VirtualMachine::shutdown() {
    destroy();
    env->shutdown();
}

void VirtualMachine::sysInterrupt(int32_t signal) {
    switch (signal) {
        case 0x9f:
            //cout << env->strings[(int64_t )thread_self->__stack[(int64_t)__rxs[sp]--].var].value.str();
            return;
        case 0xa0:
            registers[bmr]= Clock::__os_time((int) registers[ebx]);
            return;
        case 0xa1:
            /**
             * Make an explicit call to the garbage collector. This
             * does not garuntee that it will run
             */
            GarbageCollector::self->sendMessage(
                    CollectionPolicy::GC_EXPLICIT);
            return;
        case 0xa2:
            /**
             * This should only be used in low memory conditions
             * Sending this request will freeze your entire application
             * for an unspecified amount of time
             */
            GarbageCollector::self->sendMessage(
                    CollectionPolicy::GC_LOW);
            return;
        case 0xa3:
            registers[bmr]= Clock::realTimeInNSecs();
            return;
        case 0xa4:
            registers[cmt]=Thread::start((int32_t )registers[adx]);
            return;
        case 0xa5:
            registers[cmt]=Thread::join((int32_t )registers[adx]);
            return;
        case 0xa6:
            registers[cmt]=Thread::interrupt((int32_t )registers[adx]);
            return;
        case 0xa7:
            registers[cmt]=Thread::destroy((int32_t )registers[adx]);
            return;
        case 0xa8:
            registers[cmt]=Thread::Create((int32_t )registers[adx], (unsigned long)registers[egx]);
            return;
        default:
            // unsupported
            break;
    }
}

void VirtualMachine::executeMethod(int64_t address) {
    if(address < 0 || address >= manifest.methods) {
        stringstream ss;
        ss << "could not call method @" << address << "; method not found.";
        throw Exception(ss.str());
    }

    Method* method = env->methods+address;
    if(thread_self->callStack.empty())
        thread_self->callStack.add(
                Frame(NULL, 0, 0, 0)); // for main method
    else
        thread_self->callStack.add(
                Frame(thread_self->current, thread_self->pc, (registers[sp]-method->paramSize)+method->returnVal, registers[fp]));


    thread_self->pc = 0;
    thread_self->current = method;
    thread_self->cache = method->bytecode;
    thread_self->cacheSize = method->cacheSize;
    registers[fp] = (registers[sp] - method->paramSize) + 1;
}

int VirtualMachine::returnMethod() {
    if(thread_self->callStack.size() <= 1)
        return 1;

    Frame frame = thread_self->callStack.last();

    if(thread_self->current->finallyBlocks.size() > 0)
        executeFinally(thread_self->current);

    thread_self->current = frame.last;
    thread_self->cache = frame.last->bytecode;
    thread_self->cacheSize=frame.last->cacheSize;

    thread_self->pc = frame.pc;
    registers[sp] = frame.sp;
    registers[fp] = frame.fp;
    thread_self->callStack.pop_back();
    return 0;
}

void VirtualMachine::Throw(Object *exceptionObject) {
    if(exceptionObject->object == NULL || exceptionObject->object->k == NULL) {
        cout << "object ia not a class" << endl;
        return;
    }

    thread_self->throwable.throwable = exceptionObject->object->k;
    fillStackTrace(exceptionObject);

    if(TryThrow(thread_self->current, exceptionObject)) {
        thread_self->exceptionThrown = false;
        thread_self->throwable.drop();
        startAddress = 0;
        return;
    }

    while(thread_self->callStack.size() > 1) {
        executeFinally(thread_self->current);

        /**
         * If the finally block returns while we are trying to locate where the
         * exception will be caught we give up and the exception
         * is lost forever
         */
        if(method != thread_self->current)
            return;

        returnMethod();

        if(TryThrow(thread_self->current, exceptionObject)) {
            thread_self->exceptionThrown = false;
            thread_self->throwable.drop();
            startAddress = 0;
            return;
        }
    }

    stringstream ss;
    ss << "Unhandled exception on thread " << thread_self->name.str() << " (most recent call last):\n"; ss
            << thread_self->throwable.stackTrace.str();
    ss << endl << thread_self->throwable.throwable->name.str() << " ("
       << thread_self->throwable.message.str() << ")\n";
    throw Exception(ss.str());
}

bool VirtualMachine::TryThrow(Method *method, Object *exceptionObject) {
    int64_t pc = thread_self->pc;
    if(method->exceptions.size() > 0) {
        ExceptionTable* et, *tbl=NULL;
        for(unsigned int i = 0; i < method->exceptions.size(); i++) {
            et = &method->exceptions.get(i);

            if (et->start_pc <= pc && et->end_pc >= pc)
            {
                if (tbl == NULL || et->start_pc > tbl->start_pc)
                    tbl = et;
            }
        }

        if(tbl != NULL)
        {
            Object* object = &thread_self->dataStack[(int64_t)registers[fp]+tbl->local].object;
            *object = exceptionObject;
            thread_self->pc = tbl->handler_pc;

            return true;
        }
    }

    return false;
}

void VirtualMachine::fillStackTrace(Object *exceptionObject) {
    native_string str;
    fillStackTrace(str);
    thread_self->throwable.stackTrace = str;

    if(exceptionObject->object->k != NULL) {

        Object* stackTrace = env->findField("stackTrace", exceptionObject->object);
        Object* message = env->findField("message", exceptionObject->object);

        if(stackTrace != NULL) {
            GarbageCollector::self->createStringArray(stackTrace, str);
        }
        if(message != NULL) {
            if(thread_self->throwable.native)
                GarbageCollector::self->createStringArray(message, thread_self->throwable.message);
            else if(message->object != NULL && message->object->HEAD != NULL) {
                stringstream ss;
                for(unsigned long i = 0; i < message->object->size; i++) {
                    ss << (char) message->object->HEAD[i];
                }
                thread_self->throwable.message = ss.str();
            }
        }
    }
}

void VirtualMachine::fillStackTrace(native_string &str) {
// fill message
    stringstream ss;
    Method* m = thread_self->current;
    int64_t pc = thread_self->pc, _fp=(int64_t)registers[fp];

    unsigned int pos = thread_self->callStack.size() > EXCEPTION_PRINT_MAX ? thread_self->callStack.size()
                                                                             - EXCEPTION_PRINT_MAX : 0;
    for(long i = pos; i < thread_self->callStack.size(); i++)
    {
        Frame frame = thread_self->callStack.get(i);

        ss << "\tSource ";
        if(frame.last->sourceFile != -1 && frame.last->sourceFile < manifest.sourceFiles) {
            ss << "\""; ss << env->sourceFiles[frame.last->sourceFile].str() << "\"";
        }
        else
            ss << "\"Unknown File\"";

        long long x, line=-1;
        for(x = 0; x < frame.last->lineNumbers.size(); x++)
        {
            if(frame.last->lineNumbers.get(x).pc > pc)
                break;
        }

        if(x > 0) {
            ss << ", line " << (line = frame.last->lineNumbers.get(x - 1).line_number);
        } else
            ss << ", line ?";

        ss << ", in "; ss << frame.last->name.str() << "() [0x" << std::hex << frame.last->address << "] $0x" << frame.pc  << std::dec;

        if(line != -1 && metaData.sourceFiles.size() > 0) {
            ss << getPrettyErrorLine(line, frame.last->sourceFile);
        }

        ss << "\n";
    }
}

string VirtualMachine::getPrettyErrorLine(long line, long sourceFile) {
    stringstream ss;
    line -=2;

    if(line >= 0)
        ss << endl << "\t   " << line << ":    "; ss << metaData.getLine(line, sourceFile);
    line++;

    if(line >= 0)
        ss << endl << "\t   " << line << ":    "; ss << metaData.getLine(line, sourceFile);
    line++;

    ss << endl << "\t>  " << line << ":    "; ss << metaData.getLine(line, sourceFile);
    line++;

    if(metaData.hasLine(line, sourceFile))
        ss << endl << "\t   " << line << ":    "; ss << metaData.getLine(line, sourceFile);
    line++;

    if(metaData.hasLine(line, sourceFile))
        ss << endl << "\t   " << line << ":    "; ss << metaData.getLine(line, sourceFile);
    return ss.str();
}
