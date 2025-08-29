//
// Created by BNunnally on 8/31/2021.
//
#include "meta_data.h"
#include "types/sharp_class.h"
#include "../compiler_info.h"
#include "../sharp_file.h"

void print_impl_location(string name, string memberType, impl_location& location) {
    stringstream note;
    note << "in file: " << location.file->name
        << ":" << location.line << ":" << location.col
        << ": note: " << memberType;

    if(!name.empty())
        note << " `" << name << "`";
    note << " previously defined here" << endl;

    note << "\t\t" << location.file->p->getLines().at(location.line - 1)
        << endl << "\t\t";

    for(int i = 0; i < location.col-1; i++)
        note << " ";
    note << "^" << endl;

    GUARD(globalLock)
    cout << note.str();
}
