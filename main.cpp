#include <iostream>
#include "stdimports.h"
#include "lib/parser/tokenizer/tokenizer.h"
#include "lib/parser/parser.h"
#include "lib/util/file.h"

int main() {
    string text = file::read_alltext("examples\\HelloWorld.sharp");

    int_errs();
    tokenizer tokenizer1(text);

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

    return 0;
}