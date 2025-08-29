//
// Created by bknun on 4/28/2023.
//

#include "obfuscator.h"
#include "../../../compiler_info.h"
#include "../../types/sharp_class.h"
#include "dependency_runner.h"
#include "../../types/types.h"

void markExplicitObfuscatedClass(sharp_class *mklass) {
    bool keepClass = false;
    if(mklass->obfuscateModifier == modifier_keep) {
        mark(mklass);
        keepClass = true;
    } else if(mklass->obfuscateModifier == modifier_keep_inclusive) {
        mark(mklass);
        keepClass = true;
    }


    for(Int j = 0; j < mklass->fields.size(); j++) {
        if(keepClass || mklass->fields[j]->obfuscateModifier == modifier_keep_inclusive
            || mklass->fields[j]->obfuscateModifier == modifier_keep) {
            mark(mklass->fields.get(j));
        }
    }

    for(Int j = 0; j < mklass->functions.size(); j++) {
        if(keepClass || mklass->functions[j]->obfuscateModifier == modifier_keep_inclusive
           || mklass->functions[j]->obfuscateModifier == modifier_keep) {
            mark(mklass->functions.get(j));
        }
    }

    for(Int j = 0; j < mklass->children.size(); j++) {
        markExplicitObfuscatedClass(mklass->children.get(j));
    }
}

void markExplicitObfuscatedItems() {
    for(Int i = 0; i < classes.size(); i++) {
        auto mklass = classes.get(i);
markExplicitObfuscatedClass(mklass);
    }
}