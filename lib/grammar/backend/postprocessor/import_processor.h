//
// Created by BNunnally on 9/8/2021.
//

#ifndef SHARP_IMPORT_PROCESSOR_H
#define SHARP_IMPORT_PROCESSOR_H

#include "../../frontend/parser/Ast.h"

struct sharp_module;

void process_imports();
void process_import(Ast*);
void process_import_item(List<sharp_module*> &, Ast*);

#endif //SHARP_IMPORT_PROCESSOR_H
