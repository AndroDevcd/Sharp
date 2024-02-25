//
// Created by BNunnally on 8/31/2021.
//

#include "sharp_module.h"
#include "../../compiler_info.h"


sharp_module* get_module(string &packageName) {
    GUARD(globalLock)
    for(Int i = 0; i < modules.size(); i++) {
        if(modules.get(i)->name == packageName)
            return modules.get(i);
    }

    return NULL;
}

sharp_module* create_module(string &packageName) {
    GUARD(globalLock)

    sharp_module *module = get_module(packageName);
    if(module != NULL) return module;

    module = new sharp_module(packageName);
    module->uid = uniqueId++;
    modules.add(module);
    return module;
}

void sharp_module::copy(const sharp_module &m) {
    free();

    uid = m.uid;
    obfuscateModifier = m.obfuscateModifier;
    classes.addAll(m.classes);
    genericClasses.addAll(m.genericClasses);
}
