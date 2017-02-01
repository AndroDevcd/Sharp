//
// Created by BraxtonN on 1/30/2017.
//
#include <cstring>
#include "runtime.h"
#include "../util/file.h"

options c_options;
void parse_options(list<keypair<string, string>> list);

void runtime::interpret() {
    if(preprocess()) {

    }
}

bool runtime::preprocess() {
    bool semtekerrors = false;
    for(parser* p : parsers) {
        errors = new Errors(p->lines, p->sourcefile);
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

void rt_error(string message) {
    cout << "sharp:  error:" << message << endl;
    exit(1);
}

void help() {
    cout << "Source file must have a .sharp extension to be compiled\n" << endl;
    cout << "[-options]\n\n    -V                print the bootstrap version number and exit" << endl;
    cout <<               "    -showversion      print the bootstrap version number and continue." << endl;
    cout <<               "    -o<file>          set the output object file. Default is application.xso." << endl;
    cout <<               "    -c                compile only and do not generate object file." << endl;
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
        std::cerr << "Usage: bootstrap" << " {OPTIONS} SOURCE FILE(S)" << std::endl;
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
            rt_error("invalid option `" + string(argv[i]) + "`");
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
    size_t succeeded=0, failed=0;

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
                succeeded++;
            }
        }

        t->free();
        std::free(t);
        source = "";
    }

    if(errors == 0 && uo_errors == 0) {
          runtime rt("", parsers);
          rt.cleanup();
    }
    else {
        for(parser* p2 : parsers) {
            p2->free();
            std::free(p2);
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
