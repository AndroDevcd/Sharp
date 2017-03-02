#include <iostream>
#include <chrono>

#include "stdimports.h"
#include "lib/runtime/startup.h"
#include "lib/util/file.h"
//#include "lib/grammar/runtime.h"

#define WIN32_LEAN_AND_MEAN

template<typename TimeT = std::chrono::seconds>
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

void buildExe() {
    stringstream executable;

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
    executable << (char)0x0b << 48 << (char)0x0 << endl; // isize ---
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
    executable << (char)0x05 << (char)0x0 << (char)0x0; // nop
    executable << (char)0x05 << (char)0x1 << (char)0x1 << "0" << (char)0x0; // push_str
    executable << (char)0x05 << (char)0x1 << (char)0x2 << 0x9f << (char)0x0; // _int

    // for loop
    executable << (char)0x05 << (char)0x1 << (char)0x4a << 0 << (char)0x0; // lload (x)
    executable << (char)0x05 << (char)0x1 << (char)0x4a << 1 << (char)0x0; // lload (max)
    executable << (char)0x05 << (char)0x1 << (char)0x4a << 2 << (char)0x0; // lload (_label)
    executable << (char)0x05 << (char)0x1 << (char)0x20 << 0 << (char)0x0; // _new3
    executable << (char)0x05 << (char)0x1 << (char)0x20 << 0 << (char)0x0; // _new3
    executable << (char)0x05 << (char)0x1 << (char)0x20 << 2 << (char)0x0; // _new3
    executable << (char)0x05 << (char)0x1 << (char)0x4a << 0 << (char)0x0; // lload (x)
    executable << (char)0x05 << (char)0x1 << (char)0x3 << 0 << (char)0x0; // pushi
    executable << (char)0x05 << (char)0x0 << (char)0x26 << (char)0x0; // istore
    executable << (char)0x05 << (char)0x1 << (char)0x4a << 1 << (char)0x0; // lload (max)
    executable << (char)0x05 << (char)0x1 << (char)0x3 << 10000000 << (char)0x0; // pushi
    executable << (char)0x05 << (char)0x0 << (char)0x26 << (char)0x0; // istore

    executable << (char)0x05 << (char)0x1 << (char)0x4a << 2 << (char)0x0; // lload (_label)
    executable << (char)0x05 << (char)0x0 << (char)0x52 << (char)0x0; // _lbl

    executable << (char)0x05 << (char)0x1 << (char)0x4a << 0 << (char)0x0; // lload (x)
    executable << (char)0x05 << (char)0x0 << (char)0x53 << (char)0x0; // iinc
    executable << (char)0x05 << (char)0x1 << (char)0x4a << 1 << (char)0x0; // lload (max)
    executable << (char)0x05 << (char)0x0 << (char)0x15 << (char)0x0; // geti
    executable << (char)0x05 << (char)0x1 << (char)0x4a << 0 << (char)0x0; // lload (x)
    executable << (char)0x05 << (char)0x0 << (char)0x15 << (char)0x0; // geti
    executable << (char)0x05 << (char)0x0 << (char)0x31 << (char)0x0; // iflt
    executable << (char)0x05 << (char)0x1 << (char)0x4a << 2 << (char)0x0; // lload (_label)
    executable << (char)0x05 << (char)0x0 << (char)0x15 << (char)0x0; // geti (remove this to figure out seg fault)
    executable << (char)0x05 << (char)0x0 << (char)0x63 << (char)0x0; // _swap
    executable << (char)0x05 << (char)0x0 << (char)0x61 << (char)0x0; // _goto_e

    executable << (char)0x05 << (char)0x1 << (char)0x3 << "1" << (char)0x0; // pushi
    executable << (char)0x05 << (char)0x0 << (char)0x4 << (char)0x0; // ret
    executable << endl;
    executable << (char)0x1d;

    executable << endl;
    file::write("out", executable.str());
}

int main(int argc, const char* argv[]) {

//    std::cout << "compile time " << measure<>::execution(
//            _bootstrap, argc, argv
//    ) << "ms" << std::endl;
    buildExe();

    std::cout << "vm time " << measure<>::execution(
            runtimeStart, argc, argv
    ) << "ms" << std::endl;
    return 0;
}