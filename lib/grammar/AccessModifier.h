//
// Created by BraxtonN on 2/2/2018.
//

#ifndef SHARP_ACCESSMODIFIER_H
#define SHARP_ACCESSMODIFIER_H

enum AccessModifier
{
    /*
     * Class and Field access modifiers
     */
    PUBLIC,
    PRIVATE,
    PROTECTED,

    /*
     * Field access modifiers
     */
    mCONST,
    STATIC, /* Methods can use this modifier */

    /*
     * Function access modifiers
     */
    OVERRIDE,

    mUNDEFINED
};

#endif //SHARP_ACCESSMODIFIER_H
