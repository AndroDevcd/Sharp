//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_OBJECT_H
#define SHARP_OBJECT_H

class ArrayObject;
class ClassObject;

/* native object */
class Object {

};

/* Objects stored in memory */
class gc_object {
public:

    Object* obj;
    ArrayObject* arry;
    ClassObject* klass;
};

#endif //SHARP_OBJECT_H
