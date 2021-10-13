//
// Created by BNunnally on 10/10/2021.
//

#include "sharp_file.h"
#include "compiler_info.h"

void sharp_file::free()  {
    dependencies.free();
    imports.free();
    context.free();
    deleteList(importGroups);
}
