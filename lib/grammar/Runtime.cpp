//
// Created by bknun on 9/12/2017.
//

#include "Runtime.h"
#include "parser/ErrorManager.h"
#include "List.h"
#include "parser/Parser.h"
#include "../util/File.h"

using namespace std;

options c_options;
Sharp versions;

void help();

int _bootstrap(int argc, const char* argv[])
{
    if(argc < 2) {
        help();
        return 1;
    }


    initalizeErrors();
    List<string> files;
    for (int i = 1; i < argc; ++i) {
        args_:
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
            printVersion();
            exit(0);
        }
        else if(opt("-O")){
            c_options.optimize = true;
        }
        else if(opt("-h") || opt("-?")){
            help();
            exit(0);
        }
        else if(opt("-R") || opt("-release")){
            c_options.optimize = true;
            c_options.debug = false;
            c_options.strip = true;
        }
        else if(opt("-s")){
            c_options.strip = true;
        }
        else if(opt("-magic")){
            c_options.magic = true;
        }
        else if(opt("-debug")) {
            c_options.debugMode = true;
        }
        else if(opt("-showversion")){
            printVersion();
            cout << endl;
        }
        else if(opt("-target")){
            if(i+1 >= argc)
                rt_error("file version required after option `-target`");
            else {
                std::string x = std::string(argv[++i]);
                if(all_integers(x))
                    c_options.target = strtol(x.c_str(), NULL, 0);
                else {
                    if(to_lower(x) == "base") {
                        c_options.target = versions.BASE;
                    } else if(to_lower(x) == "alpha") {
                        c_options.target = versions.ALPHA;
                    }
                    else {
                        rt_error("unknown target " + x);
                    }
                }
            }
        }
        else if(opt("-w")){
            c_options.warnings = false;
        }
        else if(opt("-v")){
            if(i+1 >= argc)
                rt_error("file version required after option `-v`");
            else
                c_options.vers = string(argv[++i]);
        }
        else if(opt("-u") || opt("-unsafe")){
            c_options.unsafe = true;
        }
        else if(opt("-werror")){
            c_options.werrors = true;
            c_options.warnings = true;
        }
        else if(opt("-errlmt")) {
            std::string lmt = std::string(argv[++i]);
            if(all_integers(lmt)) {
                c_options.error_limit = strtoul(lmt.c_str(), NULL, 0);

                if(c_options.error_limit > 100000) {
                    rt_error("cannot set the max errors allowed higher than (100,000) - " + lmt);
                } else if(c_options.error_limit == 0) {
                    rt_error("cannot have an error limit of 0 ");
                }
            }
            else {
                rt_error("invalid error limit set " + lmt);
            }
        }
        else if(opt("-objdmp")){
            c_options.objDump = true;
        }
        else if(string(argv[i]).at(0) == '-'){
            rt_error("invalid option `" + string(argv[i]) + "`, try bootstrap -h");
        }
        else {
            // add the source files
            do {
                if(string(argv[i]).at(0) == '-')
                    goto args_;

                files.addif(string(argv[i++]));
            }while(i<argc);
            break;
        }
    }

    if(files.size() == 0){
        help();
        return 1;
    }

    for(unsigned int i = 0; i < files.size(); i++) {
        string& file = files.get(i);

        if(!File::exists(file.c_str())){
            rt_error("file `" + file + "` doesnt exist!");
        }
        if(!File::endswith(".sharp", file)){
            rt_error("file `" + file + "` is not a sharp file!");
        }
    }

    exec_runtime(files);
    return 0;
}

void help() {
    cout << "Usage: bootstrap" << "{OPTIONS} SOURCE FILE(S)" << std::endl;
    cout << "Source file must have a .sharp extion to be compiled.\n" << endl;
    cout << "[-options]\n\n    -V                print compiler version and exit"                   << endl;
    cout <<               "    -showversion      print compiler version and continue"               << endl;
    cout <<               "    -o<file>          set the output object file"                        << endl;
    cout <<               "    -c                compile only and do not generate exe"              << endl;
    cout <<               "    -a                enable aggressive error reporting"                 << endl;
    cout <<               "    -s                string debugging info"                             << endl;
    cout <<               "    -O                optimize executable"                               << endl;
    cout <<               "    -w                disable all warnings"                              << endl;
    cout <<               "    -errlmt<count>    set max errors the compiler allows before quitting"  << endl;
    cout <<               "    -v<version>       set the application version"                       << endl;
    cout <<               "    -unsafe -u        allow unsafe code"                                 << endl;
    cout <<               "    -objdmp           create dump file for generated assembly"           << endl;
    cout <<               "    -target           target the specified platform of sharp to run on"  << endl;
    cout <<               "    -werror           enable warnings as errors"                         << endl;
    cout <<               "    -release -r       generate a release build exe"                      << endl;
    cout <<               "    --h -?            display this help message"                         << endl;
}

void rt_error(string message) {
    cout << "bootstrap:  error: " << message << endl;
    exit(1);
}

void printVersion() {
    cout << progname << " " << progvers;
}

std::string to_lower(string s) {
    string newstr = "";
    for(char c : s) {
        newstr += tolower(c);
    }
    return newstr;
}

bool all_integers(string int_string) {
    for(char c : int_string) {
        if(!isdigit(c))
            return false;
    }
    return true;
}

void exec_runtime(List<string>& files)
{
    List<Parser*> parsers;
    Parser* p = NULL;
    tokenizer* t;
    File::buffer source;
    size_t errors=0, unfilteredErrors=0;
    long succeeded=0, failed=0, panic=0;

    for(unsigned int i = 0; i < files.size(); i++) {
        string& file = files.get(i);
        source.begin();

        File::read_alltext(file.c_str(), source);
        if(source.empty()) {
            for(unsigned long i = 0; i < parsers.size(); i++) {
                Parser* parser = parsers.get(i);
                parser->free();
                delete(parser);
            }

            rt_error("file `" + file + "` is empty.");
        }

        if(c_options.debugMode)
            cout << "tokenizing " << file << endl;

        t = new tokenizer(source.to_str(), file);
        if(t->getErrors()->hasErrors())
        {
            t->getErrors()->printErrors();

            errors+= t->getErrors()->getErrorCount();
            unfilteredErrors+= t->getErrors()->getUnfilteredErrorCount();
            failed++;
        }
        else {
            if(c_options.debugMode)
                cout << "parsing " << file << endl;

            p = new Parser(t);
            parsers.push_back(p);

            if(p->getErrors()->hasErrors())
            {
                p->getErrors()->printErrors();

                errors+= p->getErrors()->getErrorCount();
                unfilteredErrors+= p->getErrors()->getUnfilteredErrorCount();
                failed++;

                if(p->panic) {
                    panic = 1;
                    goto end;
                }

            } else {
                succeeded++;
            }
        }

        end:
        t->free();
        delete (t);
        source.end();

        if(panic==1) {
            cout << "Detected more than " << c_options.error_limit << "+ errors, quitting.";
            break;
        }
    }

    if(!panic && errors == 0 && unfilteredErrors == 0) {
        if(c_options.debugMode)
            cout << "preparing to perform syntax analysis on project files"<< endl;

        RuntimeEngine engine(c_options.out, parsers);

        failed = engine.failedParsers.size();
        succeeded = engine.succeededParsers.size();

        errors+=engine.errorCount;
        unfilteredErrors+=engine.unfilteredErrorCount;
        if(errors == 0 && unfilteredErrors == 0) {
            if(!c_options.compile)
                engine.generate();
        }

        engine.cleanup();
    }
    else {
        for(unsigned long i = 0; i < parsers.size(); i++) {
            Parser* parser = parsers.get(i);
            parser->free();
            delete(parser);
        }
        parsers.free();
    }

    cout << endl << "==========================================================\n" ;
    cout << "Errors: " << (c_options.aggressive_errors ? unfilteredErrors : errors) << " Succeeded: "
         << succeeded << " Failed: " << failed << " Total: " << files.size() << endl;
}

void RuntimeEngine::compile()
{
    if(preprocess()) {

    }
}

/**
 * We need to know about all the classes & variables before we attempt
 * to try to resolve anything
 * @return
 */
bool RuntimeEngine::preprocess()
{
    bool success = true;

    for(unsigned long i = 0; i < parsers.size(); i++)
    {
        activeParser = parsers.get(i);
        errors = new ErrorManager(activeParser->lines, activeParser->sourcefile, true, c_options.aggressive_errors);

        currentModule = "$unknown";
        keypair<string, List<string>> resolveMap;
        List<string> imports;

        sourceFiles.addif(activeParser->sourcefile);
        addScope(Scope(GLOBAL_SCOPE, NULL));
        for(unsigned long i = 0; i < activeParser->treesize(); i++)
        {
            Ast *ast = activeParser->ast_at(i);

            if(i == 0 && ast->getType() == ast_module_decl) {
                add_module(currentModule = parseModuleName(ast));
                imports.push_back(currentModule);
                continue;
            } else if(i == 0)
                errors->createNewError(GENERIC, ast->line, ast->col, "module declaration must be "
                        "first in every file");

            switch(ast->getType()) {
                case ast_class_decl:
                    parseClassDecl(ast);
                    break;
                case ast_import_decl:
                    imports.add(parseModuleName(ast));
                    break;
                case ast_module_decl: /* fail-safe */
                    errors->createNewError(GENERIC, ast->line, ast->col, "file module cannot be declared more than once");
                    break;
                default:
                    stringstream err;
                    err << ": unknown ast type: " << ast->getType();
                    errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
                    break;
            }
        }

        resolveMap.set(activeParser->sourcefile, imports);
        importMap.push_back(resolveMap);
        if(errors->hasErrors()){
            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            success = false;
            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        remove_scope();
    }



    return success;
}

Scope* RuntimeEngine::addScope(Scope scope) {
    scopeMap.push_back(scope);
    return currentScope();
}

void RuntimeEngine::add_module(string name) {
    if(!module_exists(name)) {
        modules.push_back(name);
    }
}

bool RuntimeEngine::module_exists(string name) {
    for(unsigned long i = 0; i < modules.size(); i++) {
        if(modules.get(i) == name)
            return true;
    }

    return false;
}

string RuntimeEngine::parseModuleName(Ast *ast) {
    if(ast == NULL) return "";
    ast = ast->getSubAst(0); // module_list

    stringstream str;
    for(long i = 0; i < ast->getEntityCount(); i++) {
        str << ast->getEntity(i).getToken();
    }
    return str.str();
}

bool RuntimeEngine::isTokenAccessDecl(token_entity token) {
    return
            token.getId() == IDENTIFIER && token.getToken() == "protected" ||
            token.getId() == IDENTIFIER && token.getToken() == "private" ||
            token.getId() == IDENTIFIER && token.getToken() == "static" ||
            token.getId() == IDENTIFIER && token.getToken() == "const" ||
            token.getId() == IDENTIFIER && token.getToken() == "override" ||
            token.getId() == IDENTIFIER && token.getToken() == "public";
}

AccessModifier RuntimeEngine::entityToModifier(token_entity entity) {
    if(entity.getToken() == "public")
        return PUBLIC;
    else if(entity.getToken() == "private")
        return PRIVATE;
    else if(entity.getToken() == "protected")
        return PROTECTED;
    else if(entity.getToken() == "const")
        return mCONST;
    else if(entity.getToken() == "static")
        return STATIC;
    else if(entity.getToken() == "override")
        return OVERRIDE;
    return mUNDEFINED;
}


list<AccessModifier> RuntimeEngine::parseAccessModifier(Ast *ast) {
    int iter=0;
    list<AccessModifier> modifiers;

    do {
        modifiers.push_back(entityToModifier(ast->getEntity(iter++)));
    }while(isTokenAccessDecl(ast->getEntity(iter)));

    return modifiers;
}

bool RuntimeEngine::parseAccessDecl(Ast *ast, List<AccessModifier> &modifiers, int &startpos) {
    if(ast == NULL) return false;

    if(isTokenAccessDecl(ast->getEntity(0))) {
        list<AccessModifier> mods = parseAccessModifier(ast);
        modifiers.addAll(mods);
        startpos+=modifiers.size();
        return true;
    }
    return false;
}

void RuntimeEngine::parseClassAccessModifiers(List<AccessModifier> &modifiers, Ast* ast) {
    if(modifiers.size() > 1)
        this->errors->createNewError(GENERIC, ast->line, ast->col, "class objects only allows for a single access "
                "specifier (public, private, protected)");
    else {
        AccessModifier mod = modifiers.at(0);

        if(mod != PUBLIC && mod != PRIVATE && mod != PROTECTED)
            this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                   " `" + ast->getEntity(0).getToken() + "`");
    }
}

bool RuntimeEngine::classExists(string module, string name) {
    ClassObject* klass = NULL;
    for(unsigned int i = 0; i < classes.size(); i++) {
        klass = &classes.get(i);
        if(klass->getName() == name) {
            if(module != "")
                return klass->getModuleName() == module;
            return true;
        }
    }

    return false;
}

bool RuntimeEngine::addClass(ClassObject klass) {
    if(!classExists(klass.getModuleName(), klass.getName())) {
        classes.add(klass);
        return true;
    }
    return false;
}

void RuntimeEngine::printNote(RuntimeNote& note, string msg) {
    if(lastNoteMsg != msg && lastNote.getLine() != note.getLine())
    {
        cout << note.getNote(msg);
        lastNoteMsg = msg;
    }
}

ClassObject *RuntimeEngine::getClass(string module, string name) {
    ClassObject* klass = NULL;
    for(unsigned int i = 0; i < classes.size(); i++) {
        klass = &classes.get(i);
        if(klass->getName() == name) {
            if(module != "" && klass->getModuleName() == module)
                return klass;
            else if(module == "")
                return klass;
        }
    }

    return NULL;
}

ClassObject *RuntimeEngine::addGlobalClassObject(string name, List<AccessModifier>& modifiers, Ast *pAst) {
    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(pAst->line),
                                   pAst->line, pAst->col);

    if(!this->addClass(ClassObject(name, currentModule, this->uniqueSerialId++,modifiers.get(0), note))){

        this->errors->createNewError(PREVIOUSLY_DEFINED, pAst->line, pAst->col, "class `" + name +
                                                                          "` is already defined in module {" + currentModule + "}");

        printNote(this->getClass(currentModule, name)->note, "class `" + name + "` previously defined here");
        return getClass(currentModule, name);
    } else
        return getClass(currentModule, name);
}

ClassObject *RuntimeEngine::addChildClassObject(string name, List<AccessModifier>& modifiers, Ast *ast, ClassObject* super) {
    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    if(!super->addChildClass(ClassObject(name,
                                         currentModule, this->uniqueSerialId++, modifiers.get(0),
                                         note, super))) {
        this->errors->createNewError(DUPLICATE_CLASS, ast->line, ast->col, " '" + name + "'");

        printNote(super->getChildClass(name)->note, "class `" + name + "` previously defined here");
        return super->getChildClass(name);
    } else
        return super->getChildClass(name);
}

void RuntimeEngine::remove_scope() {
    currentScope()->free();
    scopeMap.pop_back();
}

void RuntimeEngine::parseClassDecl(Ast *ast)
{
    Scope* scope = currentScope();
    Ast* block = ast->getLastSubAst();
    List<AccessModifier> modifiers;
    ClassObject* currentClass;
    int startPosition=1;

    if(parseAccessDecl(ast, modifiers, startPosition)){
        parseClassAccessModifiers(modifiers, ast);
    } else {
        modifiers.push_back(PUBLIC);
    }

    string className =  ast->getEntity(startPosition).getToken();

    if(scope->klass == NULL)
        currentClass = addGlobalClassObject(className, modifiers, ast);
    else
        currentClass = addChildClassObject(className, modifiers, ast, scope->klass);


    addScope(Scope(CLASS_SCOPE, currentClass));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        ast = block->getSubAst(i);

        switch(ast->getType()) {
            case ast_class_decl:
                parseClassDecl(ast);
                break;
            case ast_var_decl:
                parseVarDecl(ast);
                break;
            case ast_method_decl: /* Will be parsed later */
                break;
            case ast_operator_decl: /* Will be parsed later */
                break;
            case ast_construct_decl: /* Will be parsed later */
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << ast->getType();
                errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
                break;
        }
    }
    remove_scope();
}

void RuntimeEngine::parseVarDecl(Ast *ast)
{
    Scope* scope = currentScope();
    List<AccessModifier> modifiers;
    int startpos=0;


    if(parseAccessDecl(ast, modifiers, startpos)){
        parseVarAccessModifiers(modifiers, ast);
    } else {
        modifiers.push_back(PUBLIC);
    }

    string name =  ast->getEntity(startpos).getToken();
    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    if(!scope->klass->addField(Field(NULL, uniqueSerialId++, name, scope->klass, modifiers, note))) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                               "field `" + name + "` is already defined in the scope");
        printNote(note, "field `" + name + "` previously defined here");
    }
}

int RuntimeEngine::parseVarAccessSpecifiers(List<AccessModifier> &modifiers) {
    for(long i = 0; i < modifiers.size(); i++) {
        AccessModifier modifier = modifiers.get(i);
        if(modifier > STATIC)
            return i;
    }

    if(modifiers.get(0) <= PROTECTED) {
        if(modifiers.size() > 3)
            return (int)(modifiers.size() - 1);
        else if(modifiers.size() == 2) {
            if((modifiers.get(1) != mCONST
                && modifiers.get(1) != STATIC))
                return 1;
        }
        else if(modifiers.size() == 3) {
            if(modifiers.get(1) != STATIC)
                return 1;
            if(modifiers.get(2) != mCONST)
                return 2;
        }
    }
    else if(modifiers.get(0) == STATIC) {
        if(modifiers.size() > 2)
            return (int)(modifiers.size() - 1);
        else if(modifiers.size() == 2 && modifiers.get(1) != mCONST)
            return 1;
    }
    else if(modifiers.get(0) == mCONST) {
        if(modifiers.size() != 1)
            return (int)(modifiers.size() - 1);
    }
    return -1;
}

void RuntimeEngine::parseVarAccessModifiers(List<AccessModifier> &modifiers, Ast *ast) {
    if(modifiers.size() > 3)
        this->errors->createNewError(GENERIC, ast->line, ast->col, "too many access specifiers");
    else {
        int result = parseVarAccessSpecifiers(modifiers);
        switch(result) {
            case -1:
                break;
            default:
                this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->getEntity(result).getLine(),
                                             ast->getEntity(result).getColumn(), " `" + ast->getEntity(result).getToken() + "`");
                break;
        }
    }

    if(!modifiers.find(PUBLIC) && !modifiers.find(PRIVATE)
       && !modifiers.find(PROTECTED)) {
        modifiers.push_back(PUBLIC);
    }
}

void RuntimeEngine::generate() {

}

void RuntimeEngine::cleanup() {
    freeList(parsers);
    errors->free();
    delete(errors); errors = NULL;
    modules.free();

    freeList(classes);
    freeList(scopeMap);
    sourceFiles.free();
}
