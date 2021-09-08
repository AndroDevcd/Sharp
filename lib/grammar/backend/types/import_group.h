//
// Created by BNunnally on 9/7/2021.
//

#ifndef SHARP_IMPORT_GROUP_H
#define SHARP_IMPORT_GROUP_H

#include "../../../../stdimports.h"
#include "../../List.h"
#include "../meta_data.h"

struct sharp_module;

struct import_group {
    import_group()
    :
        name(""),
        modules(),
        implLocation()
    {
    }

    import_group(const import_group &ig)
    :
            name(ig.name),
            modules(ig.modules),
            implLocation(ig.implLocation)
    {
    }

    import_group(string name, impl_location location)
    :
            name(name),
            modules(),
            implLocation(location)
    {
    }

    ~import_group() {
        free();
    }

    void free() {
        modules.free();
    }

    string name;
    List<sharp_module*> modules;
    impl_location implLocation;
};

import_group* create_import_group(sharp_file*, string, Ast*);

#endif //SHARP_IMPORT_GROUP_H
