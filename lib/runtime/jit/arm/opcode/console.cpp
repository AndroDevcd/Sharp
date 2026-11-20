//
// put, putc, get, sleep (jit/x64/opcode/console.cpp): the runtime prints, reads and waits.
//
#include "../arm_generator.h"

namespace armjit {

emit_result ArmGenerator::emit_console() {
    switch(op) {
        case Opcode::PUT:       // cout << regs[a].i
            load_register(V0, single);
            m.call(reinterpret_cast<void*>(jit_rt_put_integer), { Arg::value(V0) });
            return emitted;
        case Opcode::DPUT:      // cout << regs[a].d
            load_decimal(D0, single);
            m.call(reinterpret_cast<void*>(jit_rt_put_decimal), { Arg::decimal(D0) });
            return emitted;
        case Opcode::PUTC:      // printf("%c", regs[a])
            load_register(V0, single);
            m.call(reinterpret_cast<void*>(jit_rt_put_character), { Arg::value(V0) });
            return emitted;
        case Opcode::GET:       // regs[a] = one character of the input (regs[CMT]: shown as it is typed)
            load_register(V0, CMT);
            m.call(reinterpret_cast<void*>(jit_rt_get_character), { Arg::value(V0) });
            m.result_to_value(V0);
            store_register(single, V0);
            return emitted;
        case Opcode::SLEEP:
            load_register(V0, single);
            m.call(reinterpret_cast<void*>(jit_rt_sleep), { Arg::value(V0) });
            return emitted;
        default:
            return not_mine;
    }
}

} // namespace armjit
