//
// Created by BNunnally on 9/6/2021.
//

#include "sharp_alias.h"
#include "sharp_class.h"

void set_full_name(sharp_alias *alias) {
    alias->fullName = alias->owner->fullName + "." + alias->name;
}