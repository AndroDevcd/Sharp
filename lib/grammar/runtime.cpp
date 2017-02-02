//
// Created by BraxtonN on 1/30/2017.
//
#include <cstring>
#include <sstream>
#include "runtime.h"
#include "../util/file.h"

options c_options;
size_t succeeded, failed;
void parse_options(list<keypair<string, string>> list);

void runtime::interpret() {
    if(preprocess()) {
        succeeded = 0; // There were no errors to stop the files that succeeded

        for(parser* p : parsers) {
            errors = new Errors(p->lines, p->sourcefile);



            if(errors->_errs()){
                if(c_options.aggressive_errors)
                    cout << errors->getuo_errors();
                else
                    cout << errors->getall_errors();

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

bool runtime::preprocess() {
    bool semtekerrors = false;
    for(parser* p : parsers) {
        errors = new Errors(p->lines, p->sourcefile);

        current_module = "$invisible";
        const size_t ts= p->treesize();
        keypair<string, list<string>> resolve_map;
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
            else if(trunk->gettype() == ast_module_decl) {
                errors->newerror(GENERIC, trunk->line, trunk->col, "file module cannot be declared more than once");
            }
        }

        resolve_map.set(p->sourcefile, imports);
        import_map->push_back(resolve_map);
        if(errors->_errs()){
            if(c_options.aggressive_errors)
                cout << errors->getuo_errors();
            else
                cout << errors->getall_errors();

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
                || element_at(modifiers, 1) != mStatic))
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
    int iter=0;
    for(AccessModifier m : modifiers) {
        if(m == mConst)
            return iter;
        iter++;
    }

    if(element_at(modifiers, 0) <= mProtected) {
        if(modifiers.size() > 3)
            return (int)(modifiers.size() - 1);
        else if(modifiers.size() == 2) {
            if(element_at(modifiers, 1) != mStatic)
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
                    name, parent, NULL)));
        } else {

            params.push_back(Param(Field(
                    NULL, uid++,name, parent, NULL)));
        }
    }

    return params;
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
        namepos+=modifiers.size();
    }

    list<Param> params;
    NativeField n_rtype = fnof;
    string name =  pAst->getentity(namepos).gettoken();

    if(pAst->getsubastcount() == 3) {
        tmp = pAst->getsubast(1)->getsubast(0);
        if(tmp->getentitycount() > 0)
            n_rtype = entity_tonativefield(tmp->getentity(0));
        tmp = pAst->getsubast(2);
    } else {
        tmp = pAst->getsubast(1);
        n_rtype = fvoid;
    }

    params = ast_toparams(pAst->getsubast(0), pObject);

    if(tmp->gettype() == ast_utype_arg_list && n_rtype != fnof) {
        methodAdded =pObject->addFunction(Method(name, pObject, params, modifiers, n_rtype));
    } else
        methodAdded =pObject->addFunction(Method(name, pObject, params, modifiers, NULL));

    if(!methodAdded) {
        this->errors->newerror(PREVIOUSLY_DEFINED, pAst->line, pAst->col,
                               "function `" + name + "` is already defined in the scope");
    }

    cout << "method created\n";
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
        namepos+=modifiers.size();
    }

    string name =  pAst->getentity(namepos).gettoken();
    tmp = pAst->getsubast(0)->getsubast(0);
    if(tmp->gettype() == ast_type_identifier && tmp->getentitycount() != 0){
        fieldAdded =pObject->addField(Field(entity_tonativefield(tmp->getentity(0)),
                               uid++, name, pObject, &modifiers)); // native field
    } else
        fieldAdded =pObject->addField(Field(NULL, uid++, name, pObject, &modifiers)); // Refrence field

    if(!fieldAdded) {
        this->errors->newerror(PREVIOUSLY_DEFINED, pAst->line, pAst->col,
                               "variable `" + name + "` is already defined in the scope");
    }
    cout << "variable created\n";
}

void runtime::preprocc_class_decl(ast *trunk, ClassObject* parent) {
    ast *pAst, *block;
    block = trunk->getsubast(0);
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
    }

    string name =  trunk->getentity(namepos).gettoken();
    if(parent == NULL) {
        if(!this->add_class(ClassObject(name,
                    current_module, this->uid++, mPublic))){
            this->errors->newerror(PREVIOUSLY_DEFINED, trunk->line, trunk->col, "class `" + name +
                               "` is already defined in module {" + current_module + "}");
            return;
        } else
            klass = getClass(current_module, name);
    } else {
        if(!parent->addChildClass(ClassObject(name,
                                             current_module, this->uid++, mPublic))) {
            this->errors->newerror(DUPLICATE_CLASS, trunk->line, trunk->col, " '" + name + "'");
            return;
        } else
            klass = parent->getChildClass(name);
    }

    cout << "class created\n";

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
        //create sub methods to preprocess each block element
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
    cout <<               "    -o<file>          set the output object file. Default is application.xso." << endl;
    cout <<               "    -c                compile only and do not generate object file." << endl;
    cout <<               "    -a                enable aggressive error reporting." << endl;
    cout <<               "    -w                disable warnings." << endl;
    cout <<               "    -we               enable warnings as errors." << endl;
    cout <<               "    --help -?         display this help message." << endl;
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
        else if(opt("-h")){
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
        else if(opt("-we")){
            c_options.werrors = true;
            c_options.warnings = true;
        }
        else if(string(argv[i]).at(0) == '-'){
            rt_error("invalid option `" + string(argv[i]) + "`, try bootstrap -h");
        }
        else {
            // add the source files
            do {
                files.push_back(string(argv[i++]));
            }while(i<argc);
            break;
        }
    }

    for(string file : files){
        if(!file::exists(file.c_str())){
            rt_error("file `" + file + "` dosent exist!");
        }
        else if(!file::endswith(".sharp", file)){
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
    list<parser*> parsers;
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
            if(c_options.aggressive_errors)
                cout << t->geterrors()->getuo_errors();
            else
                cout << t->geterrors()->getall_errors();

            errors+= t->geterrors()->error_count();
            uo_errors+= t->geterrors()->uoerror_count();
            failed++;
        }
        else {
            p = new parser(t);

            if(p->geterrors()->_errs())
            {
                if(c_options.aggressive_errors)
                    cout << p->geterrors()->getuo_errors();
                else
                    cout << p->geterrors()->getall_errors();

                errors+= p->geterrors()->error_count();
                uo_errors+= p->geterrors()->uoerror_count();
                failed++;
            } else {
                parsers.push_back(p);
            }
        }

        t->free();
        std::free(t);
        source = "";
    }

    if(errors == 0 && uo_errors == 0) {
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
