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
    PUBLIC=0x001,
    PRIVATE=0x002,
    PROTECTED=0x004,

    LOCAL= 0x008, /* Only the file created can access the object */

    /*
     * Field access modifiers
     */
    flg_CONST=0x010,
    STATIC=0x020, /* Methods can use this modifier */

    /*
     * Flags for classes only
     */
    STABLE = 0x40,
    UNSTABLE = 0x80,

    EXTENSION = 0X100,

    flg_UNDEFINED=0x0
};

#endif //SHARP_ACCESSMODIFIER_H
