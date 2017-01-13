#include <iostream>
#include "stdimports.h"
#include "lib/parser/tokenizer/tokenizer.h"
#include "lib/parser/parser.h"
#include "lib/util/file.h"
#include <chrono>

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

void compile(string code)
{
    int_errs();
    tokenizer tokenizer1(code);

//    for(token_entity &entity : *tokenizer1.getentities())
//    {
//        cout << "entity " << entity.getid() << ", type " << entity.gettokentype() << " :  " << entity.gettoken().c_str() << endl;
//    }

    if(tokenizer1.geterrors()->_errs())
    {
        cout << tokenizer1.geterrors()->getall_errors();
    }

    parser parser(&tokenizer1);

    if(parser.geterrors()->_errs())
    {
        cout << parser.geterrors()->getall_errors();
    }
}

int main() {
    std::cout << "compile time " << measure<>::execution(
            compile, file::read_alltext("examples\\HelloWorld.sharp")
    ) << "ms" << std::endl;
    return 0;
}