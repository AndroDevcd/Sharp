//
// Created by BraxtonN on 10/22/2019.
//

#ifndef SHARP_CLASSOBJECT_H
#define SHARP_CLASSOBJECT_H


#include "../data/DataEntity.h"
#include "../Compiler.h"

class Method;
class Field;
class Utype;
class Alias;

enum class_type
{
    class_normal = 0,
    class_interface = 1,
    class_generic = 2,
    class_enum = 4
};

enum generic_processing_stage
{
    unprocessed,
    created,
    preprocessed,
    postprocessed,
    compiled
};

class ClassObject : public DataEntity {

public:
    ClassObject(string name, string mod, long guid, List<AccessFlag> flags, Meta &meta)
    :
        DataEntity(),
        super(NULL),
        classType(class_normal),
        processStage(unprocessed),
        globalClass(false),
        genericOwner(NULL),
        processedExtFunctions(0),
        processedMutations(0)
    {
        fields.init();
        classes.init();
        keys.init();
        functions.init();
        classMutations.init();
        keyTypes.init();
        extensionFunctions.init();
        interfaces.init();
        this->flags.addAll(flags);
        this->meta.copy(meta);
        this->name=name;
        this->module=mod;
        this->guid=guid;
        type = CLASS;
    }

    bool match(ClassObject* klass) {
        return klass != NULL && klass->guid == guid;
    }

    void free();

    size_t fieldCount();
    Method* getConstructor(List<Field*> params, bool checkBase);
    Field* getField(string name, bool checkBase);
    Field* getField(long index);
    Method* getFunction(long index);
    int getClassType() { return classType; }
    void setClassType(int type) { classType = type; }
    void setSuperClass(ClassObject* sup) { super = sup; }
    ClassObject* getSuperClass() { return super; }
    bool isClassRelated(ClassObject *klass, bool interfaceCheck = true);
    bool isAtLeast(generic_processing_stage stage) { return processStage >= stage; }
    bool isNot(generic_processing_stage stage) { return processStage < stage; }
    void setProcessStage(generic_processing_stage stage) { processStage = stage; }
    bool isGlobalClass() { return globalClass; }
    void setGlobalClass(bool glob) { globalClass = glob; }
    bool getFunctionByName(string name, List<Method*> &functions, bool checkBase = false);
    long getFieldAddress(Field* field);
    long getInstanceFieldAddress(string name) {
        long address = 0, index = getFieldIndex(name);
        for(long i = 0; i < index; i++) {
            if(!fields.get(i)->flags.find(STATIC)) {
                address++;
            }
        }

        return address;
    }
    long getStaticFieldAddress(string name) {
        long address = 0, index = getFieldIndex(name);
        for(long i = 0; i < index; i++) {
            if(fields.get(i)->flags.find(STATIC)) {
                address++;
            }
        }

        return address;
    }

    long getInstanceFieldCount() {
        long instanceFields = 0;
        for(long i = 0; i < fields.size(); i++) {
            if(!fields.get(i)->flags.find(STATIC)) {
                instanceFields++;
            }
        }

        return instanceFields;
    }

    long getStaticFieldCount() {
        long staticFields = 0;
        for(long i = 0; i < fields.size(); i++) {
            if(fields.get(i)->flags.find(STATIC)) {
                staticFields++;
            }
        }

        return staticFields;
    }

    long getFieldIndex(string &name);
    Alias* getAlias(string name, bool checkBase);
    long totalFieldCount();
    void addClass(ClassObject* k) {
        classes.addif(k);
    }
    void addField(Field* f) {
        fields.add(f);
    }
    void addFunction(Method* f) {
        functions.add(f);
    }
    Int getFunctionCount() {
        return functions.size();
    }
    void addKeys(List<string> &keys) {
        this->keys.addAll(keys);
    }
    void addKeyTypes(List<Utype*> &keyTypes) {
        this->keyTypes.addAll(keyTypes);
    }

    List<string>& getKeys() { return keys; }
    List<Utype*>& getKeyTypes() { return keyTypes; }
    List<Alias*>& getAliases() { return aliases; }
    List<ClassObject*> & getChildClasses() { return classes; }
    ClassObject* getGenericOwner() { return genericOwner; }
    void setGenericOwner(ClassObject* owner) { genericOwner = owner; }
    void setInterfaces(List<ClassObject*> &interfaces) { this->interfaces.addAll(interfaces); }
    List<ClassObject*> &getInterfaces() { return interfaces; }
    List<Ast*> &getExtensionFunctionTree() { return extensionFunctions; }
    List<Ast*> &getClassMutations() { return classMutations; }
    void getAllFunctionsByType(function_type ftype, List<Method*> &results, bool checkSuper = false) {
        for(long long i = 0; i < functions.size(); i++) {
            if(functions.get(i)->fnType == ftype)
                results.add(functions.get(i));
        }

        if(checkSuper && super != NULL)
            super->getAllFunctionsByType(ftype, results, true);
    }
    bool hasInterface(ClassObject *iface) {
        for(long i = 0; i < interfaces.size(); i++) {
            if(iface->match(interfaces.get(i)))
                return true;
        }

        return false;
    }

    void getAllFunctionsByTypeAndName(function_type ftype, string name, bool checkBase, List<Method*> &results) {
        for(long long i = 0; i < functions.size(); i++) {
            if(functions.get(i)->fnType == ftype && functions.get(i)->name == name)
                results.add(functions.get(i));
        }

        if(checkBase && super)
            super->getAllFunctionsByTypeAndName(ftype, name, true, results);
    }

    ClassObject* getChildClass(string name, bool match = false) {
        for(unsigned long i = 0; i < classes.size(); i++) {
            if(classes.get(i)->name == name || (match && classes.get(i)->name.find(name) != string::npos))
                return classes.get(i);
        }

        return NULL;
    }

    /**
     * This method is expected to be used exclusivley for the purposes of creating a generic class
     * methods and more will not be copied over as they are expected to be created later
     * @param k
     */
    void transfer(ClassObject *k) {
        free();
        this->super = k->super;
        this->processStage = k->processStage;
        this->globalClass = k->globalClass;
        this->classType = k->classType;
        this->classes.addAll(k->classes);
        this->fullName = k->fullName;
        this->name = k->name;
        this->type = k->type;
        this->module = k->module;
        this->address = k->address;
        this->guid = k->guid;
        this->flags.addAll(k->flags);
        this->owner = k->owner;
        this->ast = k->ast;
        this->keys.addAll(k->keys);
    }

    long processedExtFunctions;
    long processedMutations;
private:
    generic_processing_stage processStage;
    bool globalClass;
    int classType;
    List<Method*> functions;
    List<Ast*> extensionFunctions;
    List<Ast*> classMutations;
    List<ClassObject*> classes;
    List<Alias*> aliases;
    List<ClassObject*> interfaces;
    List<Field*> fields;
    List<string> keys;
    List<Utype*> keyTypes;
    ClassObject* super, *genericOwner;
};

#define CLASS_GENERIC_MASK 0x2
#define CLASS_INTERFACE_MASK 0x1
#define CLASS_ENUM_MASK 0x4

#define IS_CLASS_NORM(x) (x == 0)
#define IS_CLASS_GENERIC(x) (((x >> 1) & 1UL) == 1)
#define IS_CLASS_ENUM(x) (((x >> 2) & 1UL) == 1)
#define IS_CLASS_INTERFACE(x) (((x & CLASS_INTERFACE_MASK) & 1UL) == 1)

#endif //SHARP_CLASSOBJECT_H
