//
// Created by BNunnally on 8/31/2021.
//
#include "meta_data.h"
#include "types/sharp_class.h"
#include "../compiler_info.h"

void print_impl_location(sharp_class* sc, impl_location& location) {
    stringstream note;
    note << "in file: " << location.file->name
        << ":" << location.line << ":" << location.col
        << ": note: class `" << sc->name << "` previously defined here" << endl;

    note << "\t\t" << location.file->tok->getLines().at(location.line)
        << endl << "\t\t";

    for(int i = 0; i < location.col-1; i++)
        note << " ";
    note << "^" << endl;

    GUARD(globalLock)
    cout << note.str();
}