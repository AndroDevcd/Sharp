//
// Created by BNunnally on 8/31/2021.
//

#ifndef SHARP_META_DATA_H
#define SHARP_META_DATA_H

#include "../../../stdimports.h"
#include "../frontend/parser/Ast.h"

struct sharp_file;

/**
 * This structure represents where a member i.e (class, field, etc.)
 * was defined in the code
 */
struct impl_location {
    impl_location()
    :
        file(NULL),
        line(0),
        col(0)
    {}

    impl_location(sharp_file *file, Int line, Int col)
            :
            file(file),
            line(line),
            col(col)
    {}

    impl_location(sharp_file *file, Ast *ast)
            :
            file(file),
            line(ast->line),
            col(ast->col)
    {}

    impl_location(const impl_location &loc)
            :
            file(loc.file),
            line(loc.line),
            col(loc.col)
    {}

    sharp_file* file;
    Int line;
    Int col;
};

void print_impl_location(string name, string memberType, impl_location&);

#endif //SHARP_META_DATA_H
