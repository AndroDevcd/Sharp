#include <iostream>
#include "lib/grammar/parser/tokenizer/tokenizer.h"
#include "lib/grammar/parser/parser.h"
#include <chrono>
#include "lib/util/file.h"
#include "lib/grammar/runtime.h"

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

class CharStream;

void compile(const char* file)
{
    string code = file::read_alltext(file);
    if(code == "") return;

    int_errs();
    tokenizer tokenizer1(code, file);

//    for(token_entity &entity : *tokenizer1.getentities())
//    {
//        cout << "entity " << entity.getid() << ", type " << entity.gettokentype() << " :  " << entity.gettoken().c_str() << endl;
//    }

    if(tokenizer1.geterrors()->_errs())
    {
        cout << tokenizer1.geterrors()->getall_errors();
    }

    parser p(&tokenizer1);

    if(p.parsed)
    {
        if(p.geterrors()->_errs())
        {
            cout << p.geterrors()->getall_errors();
            //cout << endl << endl << "#################################################################\n";
            //cout << p.geterrors()->getuo_errors();
            cout << endl << endl << "==========================================================\n" ;
            cout << "Errors: " << p.geterrors()->error_count() << " Unoptimized errors: " << p.geterrors()->uoerror_count() << endl;
        }
        else {
            cout << "runtime starting.." << endl;
            list<parser*> parsers;
            parsers.push_back(&p);

            runtime rt("", parsers);
            rt.cleanup();
        }





        p.free();
    }

    tokenizer1.free();


}

int main() {
    std::cout << "compile time " << measure<>::execution(
            compile, "examples\\example2.sharp"
    ) << "ms" << std::endl;
    return 0;
}