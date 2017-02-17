#include <iostream>
#include <chrono>

#include "lib/runtime/startup.h"
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

int main(int argc, const char* argv[]) {

//    std::cout << "compile time " << measure<>::execution(
//            _bootstrap, argc, argv
//    ) << "ms" << std::endl;

    std::cout << "vm time " << measure<>::execution(
            runtimeStart, argc, argv
    ) << "ms" << std::endl;
    return 0;
}