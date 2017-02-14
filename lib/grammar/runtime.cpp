//
// Created by BraxtonN on 1/30/2017.
//
#include <cstring>
#include <sstream>
#include "runtime.h"
#include "../util/file.h"

options c_options;
size_t succeeded, failed;
_operator string_toop(string op);

void runtime::interpret() {
    if(preprocess()) {
        succeeded = 0; // There were no errors to stop the files that succeeded

        for(parser* p : parsers) {
            errors = new Errors(p->lines, p->sourcefile, true, c_options.aggressive_errors);
            _current = p;

            const size_t ast_count = p->treesize();
            ast* pAst;

            for(size_t i = 0; i < ast_count; i++) {
                pAst = p->ast_at(i);

                switch(pAst->gettype()) {
                    case ast_module_decl:
                        break;
                    case ast_import_decl:
                        break;

                    case ast_class_decl:
                        parse_class_decl(pAst, NULL);
                        break;

                    case ast_macros_decl:
                        break;

                    default:
                        errors->newerror(INTERNAL_ERROR, pAst->line, pAst->col, " unexpected ast type");
                        break;
                }
            }

            if(errors->_errs()){
                errs+= errors->error_count();
                uo_errs+= errors->uoerror_count();
                failed++;
            } else
                succeeded++;

            errors->free();
            std::free(errors); this->errors = NULL;
        }
    }
}

ref_ptr runtime::parse_refrence_ptr(ast *pAst) {
    bool hashfound = false, last, hash = pAst->hasentity(HASH);
    string id;
    ref_ptr ptr;

    for(long i = 0; i < pAst->getentitycount(); i++) {
        id = pAst->getentity(i).gettoken();
        last = i + 1 >= pAst->getentitycount();

        if(id == ".")
            continue;
        else if(id == "#") {
            hashfound = true;
            continue;
        }

        if(hash && !hashfound && !last) {
            ptr.module +=id;
        } else if(!last) {
            ptr.class_heiarchy->push_back(id);
        } else {
            ptr.refname = id;
        }
    }

    return ptr;
}

ClassObject *runtime::parse_base_class(ast *pAst, ClassObject* inheritor) {
    ClassObject* klass=NULL;

    for(int i = 0; i < pAst->getentitycount(); i++) {
        if(pAst->getentity(i).gettoken() == "base") {
            int base_ptr = pAst->hassubast(ast_type_declarator) ? 1 : 0;
            ref_ptr ptr = parse_refrence_ptr(pAst->getsubast(base_ptr));

            klass = resolve_class_refrence(pAst->getsubast(base_ptr), ptr);

            if(klass != NULL) {
                if(klass->match(inheritor)) {
                    errors->newerror(GENERIC, pAst->line, pAst->col, "class `" + ptr.refname + "` cannot inherit itsself");
                }
                else if(inheritor->curcular(klass)) {
                    errors->newerror(GENERIC, pAst->getsubast(base_ptr)->line, pAst->getsubast(base_ptr)->col,
                                     "circular dependency of class `" + ptr.refname + "` in parent class `" + inheritor->getName() + "`");
                }
                else {

                    if(pAst->getsubast(++base_ptr)->gettype() == ast_type_arg) {
                        // create runtime class from type args
                    }
                }
            }

            break;
        }
        else if(pAst->getentity(i).gettoken() == "class" && inheritor == NULL) {
            inheritor = getClass(current_module, pAst->getentity(i+1).gettoken());
        }
    }

    return klass;
}

ClassObject* runtime::resolve_class_refrence(ast *pAst, ref_ptr &ptr) {
    ResolvedRefrence resolvedRefrence = resolve_refrence_ptr(pAst, ptr);

    if(resolvedRefrence.rt == ResolvedRefrence::NOTRESOLVED) {
        errors->newerror(COULD_NOT_RESOLVE, pAst->line, pAst->col, " `" + ptr.refname + "` " +
                            (ptr.module == "" ? "" : "in module {" + ptr.module + "} "));
    } else {

        if(resolvedRefrence.rt == ResolvedRefrence::CLASS) {
            return resolvedRefrence.klass;
        }
        else if(resolvedRefrence.rt == ResolvedRefrence::FIELD) {
            errors->newerror(EXPECTED_REFRENCE_OF_TYPE, pAst->line, pAst->col, " 'class' instead of 'field'");
        }
    }

    return NULL;
}

void runtime::parse_class_decl(ast *pAst, ClassObject* pObject) {
    // create runtime class
    ClassObject* base = parse_base_class(pAst, pObject);
}

bool runtime::preprocess() {
    bool semtekerrors = false;
    for(parser* p : parsers) {
        errors = new Errors(p->lines, p->sourcefile, true, c_options.aggressive_errors);
        _current = p;

        current_module = "$invisible";
        const size_t ts= p->treesize();
        keypair<string, std::list<string>> resolve_map;
        list<string> imports;

        ast* trunk;
        for(int i = 0; i < ts; i++) {
            trunk = p->ast_at(i);

            if(i == 0 && trunk->gettype() == ast_module_decl) {
                add_module(current_module = ast_tostring(trunk->getsubast(0)));
                continue;
            } else if(i == 0)
                errors->newerror(GENERIC, trunk->line, trunk->col, "module declaration must be "
                        "first in every file");

            if(trunk->gettype() == ast_class_decl) {
                preprocc_class_decl(trunk, NULL);
            }
            else if(trunk->gettype() == ast_import_decl) {
                string import = get_modulename(trunk->getsubast(0));
                imports.push_back(import);
            }
            else if(trunk->gettype() == ast_macros_decl) {
                preprocc_macros_decl(trunk, NULL);
            }
            else if(trunk->gettype() == ast_module_decl) {
                errors->newerror(GENERIC, trunk->line, trunk->col, "file module cannot be declared more than once");
            }
        }

        imports.push_back(current_module);
        resolve_map.set(p->sourcefile, imports);
        import_map->push_back(resolve_map);
        if(errors->_errs()){
            errs+= errors->error_count();
            uo_errs+= errors->uoerror_count();

            semtekerrors = true;
            failed++;
        } else
            succeeded++;

        errors->free();
        std::free(errors); this->errors = NULL;
    }

    return !semtekerrors;
}

void runtime::warning(p_errors error, int line, int col, string xcmnts) {
    if(c_options.warnings) {
        if(c_options.werrors){
            errors->newerror(error, line, col, xcmnts);
        } else {
            errors->newwarning(error, line, col, xcmnts);
        }
    }
}

string runtime::get_modulename(ast *pAst) {
    stringstream modulename;

    for(int i = 0; i < pAst->getentitycount(); i++) {
        modulename << pAst->getentity(i).gettoken();
    }
    return modulename.str();
}

bool runtime::isaccess_decl(token_entity token) {
    return
            token.getid() == IDENTIFIER && token.gettoken() == "protected" ||
            token.getid() == IDENTIFIER && token.gettoken() == "private" ||
            token.getid() == IDENTIFIER && token.gettoken() == "static" ||
            token.getid() == IDENTIFIER && token.gettoken() == "const" ||
            token.getid() == IDENTIFIER && token.gettoken() == "override" ||
            token.getid() == IDENTIFIER && token.gettoken() == "public";
}

int runtime::ismacro_access_specifiers(list<AccessModifier>& modifiers) {
    int iter=0;
    for(AccessModifier m : modifiers) {
        if(m > mStatic)
            return iter;
        iter++;
    }

    if(element_at(modifiers, 0) <= mProtected) {
        if(modifiers.size() > 2)
            return (int)(modifiers.size() - 1);
        else if(modifiers.size() == 2) {
            if(element_at(modifiers, 1) != mStatic)
                return 1;
        }
    }
    else if(element_at(modifiers, 0) == mStatic) {
        if(modifiers.size() > 1)
            return (int)(modifiers.size() - 1);
    }
    return -1;
}

int runtime::isvar_access_specifiers(list<AccessModifier>& modifiers) {
    int iter=0;
    for(AccessModifier m : modifiers) {
        if(m > mStatic)
            return iter;
        iter++;
    }

    if(element_at(modifiers, 0) <= mProtected) {
        if(modifiers.size() > 3)
            return (int)(modifiers.size() - 1);
        else if(modifiers.size() == 2) {
            if((element_at(modifiers, 1) != mConst
                && element_at(modifiers, 1) != mStatic))
                return 1;
        }
        else if(modifiers.size() == 3) {
            if(element_at(modifiers, 1) != mStatic)
                return 1;
            if(element_at(modifiers, 2) != mConst)
                return 2;
        }
    }
    else if(element_at(modifiers, 0) == mStatic) {
        if(modifiers.size() > 2)
            return (int)(modifiers.size() - 1);
        else if(modifiers.size() == 2 && element_at(modifiers, 1) != mConst)
            return 1;
    }
    else if(element_at(modifiers, 0) == mConst) {
        if(modifiers.size() != 1)
            return (int)(modifiers.size() - 1);
    }
    return -1;
}

int runtime::ismethod_access_specifiers(list<AccessModifier>& modifiers) {
    int iter=0, acess=0;
    for(AccessModifier m : modifiers) {
        if(m == mConst)
            return iter;
        iter++;
    }

    if(element_at(modifiers, 0) <= mProtected) {
        if(modifiers.size() > 3)
            return (int)(modifiers.size() - 1);
        else if(modifiers.size() == 2) {
            if(element_at(modifiers, 1) != mStatic && element_at(modifiers, 1) != mOverride)
                return 1;
        }
        else if(modifiers.size() == 3) {
            if(element_at(modifiers, 1) != mStatic)
                return 1;
            if(element_at(modifiers, 2) != mOverride)
                return 2;
        }
    }
    else if(element_at(modifiers, 0) == mStatic) {
        if(modifiers.size() > 2)
            return (int)(modifiers.size() - 1);
        else if(modifiers.size() == 2 && element_at(modifiers, 1) != mOverride)
            return 1;
    }
    else if(element_at(modifiers, 0) == mOverride) {
        if(modifiers.size() != 1)
            return (int)(modifiers.size() - 1);
    }
    return -1;
}

list<Param> runtime::ast_toparams(ast *pAst, ClassObject* parent) {
    list<Param> params;

    if(pAst->gettype() != ast_utype_arg_list)
        return params;
    if(pAst->getsubastcount() == 0)
        return params;

    ast* tmp;
    for(int i = 0; i < pAst->getsubastcount(); i++) {
        tmp = pAst->getsubast(i)->getsubast(0)->getsubast(0);

        string name = pAst->getsubast(i)->getentity(0).gettoken();
        if(tmp->getentitycount() > 0) {
            params.push_back(Param(Field(
                    entity_tonativefield(tmp->getentity(0)), uid++,
                    name, parent, NULL,
                    RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                pAst->line, pAst->col))));
        } else {

            params.push_back(Param(Field(
                    NULL, uid++,name, parent, NULL, RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                pAst->line, pAst->col))));
        }
    }

    return params;
}

list<string> runtime::parse_templArgs(ast *pAst) {
    list<string> tmplArgs;
    if(pAst == NULL || pAst->gettype() != ast_type_declarator)
        return tmplArgs;

    token_entity entity;
    for(size_t i = 1; i < pAst->getentitycount(); i++) {
        entity = pAst->getentity(i);

        if(entity == ">")
            break;
        else if(entity == ","){}
        else
            tmplArgs.push_back(entity.gettoken());
    }

    return tmplArgs;
}

void runtime::preprocc_macros_decl(ast *pAst, ClassObject *pObject) {
    ast *tmp;
    list<AccessModifier> modifiers;
    int namepos=1;
    bool macrosAdded;

    if(isaccess_decl(pAst->getentity(0))) {
        modifiers = this->preprocc_access_modifier(pAst);
        if (modifiers.size() > 2)
            this->errors->newerror(GENERIC, pAst->line, pAst->col, "too many access specifiers");
        else {
            int m = ismacro_access_specifiers(modifiers);
            switch (m) {
                case -1:
                    break;
                default:
                    this->errors->newerror(INVALID_ACCESS_SPECIFIER, pAst->getentity(m).getline(),
                                           pAst->getentity(m).getcolumn(), " `" + pAst->getentity(m).gettoken() + "`");
                    break;
            }
        }

        namepos+=modifiers.size();
        if(!element_has(modifiers, mPublic) && !element_has(modifiers, mPrivate)
           && element_has(modifiers, mProtected)) {
            modifiers.push_back(mPublic);
        }

        if(element_has(modifiers, mStatic))
            warning(REDUNDANT_TOKEN, pAst->getentity(element_index(modifiers, mStatic)).getline(),
                    pAst->getentity(element_index(modifiers, mStatic)).getcolumn(), " `static`, macros are static by default");
    } else
        modifiers.push_back(mPublic);

    list<Param> params;
    NativeField n_rtype = fnof;
    string name =  pAst->getentity(namepos).gettoken();

    if(pAst->getsubastcount() == 3) {
        tmp = pAst->getsubast(1)->getsubast(0);
        if(tmp->getentitycount() > 0)
            n_rtype = entity_tonativefield(tmp->getentity(0));
    } else
        n_rtype = fvoid;

    tmp = pAst->getsubast(0);
    params = ast_toparams(pAst->getsubast(0), pObject);

    if(tmp->gettype() == ast_utype_arg_list && n_rtype != fnof) {
        if(pObject != NULL)
            macrosAdded =pObject->addMacros(Method(name, current_module, pObject, params, modifiers, n_rtype, RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                                                          pAst->line, pAst->col)));
        else
            macrosAdded =add_macros(Method(name, current_module, pObject, params, modifiers, n_rtype, RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                                                  pAst->line, pAst->col)));
    } else {
        if(pObject != NULL)
            macrosAdded =pObject->addMacros(Method(name, current_module, pObject, params, modifiers, NULL, RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                                                         pAst->line, pAst->col)));
        else
            macrosAdded =add_macros(Method(name, current_module, pObject, params, modifiers, NULL, RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                                               pAst->line, pAst->col)));
    }


    if(!macrosAdded) {
        this->errors->newerror(PREVIOUSLY_DEFINED, pAst->line, pAst->col,
                               "macro `" + name + "` is already defined in the scope");
        Method* macro;
        if(pObject != NULL)
            macro = pObject->getMacros(name, params);
        else
            macro = getmacros(current_module, name, params);

        printnote(macro->note, "macro `" + name + "` previously defined here");
    }
}

void runtime::preprocc_constructor_decl(ast *pAst, ClassObject *pObject) {
    list<AccessModifier> modifiers;
    int namepos=0;
    bool methodAdded;

    if(isaccess_decl(pAst->getentity(0))) {
        modifiers = this->preprocc_access_modifier(pAst);
        if(modifiers.size() > 1)
            this->errors->newerror(GENERIC, pAst->line, pAst->col, "too many access specifiers");
        else {
            AccessModifier mod = element_at(modifiers, 0);

            if(mod != mPublic && mod != mPrivate && mod != mProtected)
                this->errors->newerror(INVALID_ACCESS_SPECIFIER, pAst->line, pAst->col,
                                       " `" + pAst->getentity(0).gettoken() + "`");
        }
        namepos+=modifiers.size();
    } else
        modifiers.push_back(mPublic);


    list<Param> params;
    string name = pAst->getentity(namepos).gettoken();
    if(name == pObject->getName()) {
        params = ast_toparams(pAst->getsubast(0), pObject);
        methodAdded =pObject->addConstructor(Method(name, "", pObject, params, modifiers, fvoid,
                                RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                              pAst->line, pAst->col)));

        if(!methodAdded) {
            this->errors->newerror(PREVIOUSLY_DEFINED, pAst->line, pAst->col,
                                   "constructor `" + name + "` is already defined in the scope");
            Method* constr;
            constr = pObject->getConstructor(params);
            printnote(constr->note, "constructor `" + name + "` previously defined here");
        }
    } else
        this->errors->newerror(PREVIOUSLY_DEFINED, pAst->line, pAst->col,
                               "constructor `" + name + "` must be the same name as its parent");
}

void runtime::preprocc_operator_decl(ast *pAst, ClassObject *pObject) {
    ast *tmp;
    list<AccessModifier> modifiers;
    bool methodAdded;

    if(isaccess_decl(pAst->getentity(0))) {
        modifiers = this->preprocc_access_modifier(pAst);
        if(modifiers.size() > 3)
            this->errors->newerror(GENERIC, pAst->line, pAst->col, "too many access specifiers");
        else {
            int m=ismethod_access_specifiers(modifiers);
            switch(m) {
                case -1:
                    break;
                default:
                    this->errors->newerror(INVALID_ACCESS_SPECIFIER, pAst->getentity(m).getline(),
                                           pAst->getentity(m).getcolumn(), " `" + pAst->getentity(m).gettoken() + "`");
                    break;
            }
        }

        if(!element_has(modifiers, mPublic) && !element_has(modifiers, mPrivate)
           && element_has(modifiers, mProtected)) {
            modifiers.push_back(mPublic);
        }
    } else
        modifiers.push_back(mPublic);

    list<Param> params;
    NativeField n_rtype = fnof;

    if(pAst->getsubastcount() == 3) {
        tmp = pAst->getsubast(1)->getsubast(0);
        if(tmp->getentitycount() > 0)
            n_rtype = entity_tonativefield(tmp->getentity(0));
        tmp = pAst->getsubast(0);
    } else {
        tmp = pAst->getsubast(1);
        n_rtype = fvoid;
    }

    params = ast_toparams(pAst->getsubast(0), pObject);
    string op = pAst->getentity(pAst->getentitycount()-1).gettoken();

    if(tmp->gettype() == ast_utype_arg_list && n_rtype != fnof) {
        methodAdded =pObject->addOperatorOverload(OperatorOverload(RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                               pAst->line, pAst->col),
                                                                   pObject, params, modifiers, n_rtype, string_toop(op)));
    } else {
        if(pObject->isTmplClass() && pAst->getsubastcount() == 3 ) {
            ast* astRefPtr = pAst->getsubast(1)->getsubast(0)->getsubast(0);

            /* Check if rtype class is template parameter */
            if(astRefPtr->getentitycount() == 1 && element_has(*pObject->getTemplRefs(), astRefPtr->getentity(0).gettoken())){
                methodAdded =pObject->addOperatorOverload(OperatorOverload(RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                                       pAst->line, pAst->col),
                                                                           pObject, params, modifiers, astRefPtr->getentity(0).gettoken(), string_toop(op)));
            }
            else
                methodAdded =pObject->addOperatorOverload(OperatorOverload(RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                                       pAst->line, pAst->col),
                                                                           pObject, params, modifiers, NULL, string_toop(op)));
        }
        else
            methodAdded =pObject->addOperatorOverload(OperatorOverload(RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                                   pAst->line, pAst->col),
                                                                       pObject, params, modifiers, NULL, string_toop(op)));
    }


    if(!methodAdded) {
        this->errors->newerror(PREVIOUSLY_DEFINED, pAst->line, pAst->col,
                               "function `$operator" + op + "` is already defined in the scope");
        Method* oper;
        oper = pObject->getOverload(string_toop(op), params);
        printnote(oper->note, "function `$operator" + op + "` previously defined here");
    }
}

void runtime::preprocc_method_decl(ast *pAst, ClassObject *pObject) {
    ast *tmp;
    list<AccessModifier> modifiers;
    int namepos=1;
    bool methodAdded;

    if(isaccess_decl(pAst->getentity(0))) {
        modifiers = this->preprocc_access_modifier(pAst);
        if(modifiers.size() > 3)
            this->errors->newerror(GENERIC, pAst->line, pAst->col, "too many access specifiers");
        else {
            int m=ismethod_access_specifiers(modifiers);
            switch(m) {
                case -1:
                    break;
                default:
                    this->errors->newerror(INVALID_ACCESS_SPECIFIER, pAst->getentity(m).getline(),
                                           pAst->getentity(m).getcolumn(), " `" + pAst->getentity(m).gettoken() + "`");
                    break;
            }
        }

        if(!element_has(modifiers, mPublic) && !element_has(modifiers, mPrivate)
           && element_has(modifiers, mProtected)) {
            modifiers.push_back(mPublic);
        }
        namepos+=modifiers.size();
    } else
        modifiers.push_back(mPublic);

    list<Param> params;
    NativeField n_rtype = fnof;
    string name =  pAst->getentity(namepos).gettoken();

    if(pAst->getsubastcount() == 3) {
        tmp = pAst->getsubast(1)->getsubast(0);
        if(tmp->getentitycount() > 0)
            n_rtype = entity_tonativefield(tmp->getentity(0));
        tmp = pAst->getsubast(0);
    } else {
        tmp = pAst->getsubast(1);
        n_rtype = fvoid;
    }

    params = ast_toparams(pAst->getsubast(0), pObject);

    if(tmp->gettype() == ast_utype_arg_list && n_rtype != fnof) {
        methodAdded =pObject->addFunction(Method(name, "", pObject, params, modifiers, n_rtype,RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                                                       pAst->line, pAst->col)));
    } else {
        if(pObject->isTmplClass() && pAst->getsubastcount() == 3 ) {
            ast* astRefPtr = pAst->getsubast(1)->getsubast(0)->getsubast(0);

            /* Check if rtype class is template parameter */
            if(astRefPtr->getentitycount() == 1 && element_has(*pObject->getTemplRefs(), astRefPtr->getentity(0).gettoken())){
                methodAdded =pObject->addFunction(Method(name, "", pObject, params, modifiers,astRefPtr->getentity(0).gettoken(),
                                                         RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                     pAst->line, pAst->col)));
            }
            else
                methodAdded =pObject->addFunction(Method(name, "", pObject, params, modifiers, NULL,
                                                         RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                     pAst->line, pAst->col)));
        }
        else
            methodAdded =pObject->addFunction(Method(name, "", pObject, params, modifiers, NULL,
                                                     RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                                 pAst->line, pAst->col)));
    }


    if(!methodAdded) {
        this->errors->newerror(PREVIOUSLY_DEFINED, pAst->line, pAst->col,
                               "function `" + name + "` is already defined in the scope");
        Method* function;
        function = pObject->getFunction(name, params);
        printnote(function->note, "function `" + name + "` previously defined here");
    }
}

void runtime::preprocc_var_decl(ast *pAst, ClassObject *pObject) {
    ast *tmp;
    list<AccessModifier> modifiers;
    int namepos=0;
    bool fieldAdded;

    if(isaccess_decl(pAst->getentity(0))) {
        modifiers = this->preprocc_access_modifier(pAst);
        if(modifiers.size() > 3)
            this->errors->newerror(GENERIC, pAst->line, pAst->col, "too many access specifiers");
        else {
            int m=isvar_access_specifiers(modifiers);
            switch(m) {
                case -1:
                    break;
                default:
                    this->errors->newerror(INVALID_ACCESS_SPECIFIER, pAst->getentity(m).getline(),
                                           pAst->getentity(m).getcolumn(), " `" + pAst->getentity(m).gettoken() + "`");
                    break;
            }
        }

        if(!element_has(modifiers, mPublic) && !element_has(modifiers, mPrivate)
           && element_has(modifiers, mProtected)) {
            modifiers.push_back(mPublic);
        }
        namepos+=modifiers.size();
    } else
        modifiers.push_back(mPublic);

    string name =  pAst->getentity(namepos).gettoken();
    tmp = pAst->getsubast(0)->getsubast(0);
    if(tmp->gettype() == ast_type_identifier && tmp->getentitycount() != 0){
        fieldAdded =pObject->addField(Field(entity_tonativefield(tmp->getentity(0)),
                               uid++, name, pObject, &modifiers,
                                            RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                        pAst->line, pAst->col))); // native field
    } else {
        ast* templRefAst = tmp->getsubast(0);
        if(templRefAst->getentitycount() == 1 &&
                element_has(*pObject->getTemplRefs(), templRefAst->getentity(0).gettoken())) {
            fieldAdded =pObject->addField(Field(templRefAst->getentity(0).gettoken(), uid++, name, pObject, &modifiers,
                                                RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                            pAst->line, pAst->col))); // Template field
        } else
            fieldAdded =pObject->addField(Field(NULL, uid++, name, pObject, &modifiers,
                                                RuntimeNote(_current->sourcefile, _current->geterrors()->getline(pAst->line),
                                                            pAst->line, pAst->col))); // Refrence field

    }


    if(!fieldAdded) {
        this->errors->newerror(PREVIOUSLY_DEFINED, pAst->line, pAst->col,
                               "field `" + name + "` is already defined in the scope");
        Field* field;
        field = pObject->getField(name);
        printnote(field->note, "field `" + name + "` previously defined here");
    }
}

void runtime:: preprocc_class_decl(ast *trunk, ClassObject* parent) {
    ast *pAst, *block;
    block = trunk->getsubast(trunk->getsubastcount()-1);
    list<AccessModifier> modifiers;
    ClassObject* klass;
    int namepos=1;

    if(isaccess_decl(trunk->getentity(0))) {
        modifiers = this->preprocc_access_modifier(trunk);
        if(modifiers.size() > 1)
            this->errors->newerror(GENERIC, trunk->line, trunk->col, "too many access specifiers");
        else {
            AccessModifier mod = element_at(modifiers, 0);

            if(mod != mPublic && mod != mPrivate && mod != mProtected)
                this->errors->newerror(INVALID_ACCESS_SPECIFIER, trunk->line, trunk->col,
                                 " `" + trunk->getentity(0).gettoken() + "`");
        }
        namepos+=modifiers.size();
    } else
        modifiers.push_back(mPublic);

    string name =  trunk->getentity(namepos).gettoken();
    list<string> templArgs = parse_templArgs(trunk->getsubastcount() <= 1 ? NULL:trunk->getsubast(0));
    bool isTempl = templArgs.size() != 0;

    if(parent == NULL) {
        if(!this->add_class(ClassObject(name,
                    current_module, this->uid++, mPublic, isTempl, templArgs,
                                        RuntimeNote(_current->sourcefile, _current->geterrors()->getline(trunk->line),
                                                    trunk->line, trunk->col)))){
            this->errors->newerror(PREVIOUSLY_DEFINED, trunk->line, trunk->col, "class `" + name +
                               "` is already defined in module {" + current_module + "}");
            ClassObject* klazz;
            klazz = getClass(current_module, name);
            printnote(klazz->note, "class `" + name + "` previously defined here");
            return;
        } else
            klass = getClass(current_module, name);
    } else {
        if(!parent->addChildClass(ClassObject(name,
                                             current_module, this->uid++, mPublic, isTempl, templArgs,
                                              RuntimeNote(_current->sourcefile, _current->geterrors()->getline(trunk->line),
                                                          trunk->line, trunk->col)))) {
            this->errors->newerror(DUPLICATE_CLASS, trunk->line, trunk->col, " '" + name + "'");

            ClassObject* klazz;
            klazz = parent->getChildClass(name);
            printnote(klazz->note, "class `" + name + "` previously defined here");
            return;
        } else
            klass = parent->getChildClass(name);

        klass->setSuperClass(parent);
    }


    for(uint32_t i = 0; i < block->getsubastcount(); i++) {
        pAst = block->getsubast(i);
        if(pAst->gettype() == ast_class_decl) {
            preprocc_class_decl(pAst, klass);
        }
        else if(pAst->gettype() == ast_var_decl) {
            preprocc_var_decl(pAst, klass);
        }
        else if(pAst->gettype() == ast_method_decl) {
            preprocc_method_decl(pAst, klass);
        }
        else if(pAst->gettype() == ast_operator_decl) {
            preprocc_operator_decl(pAst, klass);
        }
        else if(pAst->gettype() == ast_construct_decl) {
            preprocc_constructor_decl(pAst, klass);
        }
        else if(pAst->gettype() == ast_macros_decl) {
            preprocc_macros_decl(pAst, klass);
        }
    }
}

list<AccessModifier> runtime::preprocc_access_modifier(ast *trunk) {
    int iter=0;
    list<AccessModifier> modifiers;

    do {
        modifiers.push_back(entity_tomodifier(trunk->getentity(iter++)));
    }while(isaccess_decl(trunk->getentity(iter)));

    return modifiers;
}

void runtime::cleanup() {
    for(parser* p2 : parsers) {
        p2->free();
    }
    parsers.clear();
    std::free(errors); errors = NULL;
    modules->clear();
    std::free(modules); modules = NULL;

    for(ClassObject& klass : *classes) {
        klass.free();
    }
    classes->clear();
    std::free(classes); classes = NULL;

    for(keypair<string, std::list<string>>& map : *import_map) {
        map.value.clear();
    }
    import_map->clear();
    std::free(import_map); import_map = NULL;

    for(Method& macro : *macros) {
        macro.clear();
    }
    macros->clear();
    std::free(macros); macros = NULL;
}

void rt_error(string message) {
    cout << "sharp:  error: " << message << endl;
    exit(1);
}

void help() {
    std::cerr << "Usage: bootstrap" << " {OPTIONS} SOURCE FILE(S)" << std::endl;
    cout << "Source file must have a .sharp extension to be compiled\n" << endl;
    cout << "[-options]\n\n    -V                print the bootstrap version number and exit" << endl;
    cout <<               "    -showversion      print the bootstrap version number and continue." << endl;
    cout <<               "    -o<file>          set the output object file." << endl;
    cout <<               "    -c                compile only and do not generate object file." << endl;
    cout <<               "    -a                enable aggressive error reporting." << endl;
    cout <<               "    -O                optimize code." << endl;
    cout <<               "    -w                disable warnings." << endl;
    cout <<               "    -werror           enable warnings as errors." << endl;
    cout <<               "    --h -?            display this help message." << endl;
}

#define opt(v) strcmp(argv[i], v) == 0
void _srt_start(list<string> files);

void print_vers();

int _bootstrap(int argc, const char* argv[]) {
    int_errs();

    if (argc < 2) { // We expect 2 arguments: the source file(s), and program options
        help();
        return 1;
    }

    list<string> files;
    for (int i = 1; i < argc; ++i) {
        if(opt("-a")){
            c_options.aggressive_errors = true;
        }
        else if(opt("-c")){
            c_options.compile = true;
        }
        else if(opt("-o")){
            if(i+1 >= argc)
                rt_error("output file required after option `-o`");
            else
                c_options.out = string(argv[++i]);
        }
        else if(opt("-V")){
            print_vers();
            exit(0);
        }
        else if(opt("-O")){
            c_options.optimize = true;
        }
        else if(opt("-h") || opt("-?")){
            help();
            exit(0);
        }
        else if(opt("-showversion")){
            print_vers();
            cout << endl;
        }
        else if(opt("-w")){
            c_options.warnings = false;
        }
        else if(opt("-werror")){
            c_options.werrors = true;
            c_options.warnings = true;
        }
        else if(string(argv[i]).at(0) == '-'){
            rt_error("invalid option `" + string(argv[i]) + "`, try bootstrap -h");
        }
        else {
            // add the source files
            string f;
            do {
                f =string(argv[i++]);

                if(!element_has(files, f))
                    files.push_back(f);
            }while(i<argc);
            break;
        }
    }

    if(files.size() == 0){
        help();
        return 1;
    }

    for(string file : files){
        if(!file::exists(file.c_str())){
            rt_error("file `" + file + "` dosent exist!");
        }
        if(!file::endswith(".sharp", file)){
            rt_error("file `" + file + "` is not a sharp file!");
        }
    }

    _srt_start(files);
    return 0;
}

void print_vers() {
    cout << progname << " " << progvers;
}

void _srt_start(list<string> files)
{
    std::list<parser*> parsers;
    parser* p = NULL;
    tokenizer* t;
    string source;
    size_t errors=0, uo_errors=0;
    succeeded=0, failed=0;

    for(string file : files) {
        source = file::read_alltext(file.c_str());
        if(source == "") {
            for(parser* p2 : parsers) {
                p2->free();
                std::free(p2);
            }

            rt_error("file `" + file + "` is empty.");
        }

        t = new tokenizer(source, file);
        if(t->geterrors()->_errs())
        {
            t->geterrors()->print_errors();

            errors+= t->geterrors()->error_count();
            uo_errors+= t->geterrors()->uoerror_count();
            failed++;
        }
        else {
            p = new parser(t);

            if(p->geterrors()->_errs())
            {
                p->geterrors()->print_errors();

                errors+= p->geterrors()->error_count();
                uo_errors+= p->geterrors()->uoerror_count();
                failed++;
            } else {
                parsers.push_back(p);
                succeeded++;
            }
        }

        t->free();
        std::free(t);
        source = "";
    }

    if(errors == 0 && uo_errors == 0) {
        failed = 0, succeeded=0;
        runtime rt(c_options.out, parsers);

        errors+=rt.errs;
        uo_errors+=rt.uo_errs;
        rt.cleanup();
    }
    else {
        for(parser* p2 : parsers) {
            p2->free();
        }
        parsers.clear();
    }

    cout << endl << "==========================================================\n" ;
    cout << "Errors: " << (c_options.aggressive_errors ? uo_errors : errors) << " Succeeded: "
         << succeeded << " Failed: " << failed << endl;

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

Method *runtime::getmacros(string module, string name, list<Param> params) {
    for(Method& macro : *macros) {
        if(Param::match(*macro.getParams(), params) && name == macro.getName()) {
            if(module != "")
                return module == macro.getModule() ? &macro : NULL;

            return &macro;
        }
    }

    return NULL;
}

bool runtime::add_macros(Method macro) {
    if(getmacros(macro.getModule(), macro.getName(), *macro.getParams()) == NULL) {
        macros->push_back(macro);
        return true;
    }
    return false;
}

bool runtime::add_class(ClassObject klass) {
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

string runtime::ast_tostring(ast *pAst) {
    if(pAst == NULL) return "";

    stringstream str;
    for(long i = 0; i < pAst->getentitycount(); i++) {
        str << pAst->getentity(i).gettoken();
    }
    return str.str();
}

NativeField runtime::entity_tonativefield(token_entity entity) {
    if(entity.gettoken() == "int")
        return fint;
    else if(entity.gettoken() == "short")
        return fshort;
    else if(entity.gettoken() == "long")
        return flong;
    else if(entity.gettoken() == "double")
        return fdouble;
    else if(entity.gettoken() == "float")
        return ffloat;
    else if(entity.gettoken() == "bool")
        return fbool;
    else if(entity.gettoken() == "char")
        return fchar;
    else if(entity.gettoken() == "string")
        return fstring;
    return fnof;
}

AccessModifier runtime::entity_tomodifier(token_entity entity) {
    if(entity.gettoken() == "public")
        return mPublic;
    else if(entity.gettoken() == "private")
            return mPrivate;
    else if(entity.gettoken() == "protected")
        return mProtected;
    else if(entity.gettoken() == "const")
        return mConst;
    else if(entity.gettoken() == "static")
        return mStatic;
    else if(entity.gettoken() == "override")
        return mOverride;
    return mStatic;
}

void runtime::printnote(RuntimeNote& note, string msg) {
    if(last_notemsg != msg && last_note.getLine() != note.getLine())
    {
        cout << note.getNote(msg);
        last_notemsg = msg;
    }
}

ResolvedRefrence runtime::resolve_refrence_ptr(ast* pAst, ref_ptr &ref_ptr) {
    ResolvedRefrence refrence;

    if(ref_ptr.class_heiarchy->size() == 0) {
        ClassObject* klass = getClass(ref_ptr.module, ref_ptr.refname);
        if(klass == NULL) {
            refrence.rt = ResolvedRefrence::NOTRESOLVED;
        } else {
            refrence.rt = ResolvedRefrence::CLASS;
            refrence.klass = klass;
        }
    } else {
        ClassObject* klass = getClass(ref_ptr.module, element_at(*ref_ptr.class_heiarchy, 0));
        if(klass == NULL) {
            refrence.rt = ResolvedRefrence::NOTRESOLVED;
        } else {
            ClassObject* childClass = NULL;
            string className;
            for(size_t i = 1; i < ref_ptr.class_heiarchy->size(); i++) {
                className = element_at(*ref_ptr.class_heiarchy, i);

                if((childClass = klass->getChildClass(className)) == NULL) {
                    refrence.rt = ResolvedRefrence::NOTRESOLVED;
                    break;
                }
            }

            if(childClass != NULL) {
                if(childClass->getField(ref_ptr.refname) != NULL) {
                    refrence.rt = ResolvedRefrence::FIELD;
                    refrence.field = childClass->getField(ref_ptr.refname);
                } else {
                    refrence.rt = ResolvedRefrence::NOTRESOLVED;
                }
            } else {
                if(klass->getField(ref_ptr.refname) != NULL) {
                    refrence.rt = ResolvedRefrence::FIELD;
                    refrence.field = klass->getField(ref_ptr.refname);
                } else if(klass->getChildClass(ref_ptr.refname) != NULL) {
                    refrence.rt = ResolvedRefrence::CLASS;
                    refrence.klass = klass->getChildClass(ref_ptr.refname);
                } else {
                    refrence.rt = ResolvedRefrence::NOTRESOLVED;
                }
            }
        }
    }

    return refrence;
}

_operator string_toop(string op) {
    if(op=="+")
        return op_PLUS;
    if(op=="-")
        return op_MINUS;
    if(op=="*")
        return op_MULT;
    if(op=="/")
        return op_DIV;
    if(op=="%")
        return op_MOD;
    if(op=="++")
        return op_INC;
    if(op=="--")
        return op_DEC;
    if(op=="=")
        return op_EQUALS;
    if(op=="==")
        return op_EQUALS_EQ;
    if(op=="+=")
        return op_PLUS_EQ;
    if(op=="-=")
        return op_MIN_EQ;
    if(op=="*=")
        return op_MULT_EQ;
    if(op=="/=")
        return op_DIV_EQ;
    if(op=="&=")
        return op_AND_EQ;
    if(op=="|=")
        return op_OR_EQ;
    if(op=="!=")
        return op_NOT_EQ;
    if(op=="%=")
        return op_MOD_EQ;
    return op_NO;
}
