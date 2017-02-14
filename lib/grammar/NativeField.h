//
// Created by BraxtonN on 1/31/2017.
//

#ifndef SHARP_NATIVEFIELD_H
#define SHARP_NATIVEFIELD_H

enum NativeField
{
    fint,
    fshort,
    flong,
    fdouble,
    ffloat,
    fbool,
    fchar,
    fstring,
    fdynamic, /* Dynamic object type (Not a native type but is treated as one) */
    fvoid, /* only used as a return type for functions */
    fnof /* not a field */
};

#endif //SHARP_NATIVEFIELD_H
