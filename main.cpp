#include <iostream>
#include "lib/grammar/parser/tokenizer/tokenizer.h"
#include "lib/grammar/parser/parser.h"
#include <chrono>
#include "lib/util/file.h"

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

void compile(string code)
{
    int_errs();
    bool comp = false;
    for(int i = 0; i < 90; i++)
    {
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

        if(parser.parsed)
        {
            if(!comp && i == 89)
            {
                comp = true;
                if(parser.geterrors()->_errs())
                {
                    cout << parser.geterrors()->getall_errors();
                    cout << endl << endl << "#################################################################\n";
                    cout << parser.geterrors()->getuo_errors();
                }

                cout << endl << endl << "==========================================================\n" ;
                cout << "Errors: " << parser.geterrors()->error_count() << " Unoptimized errors: " << parser.geterrors()->uoerror_count() << endl;
            }

            parser.free();
        }

        tokenizer1.free();
    }
}

int main() {
    std::cout << "compile time " << measure<>::execution(
            compile, file::read_alltext("examples\\example2.sharp")
    ) << "ms" << std::endl;
    return 0;
}