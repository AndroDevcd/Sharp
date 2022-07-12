//
// Created by bknun on 6/2/2022.
//

#ifndef SHARP_COMPILER_H
#define SHARP_COMPILER_H

class sharp_class;
class Ast;

void __compile__();
void compile_class(sharp_class* parentClass, sharp_class *with_class, Ast *ast);
bool validate_control_paths(bool *controlPaths);

#define CONTROL_PATH_SIZE 8
#define MAIN_CONTROL_PATH 0
#define IF_CONTROL_PATH 1
#define ELSEIF_CONTROL_PATH 2
#define ELSE_CONTROL_PATH 3
#define TRY_CONTROL_PATH 4
#define CATCH_CONTROL_PATH 5
#define WHEN_CONTROL_PATH 6
#define WHEN_ELSE_CONTROL_PATH 7

#endif //SHARP_COMPILER_H
