//
// Created by BNunnally on 2/26/2020.
//

#ifndef SHARP_ALIAS_H
#define SHARP_ALIAS_H

#include "DataEntity.h"

class Utype;

class Alias : public DataEntity {
public:
    Alias()
            :
            DataEntity(),
            utype(NULL)
    {
    }

    Alias(long guid, string name, ClassObject* owner, List<AccessFlag>& flags,
          Meta& meta)
            :
            DataEntity(),
            utype(NULL)
    {
        this->type = type;
        this->guid = guid;
        this->name = name;
        this->owner = owner;
        this->flags.addAll(flags);
    }

    Utype *utype;
};


#endif //SHARP_ALIAS_H
