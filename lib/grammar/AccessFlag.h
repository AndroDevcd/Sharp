//
// Created by BraxtonN on 2/2/2018.
//

#ifndef SHARP_ACCESSMODIFIER_H
#define SHARP_ACCESSMODIFIER_H

enum AccessFlag
{
    /*
     * Class, Interface, Field, and Method access modifiers
     */
    PUBLIC=0,
    PRIVATE=1,
    PROTECTED=2,

    LOCAL = 3, /* Only the file created can access the object */

    /*
     * Field access modifiers
     */
    flg_CONST=4,
    STATIC=5, /* Methods can use this modifier */


    flg_UNDEFINED=6
};

#endif //SHARP_ACCESSMODIFIER_H
