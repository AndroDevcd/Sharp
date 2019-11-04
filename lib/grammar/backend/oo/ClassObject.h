//
// Created by BraxtonN on 10/22/2019.
//

#ifndef SHARP_CLASSOBJECT_H
#define SHARP_CLASSOBJECT_H


#include "../DataEntity.h"

class Method;
class Field;

enum class_type
{
    class_normal = 0,
    class_interface = 1,
    class_generic = 2,
    class_enum = 4,
};

class ClassObject : public DataEntity {

public:
    ClassObject(string name, string mod, long guid, List<AccessFlag> flags, Meta &meta)
    :
        DataEntity(),
        super(NULL),
        classType(class_normal)
    {
        constructors.init();
        keys.init();
        this->flags.addAll(flags);
        this->meta.copy(meta);
        this->name=name;
        this->module=mod;
        this->guid=guid;
    }

    bool match(ClassObject* klass) {
        return klass != NULL && klass->guid == guid;
    }

    void free();

    Method* getConstructor(List<Field*> params, bool checkBase);
    Field* getField(string& name, bool checkBase);
    List<Method*>& getConstructors() { return constructors; }
    int getClassType() { return classType; }
    void setClassType(int type) { classType = type; }
    void addConstructor(Method* method) {
        constructors.add(method);
    }
    void addClass(ClassObject* k) {
        classes.add(k);
    }
    void addField(Field* f) {
        fields.add(f);
    }
    void addKeys(List<string> &keys) {
        this->keys.addAll(keys);
    }

    ClassObject* getChildClass(string name) {
        for(unsigned long i = 0; i < classes.size(); i++) {
            if(classes.get(i)->name == name)
                return classes.get(i);
        }

        return NULL;
    }

private:
    int classType;
    List<Method*> constructors;
    List<ClassObject*> classes;
    List<Field*> fields;
    List<string> keys;
    ClassObject* super;
};

#define CLASS_GENERIC_MASK 0x1
#define CLASS_INTERFACE_MASK 0x2
#define CLASS_ENUM_MASK 0x4

#define IS_CLASS_NORM(x) (x == 0)
#define IS_CLASS_GENERIC(x) ((x >> 1) & CLASS_GENERIC_MASK)
#define IS_CLASS_ENUM(x) ((x >> 2) & 1UL)
#define IS_CLASS_INTERFACE(x) (x & CLASS_INTERFACE_MASK)

#endif //SHARP_CLASSOBJECT_H
