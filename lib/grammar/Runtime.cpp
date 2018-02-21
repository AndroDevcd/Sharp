//
// Created by bknun on 9/12/2017.
//

#include "Runtime.h"
#include "parser/ErrorManager.h"
#include "List.h"
#include "parser/Parser.h"
#include "../util/File.h"
#include "../runtime/Opcode.h"
#include "../runtime/register.h"

using namespace std;

unsigned long RuntimeEngine::uniqueSerialId = 0;
options c_options;

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
        resolveAllFields();
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
        KeyPair<string, List<string>> resolveMap;
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
        removeScope();
    }



    return success;
}

void RuntimeEngine::resolveAllFields() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        activeParser = parsers.get(i);
        errors = new ErrorManager(activeParser->lines, activeParser->sourcefile, true, c_options.aggressive_errors);
        currentModule = "$unknown";

        addScope(Scope(GLOBAL_SCOPE, NULL));
        for(int i = 0; i < activeParser->treesize(); i++) {
            Ast* ast = activeParser->ast_at(i);

            if(i==0) {
                if(ast->getType() == ast_module_decl) {
                    add_module(currentModule = parseModuleName(ast));
                    continue;
                }
            }

            switch(ast->getType()) {
                case ast_class_decl:
                    resolveClassDecl(ast);
                    break;
                default:
                    /* ignore */
                    break;
            }
        }

        if(errors->hasErrors()){
            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

ReferencePointer RuntimeEngine::parseReferencePtr(Ast *ast) {
    ast = ast->getSubAst(ast_refrence_pointer);
    bool hashfound = false, last, hash = ast->hasEntity(HASH);
    string id="";
    ReferencePointer ptr;

    for(long i = 0; i < ast->getEntityCount(); i++) {
        id = ast->getEntity(i).getToken();
        last = i + 1 >= ast->getEntityCount();

        if(id == ".")
            continue;
        else if(id == "#") {
            hashfound = true;
            continue;
        }

        if(hash && !hashfound && !last) {
            if(ptr.module == "")
                ptr.module =id;
            else
                ptr.module += "." + id;
        } else if(!last) {
            ptr.classHeiarchy.push_back(id);
        } else {
            ptr.referenceName = id;
        }
    }

    return ptr;
}

ClassObject* RuntimeEngine::tryClassResolve(string moduleName, string name) {
    ClassObject* klass = NULL;

    if((klass = getClass(moduleName, name)) == NULL) {
        for(unsigned int i = 0; i < importMap.size(); i++) {
            if(importMap.get(i).key == activeParser->sourcefile) {

                List<string>& lst = importMap.get(i).value;
                for(unsigned int x = 0; x < lst.size(); x++) {
                    if((klass = getClass(lst.get(i), name)) != NULL)
                        return klass;
                }

                break;
            }
        }
    }

    return klass;
}

ResolvedReference RuntimeEngine::resolveReferencePointer(ReferencePointer &ptr) {
    ResolvedReference reference;

    if(ptr.classHeiarchy.size() == 0) {
        ClassObject* klass = tryClassResolve(ptr.module, ptr.referenceName);
        if(klass == NULL) {
            reference.type = UNDEFINED;
            reference.referenceName = ptr.referenceName;
        } else {
            reference.type = CLASS;
            reference.klass = klass;
            reference.resolved = true;
        }
    } else {
        ClassObject* klass = tryClassResolve(ptr.module, ptr.classHeiarchy.get(0));
        if(klass == NULL) {
            reference.type = UNDEFINED;
            reference.referenceName = ptr.classHeiarchy.get(0);
        } else {
            ClassObject* childClass = NULL;
            string className;
            for(size_t i = 1; i < ptr.classHeiarchy.size(); i++) {
                className = ptr.classHeiarchy.get(i);

                if((childClass = klass->getChildClass(className)) == NULL) {
                    reference.type = UNDEFINED;
                    reference.referenceName = className;
                    return reference;
                } else {
                    klass = childClass;
                }

            }

            if(childClass != NULL) {
                if(childClass->getChildClass(ptr.referenceName) != NULL) {
                    reference.type = CLASS;
                    reference.klass = klass->getChildClass(ptr.referenceName);
                    reference.resolved = true;
                } else {
                    reference.type = UNDEFINED;
                    reference.referenceName = ptr.referenceName;
                }
            } else {
                if(klass->getChildClass(ptr.referenceName) != NULL) {
                    reference.type = CLASS;
                    reference.klass = klass->getChildClass(ptr.referenceName);
                    reference.resolved = true;
                } else {
                    reference.type = UNDEFINED;
                    reference.referenceName = ptr.referenceName;
                }
            }
        }
    }

    return reference;
}

bool RuntimeEngine::expectReferenceType(ResolvedReference refrence, FieldType expectedType, bool method, Ast *ast) {
    if(refrence.type == expectedType && refrence.isMethod==method)
        return true;

    errors->createNewError(EXPECTED_REFRENCE_OF_TYPE, ast->line, ast->col, " '" + (method ? "method" : ResolvedReference::typeToString(expectedType)) + "' instead of '" +
                                                                       refrence.typeToString() + "'");
    return false;
}

ClassObject* RuntimeEngine::resolveClassRefrence(Ast *ast, ReferencePointer &ptr) {
    ResolvedReference resolvedRefrence = resolveReferencePointer(ptr);
    ast = ast->getSubAst(ast_refrence_pointer);

    if(!resolvedRefrence.resolved) {
        errors->createNewError(COULD_NOT_RESOLVE, ast->line, ast->col, " `" + resolvedRefrence.referenceName + "` " +
                                                                   (ptr.module == "" ? "" : "in module {" + ptr.module + "} "));
    } else {

        if(expectReferenceType(resolvedRefrence, CLASS, false, ast)) {
            return resolvedRefrence.klass;
        }
    }

    return NULL;
}

ClassObject *RuntimeEngine::parseBaseClass(Ast *ast, int startpos) {
    Scope* scope = currentScope();
    ClassObject* klass=NULL;

    if(startpos >= ast->getEntityCount()) {
        return NULL;
    } else {
        ReferencePointer ptr = parseReferencePtr(ast);
        klass = resolveClassRefrence(ast, ptr);

        if(klass != NULL) {
            if((scope->klass->getHeadClass() != NULL && scope->klass->getHeadClass()->isCurcular(klass)) ||
               scope->klass->match(klass) || klass->match(scope->klass->getHeadClass())) {
                errors->createNewError(GENERIC, ast->getSubAst(0)->line, ast->getSubAst(0)->col,
                                 "cyclic dependency of class `" + ptr.referenceName + "` in parent class `" + scope->klass->getName() + "`");
            }
        }
    }

    return klass;
}

string Expression::typeToString() {
    switch(type) {
        case expression_string:
            return "var[]";
        case expression_unresolved:
            return "?";
        case expression_var:
            return string("var") + (utype.array ? "[]" : "");
        case expression_lclass:
            return utype.typeToString();
        case expression_native:
            return utype.typeToString();
        case expression_unknown:
            return "?";
        case expression_class:
            return utype.typeToString();
        case expression_void:
            return "void";
        case expression_objectclass:
            return "object";
        case expression_field:
            return utype.typeToString();
        case expression_null:
            return "null";
    }
    return utype.typeToString();
}

void Expression::operator=(Expression expression) {
    this->type=expression.type;
    this->newExpression=expression.newExpression;

    this->code.free();
    this->code.inject(0, expression.code);
    this->dot=expression.dot;
    this->func=expression.func;
    this->intValue=expression.intValue;
    this->literal=expression.literal;
    this->link = expression.link;
    this->utype  = expression.utype;
    this->value = expression.value;
    this->arrayElement = expression.arrayElement;
    this->boolExpressions.addAll(expression.boolExpressions);
}

void Expression::inject(Expression &expression) {
    this->code.inject(this->code.size(), expression.code);
}

ReferencePointer RuntimeEngine::parseTypeIdentifier(Ast *ast) {
    ast = ast->getSubAst(ast_type_identifier);

    if(ast->getSubAstCount() == 0) {
        ReferencePointer ptr;
        ptr.referenceName = ast->getEntity(0).getToken();
        return ptr;
    } else
        return parseReferencePtr(ast);
}

FieldType RuntimeEngine::tokenToNativeField(string entity) {
    if(entity == "var")
        return VAR;
    else if(entity == "object")
        return OBJECT;
    return UNDEFINED;
}

double RuntimeEngine::getInlinedFieldValue(Field* field) {
    for(unsigned int i = 0; i < inline_map.size(); i++) {
        if(inline_map.get(i).key==field->fullName)
            return inline_map.get(i).value;
    }

    return 0;
}


bool RuntimeEngine::isFieldInlined(Field* field) {
    if(!field->isStatic())
        return false;

    for(unsigned int i = 0; i < inline_map.size(); i++) {
        if(inline_map.get(i).key==field->fullName)
            return true;
    }

    return false;
}

int64_t RuntimeEngine::get_low_bytes(double var) {
    stringstream ss;
    ss.precision(16);
    ss << var;
    string num = ss.str(), result = "";
    int iter=0;
    for(unsigned int i = 0; i < num.size(); i++) {
        if(num.at(i) == '.') {
            for(unsigned int x = i+1; x < num.size(); x++) {
                if(iter++ > 16)
                    break;
                result += num.at(x);
            }
            return strtoll(result.c_str(), NULL, 0);
        }
    }
    return 0;
}

void RuntimeEngine::inlineVariableValue(Expression &expression, Field *field) {
    int64_t i64;
    double value = getInlinedFieldValue(field);
    expression.free();

    if(isDClassNumberEncodable(value))
        expression.code.push_i64(SET_Di(i64, op_MOVI, value), ebx);
    else {
        expression.code.push_i64(SET_Di(i64, op_MOVBI, ((int64_t)value)), abs(get_low_bytes(value)));
        expression.code.push_i64(SET_Ci(i64, op_MOVR, ebx,0, bmr));
    }

    expression.type=expression_var;
    expression.literal=true;
    expression.intValue=value;
}

bool RuntimeEngine::isDClassNumberEncodable(double var) { return ((int64_t )var > DA_MAX || (int64_t )var < DA_MIN) == false; }

void RuntimeEngine::resolveClassHeiarchy(ClassObject* klass, ReferencePointer& refrence, Expression& expression, Ast* pAst, bool requireStatic) {
    int64_t i64;
    string object_name = "";
    Field* field = NULL;
    ClassObject* k;
    bool lastRefrence = false;

    for(unsigned int i = 1; i < refrence.classHeiarchy.size()+1; i++) {
        if(i >= refrence.classHeiarchy.size()) {
            // field? if not then class?
            lastRefrence = true;
            object_name = refrence.referenceName;
        } else
            object_name = refrence.classHeiarchy.at(i);

        if((k = klass->getChildClass(object_name)) == NULL) {

            // field?
            if((field = klass->getField(object_name, true)) != NULL) {
                // is static?
                if(!lastRefrence && field->isArray) {
                    errors->createNewError(INVALID_ACCESS, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, " field array");
                } else if(requireStatic && !field->isStatic() && field->type != UNDEFINED) {
                    errors->createNewError(GENERIC, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, "static access on instance field `" + object_name + "`");
                }

                // for now we are just generating code for x.x.f not Main.x...thats static access
                expression.code.push_i64(SET_Di(i64, op_MOVN, field->address));

                if(lastRefrence) {
                    expression.utype.type = CLASSFIELD;
                    expression.utype.field = field;
                    expression.type = expression_field;
                }

                switch(field->type) {
                    case UNDEFINED:
                        expression.utype.type = UNDEFINED;
                        expression.utype.referenceName = object_name;
                        return;
                    case VAR:
                        if(lastRefrence){
                            if(isFieldInlined(field)) {
                                inlineVariableValue(expression, field);
                            }
                        }
                        else {
                            errors->createNewError(GENERIC, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, "invalid access to non-class field `" + object_name + "`");
                            expression.utype.referenceName = object_name;
                            return;
                        }
                        break;
                    case CLASS:
                        klass = field->klass;
                        break;
                }
            } else {
                errors->createNewError(COULD_NOT_RESOLVE, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, " `" + object_name + "` " +
                        (refrence.module == "" ? "" : "in module {" + refrence.module + "} "));
                expression.utype.type = UNDEFINED;
                expression.utype.referenceName = object_name;
                expression.type = expression_unknown;
                return;
            }
        } else {
            if(field != NULL) {
                field = NULL;
                errors->createNewError(GENERIC, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, " ecpected class or module before `" + object_name + "` ");
            }
            expression.code.push_i64(SET_Di(i64, op_MOVG, klass->address));
            klass = k;

            if(lastRefrence) {
                expression.utype.type = CLASSFIELD;
                expression.utype.klass = klass;
                expression.type = expression_class;
            }
        }
    }
}

void RuntimeEngine::resolveFieldHeiarchy(Field* field, ReferencePointer& refrence, Expression& expression, Ast* pAst) {
    switch(field->type) {
        case UNDEFINED:
            return;
        case VAR:
            errors->createNewError(GENERIC, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, "field `" + field->name + "` cannot be de-referenced");
            expression.utype.type = VAR;
            return;
        case CLASS:
            resolveClassHeiarchy(field->klass, refrence, expression, pAst, false);
            return;
    }
}


void RuntimeEngine::resolveUtype(ReferencePointer& refrence, Expression& expression, Ast* pAst) {
    Scope* scope = currentScope();
    int64_t i64;

    expression.link = pAst;
    if(scope->self) {
       // resolveSelfUtype(scope, refrence, expression, pAst);
    } else if(scope->base) {
      //  resolveBaseUtype(scope, refrence, expression, pAst);
    } else {
        if(refrence.singleRefrence()) {
            ClassObject* klass=NULL;
            Field* field=NULL;

            if(scope->type == GLOBAL_SCOPE) {

                if((klass = getClass(refrence.module, refrence.referenceName)) != NULL) {
                    expression.utype.type = CLASS;
                    expression.utype.klass = klass;
                    expression.type = expression_class;
                } else {
                    /* Un resolvable */
                    errors->createNewError(COULD_NOT_RESOLVE, pAst->getSubAst(ast_type_identifier)->line,
                                           pAst->getSubAst(ast_type_identifier)->col, " `" + refrence.referenceName + "` " +
                                                                                      (refrence.module == "" ? "" : "in module {" + refrence.module + "} "));

                    expression.utype.type = UNDEFINED;
                    expression.utype.referenceName = refrence.referenceName;
                    expression.type = expression_unresolved;
                }
            } else {

                // scope_class? | scope_instance_block? | scope_static_block?
                if(scope->type != CLASS_SCOPE && scope->getLocalField(refrence.referenceName) != NULL) {
                    field = &scope->getLocalField(refrence.referenceName)->value;
                    expression.utype.type = CLASSFIELD;
                    expression.utype.field = field;
                    expression.type = expression_field;

                    if(field->isVar()) {
                        if(field->isArray) {
                            expression.code.push_i64(SET_Di(i64, op_MOVL, field->address));
                        } else
                            expression.code.push_i64(SET_Ci(i64, op_LOADL, ebx, 0, field->address));
                    }
                    else
                        expression.code.push_i64(SET_Di(i64, op_MOVL, field->address));
                }
                else if((field = scope->klass->getField(refrence.referenceName)) != NULL) {
                    // field?
                    if(scope->type == STATIC_BLOCK) {
                        errors->createNewError(GENERIC, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col,
                                         "cannot get object `" + refrence.referenceName + "` from self in static context");
                    }

                    expression.utype.type = CLASSFIELD;
                    expression.utype.field = field;
                    expression.type = expression_field;

                    if(isFieldInlined(field)) {
                        inlineVariableValue(expression, field);
                    } else {
                        expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                        expression.code.push_i64(SET_Di(i64, op_MOVN, field->address));
                    }
                } else {
                    if((klass = getClass(refrence.module, refrence.referenceName)) != NULL) {
                        // global class ?
                        expression.utype.type = CLASS;
                        expression.utype.klass = klass;
                        expression.type = expression_class;
                    } else {
                        /* Un resolvable */
                        errors->createNewError(COULD_NOT_RESOLVE, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, " `" + refrence.referenceName + "` " +
                                                                                  (refrence.module == "" ? "" : "in module {" + refrence.module + "} "));

                        expression.utype.type = UNDEFINED;
                        expression.utype.referenceName = refrence.referenceName;
                        expression.type = expression_unresolved;
                    }
                }
            }
        } else if(refrence.singleRefrenceModule()){
            /* Must be a class i.e module#globalClass */
            ClassObject* klass=NULL;

            // in this case we ignore scope
            if((klass = getClass(refrence.module, refrence.referenceName)) != NULL) {
                expression.utype.type = CLASS;
                expression.utype.klass = klass;
                expression.type = expression_class;
            } else {
                /* Un resolvable */
                errors->createNewError(COULD_NOT_RESOLVE, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, " `" + refrence.referenceName + "` " +
                        (refrence.module == "" ? "" : "in module {" + refrence.module + "} "));

                expression.utype.type = UNDEFINED;
                expression.utype.referenceName = refrence.referenceName;
                expression.type = expression_unresolved;
            }
        } else {
            /* field? or class? */
            ClassObject* klass=NULL;
            Field* field=NULL;
            string starter_name = refrence.classHeiarchy.at(0);

            if(scope->type == GLOBAL_SCOPE) {

                // class?
                if((klass = getClass(refrence.module, starter_name)) != NULL) {
                    resolveClassHeiarchy(klass, refrence, expression, pAst);
                    return;
                } else {
                    /* un resolvable */
                    errors->createNewError(COULD_NOT_RESOLVE, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, " `" + starter_name + "` " +
                            (refrence.module == "" ? "" : "in module {" + refrence.module + "} "));
                    expression.utype.type = UNDEFINED;
                    expression.utype.referenceName = refrence.toString();
                    expression.type = expression_unresolved;

                }
            } else {

                // scope_class? | scope_instance_block? | scope_static_block?
                if(refrence.module != "") {
                    if((klass = getClass(refrence.module, starter_name)) != NULL) {
                        resolveClassHeiarchy(klass, refrence, expression, pAst);
                        return;
                    } else {
                        errors->createNewError(COULD_NOT_RESOLVE, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, " `" + starter_name + "` " +
                                (refrence.module == "" ? "" : "in module {" + refrence.module + "} "));
                        expression.utype.type = UNDEFINED;
                        expression.utype.referenceName = refrence.toString();
                        expression.type = expression_unresolved;
                        return;
                    }
                }

                if(scope->type != CLASS_SCOPE && scope->getLocalField(starter_name) != NULL) {
                    field = &scope->getLocalField(starter_name)->value;

                    /**
                     * You cannot access a var in this manner because it
                     * is not a data structure
                     */
                    if(field->isVar()) {
                        errors->createNewError(GENERIC, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col,
                                         "field `" + starter_name + "` cannot be de-referenced");
                    }
                    else
                        expression.code.push_i64(SET_Di(i64, op_MOVL, field->address));
                    resolveFieldHeiarchy(field, refrence, expression, pAst);
                    return;
                }
                else if((field = scope->klass->getField(starter_name)) != NULL) {
                    if(scope->type == STATIC_BLOCK) {
                        errors->createNewError(GENERIC, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col,
                                               "cannot get object `" + starter_name + "` from self in static context");
                    }

                    expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                    expression.code.push_i64(SET_Di(i64, op_MOVN, field->address));
                    resolveFieldHeiarchy(field, refrence, expression, pAst);
                    return;
                } else {
                    if((klass = getClass(refrence.module, starter_name)) != NULL) {
                        resolveClassHeiarchy(klass, refrence, expression, pAst);
                        return;
                    } else {
                        errors->createNewError(COULD_NOT_RESOLVE, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, " `" + starter_name + "` " +
                                (refrence.module == "" ? "" : "in module {" + refrence.module + "} "));
                        expression.utype.type = UNDEFINED;
                        expression.utype.referenceName = refrence.toString();
                        expression.type = expression_unresolved;
                    }
                }
            }
        }
    }
}

Expression RuntimeEngine::parseUtype(Ast* ast) {
    ast = ast->getSubAst(ast_utype);

    ReferencePointer ptr=parseTypeIdentifier(ast);
    Expression expression;

    if(ptr.singleRefrence() && Parser::isnative_type(ptr.referenceName)) {
        expression.utype.type = tokenToNativeField(ptr.referenceName);
        expression.type = expression_native;
        expression.utype.referenceName = ptr.toString();

        if(ast->hasEntity(LEFTBRACE) && ast->hasEntity(RIGHTBRACE)) {
            expression.utype.array = true;
        }

        expression.link = ast;
        ptr.free();
        return expression;
    }

    resolveUtype(ptr, expression, ast);

    if(ast->hasEntity(LEFTBRACE) && ast->hasEntity(RIGHTBRACE)) {
        expression.utype.array = true;
    }

    expression.link = ast;
    expression.utype.referenceName = ptr.toString();
    ptr.free();
    return expression;
}

void RuntimeEngine::resolveVarDecl(Ast* ast) {
    Scope* scope = currentScope();
    List<AccessModifier> modifiers;
    int startpos=0;

    parseAccessDecl(ast, modifiers, startpos);
    string name =  ast->getEntity(startpos).getToken();
    Field* field = scope->klass->getField(name);
    Expression expression = parseUtype(ast);
    if(expression.utype.type == CLASS) {
        field->klass = expression.utype.klass;
        field->type = CLASS;
    } else if(expression.utype.type == VAR) {
        field->type = VAR;
    } else {
        field->type = UNDEFINED;
    }

    field->isArray = expression.utype.array;
    field->address = scope->klass->getFieldIndex(name);
}

int RuntimeEngine::parseMethodAccessSpecifiers(List<AccessModifier> &modifiers) {
    for(long i = 0; i < modifiers.size(); i++) {
        AccessModifier modifier = modifiers.get(i);
        if(modifier > mCONST)
            return i;
    }

    if(modifiers.get(0) <= PROTECTED) {
        if(modifiers.size() > 3)
            return (int)(modifiers.size() - 1);
        else if(modifiers.size() == 2) {
            if(modifiers.get(1) != STATIC && modifiers.get(1) != OVERRIDE)
                return 1;
        }
        else if(modifiers.size() == 3) {
            if(modifiers.get(1) != STATIC)
                return 1;
            if(modifiers.get(2) != OVERRIDE)
                return 2;
        }
    }
    else if(modifiers.get(0) == STATIC) {
        if(modifiers.size() > 2)
            return (int)(modifiers.size() - 1);
        else if(modifiers.size() == 2 && modifiers.get(1) != OVERRIDE)
            return 1;
    }
    else if(modifiers.get(0) == OVERRIDE) {
        if(modifiers.size() != 1)
            return (int)(modifiers.size() - 1);
    }
    return -1;
}

void RuntimeEngine::parseMethodAccessModifiers(List<AccessModifier> &modifiers, Ast *pAst) {
    if(modifiers.size() > 3)
        this->errors->createNewError(GENERIC, pAst->line, pAst->col, "too many access specifiers");
    else {
        int m= parseMethodAccessSpecifiers(modifiers);
        switch(m) {
            case -1:
                break;
            default:
                this->errors->createNewError(INVALID_ACCESS_SPECIFIER, pAst->getEntity(m).getLine(),
                                       pAst->getEntity(m).getColumn(), " `" + pAst->getEntity(m).getToken() + "`");
                break;
        }
    }

    if(!modifiers.find(PUBLIC) && !modifiers.find(PRIVATE)
       && modifiers.find(PROTECTED)) {
        modifiers.add(PUBLIC);
    }
}

bool RuntimeEngine::containsParam(List<Param> params, string param_name) {
    for(unsigned int i = 0; i < params.size(); i++) {
        if(params.get(i).field.name == param_name)
            return true;
    }
    return false;
}

Field RuntimeEngine::fieldMapToField(string param_name, ResolvedReference& utype, Ast* pAst) {
    Field field;

    if(utype.type == CLASSFIELD) {
        errors->createNewError(COULD_NOT_RESOLVE, utype.field->note.getLine(), utype.field->note.getCol(), " `" + utype.field->name + "`");
        field.type = UNDEFINED;
        field.note = utype.field->note;
        field.modifiers.addAll(utype.field->modifiers);
    } else if(utype.type == CLASS) {
        field.type = CLASS;
        field.note = utype.klass->note;
        field.klass = utype.klass;
        field.modifiers.add(utype.klass->getAccessModifier());
    } else if(utype.type == VAR) {
        field.type = VAR;
        field.modifiers.add(PUBLIC);
    }
    else {
        field.type = UNDEFINED;
    }

    field.fullName = param_name;
    field.address = uniqueSerialId++;
    field.name = param_name;
    field.isArray = utype.array;
    field.note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(pAst->line), pAst->line, pAst->col);

    if(utype.type == CLASSFIELD)
        return *utype.field;

    return field;
}

void RuntimeEngine::parseMethodParams(List<Param>& params, KeyPair<List<string>, List<ResolvedReference>> fields, Ast* pAst) {
    for(unsigned int i = 0; i < fields.key.size(); i++) {
        if(containsParam(params, fields.key.get(i))) {
            errors->createNewError(SYMBOL_ALREADY_DEFINED, pAst->line, pAst->col, "symbol `" + fields.key.get(i) + "` already defined in the scope");
        } else
            params.add(Param(fieldMapToField(fields.key.get(i), fields.value.get(i), pAst)));
    }
}

KeyPair<string, ResolvedReference> RuntimeEngine::parseUtypeArg(Ast* ast) {
    KeyPair<string, ResolvedReference> utype_arg;
    utype_arg.value = parseUtype(ast).utype;
    if(ast->getEntityCount() != 0)
        utype_arg.key = ast->getEntity(0).getToken();
    else
        utype_arg.key = "";

    return utype_arg;
}

KeyPair<List<string>, List<ResolvedReference>> RuntimeEngine::parseUtypeArgList(Ast* ast) {
    KeyPair<List<string>, List<ResolvedReference>> utype_argmap;
    KeyPair<string, ResolvedReference> utype_arg;
    utype_argmap.key.init();
    utype_argmap.value.init();

    for(unsigned int i = 0; i < ast->getSubAstCount(); i++) {
        utype_arg = parseUtypeArg(ast->getSubAst(i));
        utype_argmap.key.push_back(utype_arg.key);
        utype_argmap.value.push_back(utype_arg.value);
    }

    return utype_argmap;
}

void RuntimeEngine::parseMethodReturnType(Expression& expression, Method& method) {
    method.array = expression.utype.array;
    if(expression.type == expression_class) {
        method.type = CLASS;
        method.klass = expression.utype.klass;
    } else if(expression.type == expression_native) {
        method.type = expression.utype.type;
    } else {
        method.type = UNDEFINED;
        errors->createNewError(GENERIC, expression.link->line, expression.link->col, "expected class or native type for method's return value");
    }
}

void RuntimeEngine::resolveMethodDecl(Ast* ast) {
    Scope* scope = currentScope();
    List<AccessModifier> modifiers;
    int startpos=1;

    if(parseAccessDecl(ast, modifiers, startpos)){
        parseMethodAccessModifiers(modifiers, ast);
    } else {
        modifiers.add(PUBLIC);
    }

    List<Param> params;
    string name =  ast->getEntity(startpos).getToken();
    parseMethodParams(params, parseUtypeArgList(ast->getSubAst(ast_utype_arg_list)), ast->getSubAst(ast_utype_arg_list));

    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    Expression utype;
    Method method = Method(name, currentModule, scope->klass, params, modifiers, NULL, note, sourceFiles.indexof(activeParser->sourcefile));
    if(ast->hasSubAst(ast_method_return_type)) {
        utype = parseUtype(ast->getSubAst(ast_method_return_type));
        parseMethodReturnType(utype, method);
    } else
        method.type = TYPEVOID;

    method.address = methods++;
    if(!scope->klass->addFunction(method)) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                               "function `" + name + "` is already defined in the scope");
        printNote(scope->klass->getFunction(name, params)->note, "function `" + name + "` previously defined here");
    }
}

Operator stringToOp(string op) {
    if(op=="+")
        return oper_PLUS;
    if(op=="-")
        return oper_MINUS;
    if(op=="*")
        return oper_MULT;
    if(op=="/")
        return oper_DIV;
    if(op=="%")
        return oper_MOD;
    if(op=="++")
        return oper_INC;
    if(op=="--")
        return oper_DEC;
    if(op=="=")
        return oper_EQUALS;
    if(op=="==")
        return oper_EQUALS_EQ;
    if(op=="+=")
        return oper_PLUS_EQ;
    if(op=="-=")
        return oper_MIN_EQ;
    if(op=="*=")
        return oper_MULT_EQ;
    if(op=="/=")
        return oper_DIV_EQ;
    if(op=="&=")
        return oper_AND_EQ;
    if(op=="|=")
        return oper_OR_EQ;
    if(op=="!=")
        return oper_NOT_EQ;
    if(op=="%=")
        return oper_MOD_EQ;
    if(op=="!")
        return oper_NOT;
    if(op=="<<")
        return oper_SHL;
    if(op==">>")
        return oper_SHR;
    if(op=="<")
        return oper_LESSTHAN;
    if(op==">")
        return oper_GREATERTHAN;
    if(op=="<=")
        return oper_LTEQ;
    if(op==">=")
        return oper_GTEQ;
    return oper_UNDEFINED;
}

void RuntimeEngine::resolveOperatorDecl(Ast* ast) {
    Scope* scope = currentScope();
    List<AccessModifier> modifiers;
    int startpos=2;

    if(parseAccessDecl(ast, modifiers, startpos)){
        parseMethodAccessModifiers(modifiers, ast);
    } else {
        modifiers.add(PUBLIC);
    }

    List<Param> params;
    string op =  ast->getEntity(startpos).getToken();
    parseMethodParams(params, parseUtypeArgList(ast->getSubAst(ast_utype_arg_list)), ast->getSubAst(ast_utype_arg_list));

    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    Expression utype;
    OperatorOverload operatorOverload = OperatorOverload(note, scope->klass, params, modifiers, NULL, stringToOp(op), sourceFiles.indexof(activeParser->sourcefile), op);
    if(ast->hasSubAst(ast_method_return_type)) {
        utype = parseUtype(ast->getSubAst(ast_method_return_type)->getSubAst(ast_utype));
        parseMethodReturnType(utype, operatorOverload);
    } else
        operatorOverload.type = TYPEVOID;

    operatorOverload.address = methods++;

    if(!scope->klass->addOperatorOverload(operatorOverload)) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                               "function `" + op + "` is already defined in the scope");
        printNote(scope->klass->getOverload(stringToOp(op), params)->note, "function `" + op + "` previously defined here");
    }
}

void RuntimeEngine::parsConstructorAccessModifiers(List<AccessModifier> &modifiers, Ast * ast) {
    if(modifiers.size() > 1)
        this->errors->createNewError(GENERIC, ast->line, ast->col, "too many access specifiers");
    else {
        AccessModifier mod = modifiers.get(0);

        if(mod != PUBLIC && mod != PRIVATE && mod != PROTECTED)
            this->errors->createNewError(INVALID_ACCESS_SPECIFIER, ast->line, ast->col,
                                   " `" + ast->getEntity(0).getToken() + "`");
    }
}

void RuntimeEngine::resolveConstructorDecl(Ast* ast) {
    Scope* scope = currentScope();
    List<AccessModifier> modifiers;
    int startpos=0;


    if(parseAccessDecl(ast, modifiers, startpos)){
        parsConstructorAccessModifiers(modifiers, ast);
    } else {
        modifiers.add(PUBLIC);
    }

    List<Param> params;
    string name = ast->getEntity(startpos).getToken();

    if(name == scope->klass->getName()) {
        parseMethodParams(params, parseUtypeArgList(ast->getSubAst(ast_utype_arg_list)), ast->getSubAst(ast_utype_arg_list));
        RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                       ast->line, ast->col);

        Method method = Method(name, currentModule, scope->klass, params, modifiers, NULL, note, sourceFiles.indexof(activeParser->sourcefile));
        method.type = TYPEVOID;
        method.isConstructor=true;

        method.address = methods++;
        if(!scope->klass->addConstructor(method)) {
            this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                   "constructor `" + name + "` is already defined in the scope");
            printNote(scope->klass->getConstructor(params)->note, "constructor `" + name + "` previously defined here");
        }
    } else
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                               "constructor `" + name + "` must be the same name as its parent");
}

void RuntimeEngine::addDefaultConstructor(ClassObject* klass, Ast* ast) {
    List<Param> emptyParams;
    Scope* scope = currentScope();
    List<AccessModifier> modifiers;
    modifiers.add(PUBLIC);

    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    if(klass->getConstructor(emptyParams, false) == NULL) {
        Method method = Method(klass->getName(), currentModule, scope->klass, emptyParams, modifiers, NULL, note, sourceFiles.indexof(activeParser->sourcefile));

        method.isConstructor=true;
        method.address = methods++;
        klass->addConstructor(method);
    }
}

void RuntimeEngine::resolveClassDecl(Ast* ast) {
    Scope* scope = currentScope();
    Ast* block = ast->getSubAst(ast_block), *trunk;
    List<AccessModifier> modifiers;
    ClassObject* klass;
    int startpos=1;

    parseAccessDecl(ast, modifiers, startpos);
    string name =  ast->getEntity(startpos).getToken();

    if(scope->type == GLOBAL_SCOPE) {
        klass = getClass(currentModule, name);
        klass->setFullName(currentModule + "#" + name);
    }
    else {
        klass = scope->klass->getChildClass(name);
        klass->setFullName(scope->klass->getFullName() + "." + name);
    }

    if(resolvedFields)
        klass->address = classSize++;

    addScope(Scope(CLASS_SCOPE, klass));
    klass->setBaseClass(parseBaseClass(ast, ++startpos));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        trunk = block->getSubAst(i);

        switch(trunk->getType()) {
            case ast_class_decl:
                resolveClassDecl(trunk);
                break;
            case ast_var_decl:
                if(!resolvedFields)
                    resolveVarDecl(trunk);
                break;
            case ast_method_decl:
                if(resolvedFields)
                    resolveMethodDecl(trunk);
                break;
            case ast_operator_decl:
                if(resolvedFields)
                    resolveOperatorDecl(trunk);
                break;
            case ast_construct_decl:
                if(resolvedFields)
                    resolveConstructorDecl(trunk);
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << trunk->getType();
                errors->createNewError(INTERNAL_ERROR, trunk->line, trunk->col, err.str());
                break;
        }
    }

    if(resolvedFields)
        addDefaultConstructor(klass, ast);
    removeScope();
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
    if(lastNoteMsg != msg && lastNote.getLine() != note.getLine()
            && !noteMessages.find(msg))
    {
        cout << note.getNote(msg);
        lastNoteMsg = msg;
        noteMessages.push_back(msg);
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

void RuntimeEngine::removeScope() {
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
        modifiers.add(PROTECTED);
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
    removeScope();
}

void RuntimeEngine::parseVarDecl(Ast *ast)
{
    Scope* scope = currentScope();
    List<AccessModifier> modifiers;
    int startpos=0;


    if(parseAccessDecl(ast, modifiers, startpos)){
        parseVarAccessModifiers(modifiers, ast);
    } else {
        modifiers.add(PUBLIC);
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
        modifiers.add(PUBLIC);
    }
}

void RuntimeEngine::generate() {

}

void RuntimeEngine::cleanup() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        parsers.get(i)->free();
    }
    parsers.free();
    if(errors != NULL)
    {
        errors->free();
        delete(errors); errors = NULL;
    }
    modules.free();

    for(unsigned long i = 0; i < importMap.size(); i++) {
        importMap.get(i).value.free();
        importMap.get(i).key.clear();
    }

    for(unsigned long i = 0; i < inline_map.size(); i++) {
        inline_map.get(i).key.clear();
    }

    importMap.free();
    freeList(classes);
    freeList(scopeMap);
    sourceFiles.free();
    lastNoteMsg.clear();
    exportFile.clear();
    noteMessages.free();
    inline_map.free();
}
