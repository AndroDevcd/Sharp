//
// Created by BraxtonN on 1/30/2017.
//
#include "runtime.h"

void runtime::interpret() {
    if(preprocess()) {

    }
}

bool runtime::preprocess() {
    bool semtekerrors = false;
    for(parser* p : parsers) {
        errors = new Errors(p->toks->getlines(), p->toks->file);
        // Evaluate all types in memory
        // process all module creations
        const size_t ts= p->treesize();

        for(int i = 0; i < ts; i++) { // look for class, method and var decls, multiple defs
            ast* trunk = p->ast_at(i), *tmp;

            if(i == 0 && trunk->gettype() == ast_module_decl) {

                // add the module
            } else {
                errors->newerror(GENERIC, trunk->line, trunk->col, "module declaration must be "
                        "frst in every file");
            }

            if(trunk->gettype() == ast_class_decl) {
                tmp = trunk->getsubast(0);

                //if(this->) // check if we call create the class under the module
            }
        }


        if(errors->_errs()){
            cout << errors->getall_errors();
            semtekerrors = true;
        }

        errors->free();
        std::free(errors); this->errors = NULL;
    }

    return !semtekerrors;
}

void runtime::cleanup() {
}

bool runtime::module_exists(string name) {
    for(string& mname : *modules) {
        if(mname == name)
            return true;
    }

    return false;
}

bool runtime::class_exists(string module, string name) {
    for(ClassObject& klass : *classes) {
        if(klass.getName() == name) {
            if(module != "")
                return klass.getModuleName() == module;
            return true;
        }
    }

    return false;
}

bool runtime::add_class(ClassObject &klass) {
    if(!class_exists(klass.getModuleName(), klass.getName())) {
        classes->push_back(klass);
        return true;
    }
    return false;
}

void runtime::add_module(string name) {
    if(!module_exists(name)) {
        modules->push_back(name);
    }
}

Method *runtime::getStaticFunction(ClassObject *klass, string name, list<Param> &params) {
    for(Method* function : *staticfunctions) {
        if(Param::match(*function->getParams(), params) && name == function->getName()
           && klass->match(function->getParentClass()))
            return function;
    }

    return NULL;
}

ClassObject *runtime::getClass(string module, string name) {
    for(ClassObject& klass : *classes) {
        if(klass.getName() == name) {
            if(module != "" && klass.getModuleName() == module)
                return &klass;
            else if(module == "")
                return &klass;
        }
    }

    return NULL;
}

Field *runtime::getStaticField(ClassObject *klass, string name) {
    for(Field* field : *staticfields) {
        if(field->name == name && field->parent->match(klass)) {
            return field;
        }
    }

    return NULL;
}
