#include <iostream>
#include <chrono>

#include "stdimports.h"
#include "lib/runtime/startup.h"
#include "lib/util/file.h"
#include "lib/runtime/internal/Exe.h"
#include "lib/runtime/interp/Opcode.h"
#include "lib/runtime/interp/register.h"
#include "lib/runtime/oo/Object.h"
//#include "lib/grammar/runtime.h"

#define WIN32_LEAN_AND_MEAN

template<typename TimeT = std::chrono::milliseconds>
struct measure
{
    template<typename F, typename ...Args>
    static typename TimeT::rep execution(F&& func, Args&&... args)
    {
        auto start = std::chrono::steady_clock::now();
        std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
        auto duration = std::chrono::duration_cast< TimeT>
                (std::chrono::steady_clock::now() - start);
        return duration.count();
    }
};

string copychars(char c, int t) {
    string s;
    int it = 0;

    while (it++ < t)
        s += c;

    return s;
}

string mi64_tostr(int64_t i64)
{
    string str;
    mi64_t mi;
    SET_mi64(mi, i64);
    i64 = GET_mi64(mi.A, mi.B);

    str+=GET_mi32w(mi.A)<0?(char)1:(char)0; str+=(char)abs(GET_mi32w(mi.A));
    str+=GET_mi32x(mi.A)<0?(char)1:(char)0; str+=(char)abs(GET_mi32x(mi.A));
    str+=GET_mi32y(mi.A)<0?(char)1:(char)0; str+=(char)abs(GET_mi32y(mi.A));
    str+=GET_mi32z(mi.A)<0?(char)1:(char)0; str+=(char)abs(GET_mi32z(mi.A));

    str+=GET_mi32w(mi.B)<0?(char)1:(char)0; str+=(char)abs(GET_mi32w(mi.B));
    str+=GET_mi32x(mi.B)<0?(char)1:(char)0; str+=(char)abs(GET_mi32x(mi.B));
    str+=GET_mi32y(mi.B)<0?(char)1:(char)0; str+=(char)abs(GET_mi32y(mi.B));
    str+=GET_mi32z(mi.B)<0?(char)1:(char)0; str+=(char)abs(GET_mi32z(mi.B));
    return str;
}

string mi64_tostr(int64_t mi64_, int64_t overflow)
{
    string str;

    str+=mi64_tostr(mi64_);
    str+=mi64_tostr(overflow);
    return str;
}

void buildExe() {
    stringstream executable;
    int64_t i;

    executable << ((char)0x0f) << "SEF";
    executable << copychars(0, 15) << ((char)0x07);
    executable << ((char)0x1b) << ((char)0x0c);

    /* manifest */
    executable << (char)0x1;
    executable << (char)0x2 << "helloworld" << (char)0x0; // application
    executable << (char)0x4 << "1.0" << (char)0x0; // version
    executable << (char)0x5 << (char)0x0; // debug
    executable << (char)0x6 << 1 << (char)0x0; // entry
    executable << (char)0x7 << 0 << (char)0x0; // methods
    executable << (char)0x8 << 2 << (char)0x0; // classes ---
    executable << (char)0x9 << 1 << (char)0x0; // fvers
    executable << (char)0x0b << 22 << (char)0x0 << endl; // isize ---
    executable << (char)0x0c << 1 << (char)0x0; // strings
    executable << (char)0x0e << 4 << (char)0x0; // base address ---
    executable << (char)0x03;

    /* Data section */
    executable << (char)0x05 << endl;

    executable << (char)0x2f; // class
    executable << "-1" << (char)0x0; // super class
    executable << "0" << (char)0x0; // claas id
    executable << "Main" << (char)0x0; // name
    executable << "0" << (char)0x0; // fields
    executable << "1" << (char)0x0; // methods

    /* method */
    executable << endl;
    executable << (char)0x4c; // method
    executable << "main" << (char)0x0; // name
    executable << "1" << (char)0x0; // id
    executable << "0" << (char)0x0; // entry
    executable << "3" << (char)0x0; // locals
    executable << endl;

    executable << (char)0x2f; // class
    executable << "-1" << (char)0x0; // super class
    executable << "2" << (char)0x0; // claas id
    executable << "A" << (char)0x0; // name
    executable << "2" << (char)0x0; // fields
    executable << "0" << (char)0x0; // methods

    /* field */
    executable << endl;
    executable << (char)0x22; // field
    executable << "klass" << (char)0x0; // name
    executable << "3" << (char)0x0; // id
    executable << 8 << (char)0x0; // type
    executable << "2" << (char)0x0; // owner
    executable << "1" << (char)0x0; // static
    executable << endl;

    /* field */
    executable << endl;
    executable << (char)0x22; // field
    executable << "nativeInt" << (char)0x0; // name
    executable << "4" << (char)0x0; // id
    executable << 0 << (char)0x0; // type
    executable << "2" << (char)0x0; // owner
    executable << "1" << (char)0x0; // static
    executable << endl;

    // string section
    executable << (char)0x02;
    executable << (char)0x1e << "0" << (char)0x0 << "Hello, World!" << (char)0x0;
    executable << endl;
    executable << (char)0x1d;

    /* Text section */
    executable << (char)0x0e;
    executable << endl;

    executable << (char)0x05; executable << mi64_tostr(SET_Di(i, MOVI, 100000000), ecx);
    executable << (char)0x05; executable << mi64_tostr(SET_Ei(i, _NOP));
    executable << (char)0x05; executable << mi64_tostr(SET_Di(i, MOVI, 0), ebx);
    executable << (char)0x05; executable << mi64_tostr(SET_Di(i, MOVI, 100000000), ecx);
    executable << (char)0x05; executable << mi64_tostr(SET_Di(i, MOVL, 3));
    executable << (char)0x05; executable << mi64_tostr(SET_Di(i, MOVI, 1), egx);
    executable << (char)0x05; executable << mi64_tostr(SET_Ci(i, NEW, abs(nativeint), 1, egx));
    executable << (char)0x05; executable << mi64_tostr(SET_Di(i, MOVL, 3));
    executable << (char)0x05; executable << mi64_tostr(SET_Di(i, MOVI, 0), adx);
    executable << (char)0x05; executable << mi64_tostr(SET_Ci(i, MOV, adx,0, 1));
    executable << (char)0x05; executable << mi64_tostr(SET_Di(i, MOVBI, 53723), 687697862);
    executable << (char)0x05; executable << mi64_tostr(SET_Ci(i, MOVX, egx,0, adx));
    executable << (char)0x05; executable << mi64_tostr(SET_Di(i, INC, ebx));
    executable << (char)0x05; executable << mi64_tostr(SET_Ci(i, LT, ebx,0, ecx));
    executable << (char)0x05; executable << mi64_tostr(SET_Di(i, MOVI, 10), adx);
    executable << (char)0x05; executable << mi64_tostr(SET_Ei(i, BRE));
    executable << (char)0x05; executable << mi64_tostr(SET_Ei(i, RET));

    executable << endl;
    executable << (char)0x1d;

    executable << endl;
    file::write("out", executable.str());
}

int main(int argc, const char* argv[]) {

//    std::cout << "compile time " << measure<>::execution(
//            _bootstrap, argc, argv
//    ) << "ms" << std::endl;

    int64_t i64 = SET_Di(i64, MOVI, 100000000);
    mi64_t instr;
    SET_mi64(instr, i64); // TODO: fix processing of data
    int64_t i2 = GET_mi64(SET_mi32(GET_mi32w(instr.A), GET_mi32x(instr.A),
                                   GET_mi32y(instr.A),GET_mi32z(instr.A)), SET_mi32(GET_mi32w(instr.B), GET_mi32x(instr.B),
                                                                                    GET_mi32y(instr.B),GET_mi32z(instr.B)));

    buildExe();

    std::cout << "vm time " << measure<>::execution(
            runtimeStart, argc, argv
    ) << "ms" << std::endl;
    return 0;
}