//
// Created by BraxtonN on 1/31/2017.
//

#ifndef SHARP_ACCESSMODIFIER_H
#define SHARP_ACCESSMODIFIER_H

enum AccessModifier
{
    /*
     * Class and Field access modifiers
     */
    mPublic,
    mPrivate,
    mProtected,

    /*
     * Field access modifiers
     */
    mConst,
    mStatic, /* Methods can use this modifier */

    /*
     * Function access modifiers
     */
    mOverride,
};

#endif //SHARP_ACCESSMODIFIER_H
