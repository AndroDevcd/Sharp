#include <iostream>
#include <chrono>

#include "stdimports.h"
#include "lib/runtime/startup.h"
#include "lib/util/file.h"
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

string copy(char c, int t) {
    string s;
    int it = 0;

    while (it++ < t)
        s += c;

    return s;
}

void buildExe() {
    stringstream executable;

    executable << ((char)0x0f) << "SEF";
    executable << copy(0, 15) << ((char)0x07);
    executable << ((char)0x1b) << ((char)0x0c);

    char c  =0x42;
    stringstream ss;
    ss<< c;
    cout << "s " << ss.str() << endl;

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