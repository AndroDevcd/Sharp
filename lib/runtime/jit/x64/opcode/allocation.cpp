//
// New objects. The runtime makes the object (jit_rt_new_...), compiled code
// pushes the reference.
//
// The arguments are read from the register ARRAY: prepare_runtime_call() has written
// the homes there, and rdi / rsi - the argument registers - are homes
// themselves (see "Calling C++" in x64_generator.cpp).
//

#include "../x64_generator.h"
#include "../../../memory/tracing_collector.h"
#include "../../../types/sharp_class.h"
#include <cstring>

using namespace asmjit;

namespace x64jit {

/*
 * A SMALL CLASS INSTANCE MADE BY COMPILED CODE (newclass)
 *
 * What create_object(class) does for an object that fits the pool
 * (memory/sharp_object.cpp, OBJECT POOL; memory/garbage_collector.cpp,
 * ALLOCATING WITHOUT SHARED MEMORY) is a dozen plain instructions once the
 * class is known - and here it is known when the code is made:
 *
 *      block = thread->pool[c]                 the thread's free list of that size
 *      none left                               -> the runtime (it refills the list)
 *      thread->uncountedBytes + bytes too many -> the runtime (it tells the shared counters)
 *      thread->pool[c] = block->next
 *      the header: its fields, the size, the kind, the class        (copied from a pattern object made here)
 *      block->next = thread->youngObjects; thread->youngObjects = block
 *      the fields = 0
 *
 * No C++ call, so the VM registers in their CPU registers are not saved and
 * loaded around it either. The runtime is still called when the list is empty
 * or the thread's byte count is due - about once in a thousand objects.
 *
 * Not done here (the runtime makes the object as before): a class too large for
 * the pool, a collector under -gcstress (every object is counted there), and
 * builds that count or check allocations.
 */
#if !defined(SAFE_EXECUTION) && !defined(SHARP_TELEMETRY) && !defined(SHARP_TYPE_CHECK)
#define SHARP_JIT_INLINE_NEW
#endif

/** emits the fast path for `newclass` of this class and returns true; false when it has to be the runtime's */
bool X64Generator::emit_new_class_inline(Int classAddress, Label slow, Label done) {
#ifndef SHARP_JIT_INLINE_NEW
    return false;
#else
    if(tgc_stress_is_on() || classAddress < 0 || classAddress >= (Int) vm.mf.classes) return false;
    sharp_class *sc = vm.classes + classAddress;
    size_t fields = (size_t) sc->instanceFields, total = sizeof(sharp_object) + sizeof(object) * fields;
    int c = (int)((total + 15) / 16) - 1;
    if(sc->instanceFields < 0 || c < 1 || c > POOL_CLASSES) return false;

    // the header as create_object() leaves it, read back as the two words behind `data`
    sharp_object pattern;
    memset((void*) &pattern, 0, sizeof(pattern));
    init_struct(&pattern);
    pattern.poolClass = c;
    pattern.inlineData = fields > 0 ? 1 : 0;
    pattern.size = (uint32_t) fields;
    pattern.type = type_class;
    SET_INFO(pattern.info, sc->address, gc_normal);
    static_assert(sizeof(sharp_object) == 32 && offsetof(sharp_object, data) == 0 && offsetof(sharp_object, next) == 24,
                  "the inline newclass writes the header as four words: data, two of flags, next");
    uint64_t words[4];
    memcpy(words, &pattern, sizeof(words));

    x86::Mem list = x86::qword_ptr(THREAD, (int32_t)(offsetof(sharp_thread, pool) + sizeof(void*) * c));
    x86::Mem uncounted = x86::qword_ptr(THREAD, offsetof(sharp_thread, uncountedBytes));
    x86::Mem young = x86::qword_ptr(THREAD, offsetof(sharp_thread, youngObjects));

    a.mov(rax, list);                               // block = thread->pool[c]
    a.test(rax, rax);
    a.jz(slow);
    a.mov(rcx, uncounted);                          // the bytes, unless the count is due
    a.add(rcx, (int32_t) total);                    // (what reserve_bytes counts for it: the sweep gives the same back)
    a.cmp(rcx, (int32_t) COUNT_BYTES_EVERY);
    a.jae(slow);
    a.mov(uncounted, rcx);
    a.mov(rcx, x86::qword_ptr(rax));                // thread->pool[c] = block->next
    a.mov(list, rcx);

    if(fields > 0) {                                // data: the fields are right behind the header
        a.lea(rcx, x86::qword_ptr(rax, (int32_t) sizeof(sharp_object)));
        a.mov(x86::qword_ptr(rax, 0), rcx);
    } else {
        a.mov(x86::qword_ptr(rax, 0), 0);
    }
    a.mov(rcx, words[1]);
    a.mov(x86::qword_ptr(rax, 8), rcx);
    a.mov(rcx, words[2]);
    a.mov(x86::qword_ptr(rax, 16), rcx);
    a.mov(rcx, young);                              // onto the thread's list of new objects
    a.mov(x86::qword_ptr(rax, 24), rcx);
    a.mov(young, rax);

    if(fields > 0) {                                // every field starts as zero bits
        a.xor_(x86::ecx, x86::ecx);
        for(size_t i = 0; i < fields; i++)
            a.mov(x86::qword_ptr(rax, (int32_t)(sizeof(sharp_object) + sizeof(object) * i)), rcx);
    }
    a.jmp(done);
    return true;
#endif
}

emit_result X64Generator::emit_allocation() {
    switch(op) {
        case Opcode::NEWCLASS:
        case Opcode::NEWARRAY:
        case Opcode::NEWOBJARRAY:
        case Opcode::NEWCLASSARRAY:
        case Opcode::NEWSTRING: {
            // a small class instance: made right here, the runtime only when the thread's list is empty
            Label fromRuntime = a.newLabel(), made = a.newLabel();
            bool inlined = op == Opcode::NEWCLASS && emit_new_class_inline((Int) raw, fromRuntime, made);
            if(inlined) a.bind(fromRuntime);

            prepare_runtime_call();
            void *helper;
            switch(op) {
                case Opcode::NEWCLASS:      // create_object(&vm.classes[b])
                    a.mov(rdi, (int64_t)raw);
                    helper = reinterpret_cast<void*>(jit_rt_new_class);
                    break;
                case Opcode::NEWARRAY:      // create_object(regs[a], (data_type)b)
                    a.mov(rdi, vm_register_in_memory(dual1));
                    a.mov(rsi, (int64_t)dual2);
                    helper = reinterpret_cast<void*>(jit_rt_new_number_array);
                    break;
                case Opcode::NEWOBJARRAY:   // create_object(regs[a])
                    a.mov(rdi, vm_register_in_memory(single));
                    helper = reinterpret_cast<void*>(jit_rt_new_object_array);
                    break;
                case Opcode::NEWCLASSARRAY: // create_object(vm.classes + b, regs[a])
                    a.mov(rdi, (int64_t)raw);
                    a.mov(rsi, vm_register_in_memory(single));
                    helper = reinterpret_cast<void*>(jit_rt_new_class_array);
                    break;
                default:                    // the string constant a
                    a.mov(rdi, (int64_t)single);
                    helper = reinterpret_cast<void*>(jit_rt_new_string);
                    break;
            }
            call_runtime_checked(helper);
            load_vm_registers();
            if(inlined) a.bind(made);
            a.mov(machine_frame_slot(FRAME_SCRATCH), rax);   // the push below may call a helper
            push_stack_slot();
            a.mov(rax, machine_frame_slot(FRAME_SCRATCH));
            a.mov(stack_object(), rax);
            return emitted;
        }

        default:
            return not_mine;
    }
}

} // namespace x64jit
