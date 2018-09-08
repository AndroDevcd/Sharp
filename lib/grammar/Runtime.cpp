//
// Created by bknun on 9/12/2017.
//

#ifndef VISUAL_STUDIOS
#include <dirent.h>
#endif
#ifdef VISUAL_STUDIOS
#include "../support/visual-studios/vc/include/dirent.h"
#endif
#include <sys/stat.h>
#include <cmath>
#include "../../stdimports.h"
#ifdef POSIX_
#include <cmath>
#endif
#include "Runtime.h"
#include "parser/ErrorManager.h"
#include "List.h"
#include "parser/Parser.h"
#include "../util/File.h"
#include "../runtime/Opcode.h"
#include "../runtime/register.h"
#include "Asm.h"
#include "../runtime/Exe.h"
#include "Optimizer.h"
#include "../util/zip/zlib.h"
#include "../util/time.h"
#include "Method.h"

using namespace std;

unsigned long RuntimeEngine::uniqueSerialId = 0;
unsigned long RuntimeEngine::uniqueDelegateId = 0;
string globalClass = "__SRT_Global";

/*
 * How many instructions we optimized out
 */
unsigned long long optimizationResult = 0;
options c_options;
List<string> origFiles;

void help();
void get_full_file_list(native_string &path, List<native_string> &files);

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
        else if(opt("-bootstrap")){
            c_options.compileBootstrap = true;
        }
        else if(opt("-o")){
            if(i+1 >= argc)
                rt_error("output file required after option `-o`");
            else
                c_options.out = string(argv[++i]);
        }
        else if(opt("-L")){
            if(i+1 >= argc)
                rt_error("library directory required after option `-L`");
            else
                c_options.libraries.add(string(argv[++i]));
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
            c_options.debug = false;
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
                } else if(c_options.error_limit <= 0) {
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

    if(!c_options.compileBootstrap && files.size() == 0){
        help();
        return 1;
    }
#ifdef WIN32_
    native_string path("C:/Sharp/include");
#endif
#ifdef POSIX_
    native_string path("/usr/include/sharp/");
#endif

    origFiles.addAll(files);
    List<native_string> includes;
    get_full_file_list(path, includes);

    for(long i = 0; i < c_options.libraries.size(); i++) {
        path = c_options.libraries.get(i);
        get_full_file_list(path, includes);
    }

    for(long i = 0; i < includes.size(); i++)
        files.add(includes.get(i).str());

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

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

struct stat result;
void get_full_file_list(native_string &path, List<native_string> &files) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path.str().c_str())) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            if (!ent->d_name || ent->d_name[0] == '.') continue;
            native_string file;
            file = path.str() + "/" + string(ent->d_name);

            if(stat(file.str().c_str(), &result) == 0 && S_ISDIR(result.st_mode)) {
                native_string folder(file.str() + "/");
                get_full_file_list(folder, files);
                continue;
            }

            if(ends_with(file.str(), ".sharp")) {
                files.push_back();
                files.last().init();
                files.last() = file;
            }
        }
        closedir (dir);
    } else {
        /* could not open directory */
                cout << "warning: could not find support library files in path `" << path.str() << "`, do you have them installed?" << endl;
    }
}

void help() {
    cout << "Usage: sharpc " << "{OPTIONS} SOURCE FILE(S)" << std::endl;
    cout << "Source file must have a .sharp extion to be compiled.\n" << endl;
    cout << "[-options]\n\n    -V                print compiler version and exit"                       << endl;
    cout <<               "    -showversion      print compiler version and continue"                   << endl;
    cout <<               "    -o<file>          set the output object file"                            << endl;
    cout <<               "    -c                compile only and do not generate exe"                  << endl;
    cout <<               "    -a                enable aggressive error reporting"                     << endl;
    cout <<               "    -s                strip debugging info"                                 << endl;
    cout <<               "    -O                optimize executable"                                   << endl;
    cout <<               "    -L<path>          library directory path"                                << endl;
    cout <<               "    -w                disable all warnings"                                  << endl;
    cout <<               "    -errlmt<count>    set max errors the compiler allows before quitting"    << endl;
    cout <<               "    -v<version>       set the application version"                           << endl;
    cout <<               "    -unsafe -u        allow unsafe code"                                     << endl;
    cout <<               "    -objdmp           create dump file for generated assembly"               << endl;
    cout <<               "    -target           target the specified platform of sharp to run on"      << endl;
    cout <<               "    -werror           enable warnings as errors"                             << endl;
    cout <<               "    -release -r       generate a release build exe"                          << endl;
    cout <<               "    --h -?            display this help message"                             << endl;
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

                errors+= p->getErrors()->getErrorCount() == 0 ? p->getErrors()->getUnfilteredErrorCount() : p->getErrors()->getErrorCount();
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
            cout << "performing syntax analysis on project files"<< endl;

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
    cout << std::flush << std::flush;
}

void RuntimeEngine::compile()
{
    if(preprocess()) {
        if(c_options.magic) {
            List<string> lst;
            lst.addAll(modules);

            for(unsigned long i = 0; i < parsers.size(); i++) {
                Parser* p = parsers.get(i);
                bool found = false;

                for(unsigned int c = 0; c < importMap.size(); c++) {
                    if(importMap.get(c).key == p->sourcefile) {
                        importMap.get(c).value.addAll(modules);
                        found = true;
                        break;
                    }
                }

                if(!found) {
                    importMap.add(KeyPair<std::string, List<std::string>>(p->sourcefile, lst));
                }
            }

            lst.free();
        }

        resolveAllGenerics();
        resolveAllFields();
        resolveAllMethods();
        resolveClassBases();
        resolveAllGenericMethodsParams();
        resolveAllGenericMethodsReturns();
        resolveAllEnums();
        inheritObjectClass();
        resolveAllGlobalFields();
        inlineFields();
        resolveAllInterfaces();
        resolveAllDelegates();

        // TODO: make checkSupportClasses() to check on vital support classes to make sure things are as they should be
        // TODO: inforce const on variables
        preprocessed = true;
        for(unsigned long i = 0; i < parsers.size(); i++) {
            Parser *p = parsers.get(i);
            activeParser = p;
            currentModule = "$unknown";

            errors = new ErrorManager(p->lines, p->sourcefile, true, c_options.aggressive_errors);

            Ast* ast;
            addScope(Scope(GLOBAL_SCOPE, NULL));
            for(size_t c = 0; c < p->treesize(); c++) {
                ast = p->ast_at(c);
                SEMTEX_CHECK_ERRORS

                switch(ast->getType()) {
                    case ast_module_decl:
                        currentModule = parseModuleName(ast);
                        break;
                    case ast_import_decl:
                        analyzeImportDecl(ast);
                        break;
                    case ast_class_decl:
                        analyzeClassDecl(ast);
                        break;
                    case ast_interface_decl: /* ignore */
                        break;
                    case ast_generic_class_decl: /* ignore */
                        break;
                    case ast_generic_interface_decl: /* ignore */
                        break;
                    case ast_enum_decl:
                        break;
                    case ast_method_decl:
                        addScope(Scope(CLASS_SCOPE, getClass(currentModule, globalClass, classes)));
                        parseMethodDecl(ast);
                        removeScope();
                        break;
                    case ast_var_decl:
                        addScope(Scope(CLASS_SCOPE, getClass(currentModule, globalClass, classes)));
                        analyzeVarDecl(ast);
                        removeScope();
                        break;
                    default:
                        stringstream err;
                        err << ": unknown ast type: " << ast->getType();
                        errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
                        break;
                }

            }


            if((i+1) >= parsers.size() && errors->getErrorCount() == 0 && errors->getUnfilteredErrorCount() == 0) {
                getMainMethod(p);

                if(errors->getErrorCount() == 0 && errors->getUnfilteredErrorCount() > 0) {
                    errors->enableAggressive();
                    errors->printErrors();
                }
            }

            if(errors->hasErrors()){
                report:

                errorCount+= errors->getErrorCount();
                unfilteredErrorCount+= errors->getUnfilteredErrorCount();

                failedParsers.addif(activeParser->sourcefile);
                succeededParsers.removefirst(activeParser->sourcefile);
            } else {
                succeededParsers.addif(activeParser->sourcefile);
                failedParsers.removefirst(activeParser->sourcefile);
            }

            removeScope();
            errors->free();
            delete (errors); this->errors = NULL;
        }
    }
}

void RuntimeEngine::setHeadClass(ClassObject *klass) {
    ClassObject* sup = klass->getBaseClass();
    if(sup == NULL) {
        klass->setHead(klass);
        return;
    }
    while(true) {
        if(sup->getBaseClass() == NULL) {
            klass->setHead(sup);
            return;
        } else
            sup = sup->getBaseClass();
    }
}

void RuntimeEngine::addLine(Block& block, Ast *pAst) {

    currentScope()->currentFunction->line_table.add(KeyPair<long, int64_t>(pAst->line-1, block.code.__asm64.size()));
}

void RuntimeEngine::parseReturnStatement(Block& block, Ast* pAst) { // TODO: fix return sign of new
    Expression returnVal(pAst), value(pAst);
    currentScope()->reachable=false;
    currentScope()->last_statement=ast_return_stmnt;

    if(pAst->hasSubAst(ast_value)) {
        value = parseValue(pAst->getSubAst(ast_value));
        if(!value.newExpression)
            block.code.inject(block.code.__asm64.size(), value.code);

        switch(value.type) {
            case expression_var:
                if(value.newExpression) {
                    block.code.inject(block.code.__asm64.size(), value.code);
                    block.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                    block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                } else {
                    if (value.func) {
                        // TODO: pull value from function
                        if(value.isArray()) {
                            block.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                            block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                        } else {
                            Expression out(pAst);
                            pushExpressionToRegisterNoInject(value, out, i64ebx);
                            block.code.inject(block.code.__asm64.size(), out.code);
                            block.code.push_i64(SET_Di(i64, op_RETURNVAL, i64ebx));
                        }
                    } else {
                        if(value.isArray()) {
                            block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                        } else {
                            if(value.inCmtRegister) {
                                block.code.push_i64(SET_Di(i64, op_RETURNVAL, i64cmt));
                            } else {
                                Expression out(pAst);
                                pushExpressionToRegisterNoInject(value, out, i64ebx);
                                block.code.inject(block.code.__asm64.size(), out.code);
                                block.code.push_i64(SET_Di(i64, op_RETURNVAL, i64ebx));
                            }
                        }
                    }
                }
                break;
            case expression_field:
                if(value.trueType() == VAR && !value.isArray()) {
                    if(value.utype.field.local) {
                        Expression out(pAst);
                        pushExpressionToRegisterNoInject(value, out, i64ebx);
                        block.code.inject(block.code.__asm64.size(), out.code);
                        block.code.push_i64(SET_Di(i64, op_RETURNVAL, i64ebx));
                    } else {
                        block.code.push_i64(SET_Di(i64, op_MOVI, 0), i64adx);
                        block.code.push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx,0, i64adx));
                        block.code.push_i64(SET_Di(i64, op_RETURNVAL, i64ebx));
                    }
                } else if(value.trueType() == VAR && value.isArray()) {
                    block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                } else if(value.trueType() == OBJECT || value.trueType() == CLASS) {
                    block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                }
                break;
            case expression_lclass:
                if(value.newExpression) {
                    block.code.inject(block.code.__asm64.size(), value.code);
                    block.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                    block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                } else {
                    if(value.func) {
                        /* I think we do nothing? */
                        block.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                        block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                    } else {
                        block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                    }
                }
                break;
            case expression_string:
                block.code.push_i64(SET_Di(i64, op_MOVL, 0));
                block.code.push_i64(SET_Di(i64, op_NEWSTRING, value.intValue));
                break;
            case expression_null:
                block.code.push_i64(SET_Di(i64, op_MOVL, 0));
                block.code.push_i64(SET_Ei(i64, op_DEL));
                break;
            case expression_objectclass:
                if(value.newExpression) {
                    block.code.inject(block.code.__asm64.size(), value.code);
                    block.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                    block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                } else {
                    if(value.func) {
                        /* I think we do nothing? */
                        block.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                        block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                    } else {
                        if(value.utype.type == CLASSFIELD) {
                            block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                        } else {
                            block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                        }
                    }
                }
                break;
        }
    } else {
        value.type = expression_void;
        if(currentScope()->currentFunction->isConstructor) {
            block.code.push_i64(SET_Di(i64, op_MOVSL, 0));
            block.code.push_i64(SET_Ei(i64, op_RETURNOBJ));
        }
    }


    block.code.push_i64(SET_Ei(i64, op_RET));
    returnVal.type = methodReturntypeToExpressionType(currentScope()->currentFunction);
    if(returnVal.type == expression_lclass) {
        returnVal.utype.klass = currentScope()->currentFunction->klass;
        returnVal.utype.type = CLASS;
    }
    returnVal.utype.array = currentScope()->currentFunction->array;
    equals(returnVal, value, ": Returning `" + value.typeToString() + "` from a function returning `" + returnVal.typeToString() + "`");
}

void RuntimeEngine::parseIfStatement(Block& block, Ast* pAst) {
    Expression cond(pAst), out(pAst);
    currentScope()->uniqueLabelSerial++;
    cond = parseExpression(pAst->getSubAst(ast_expression));

    string ifEndLabel, ifBlockEnd, ifCondEnd;
    stringstream ss;
    ss << generic_label_id << currentScope()->uniqueLabelSerial;
    ifEndLabel=ss.str(); ss.str("");

    if(!cond.inCmtRegister)
        pushExpressionToRegister(cond, out, i64cmt);
    else
        out.inject(cond);
    block.code.inject(block.code.size(), out.code);

    ss << generic_label_id << "condition" << ++currentScope()->uniqueLabelSerial;
    ifCondEnd=ss.str(); ss.str("");
    currentScope()->label_map.add(KeyPair<string,int64_t>(ifCondEnd, __init_label_address(block.code)));


    if(pAst->getSubAstCount() > 2) {
        ss << generic_label_id << ++currentScope()->uniqueLabelSerial;
        ifBlockEnd = ss.str(); ss.str("");

        currentScope()->addStore(ifBlockEnd, i64adx, 2, block.code,
                        pAst->getSubAst(ast_expression)->line, pAst->getSubAst(ast_expression)->col);
        block.code.push_i64(SET_Ei(i64, op_IFNE));

        parseBlock(pAst->getSubAst(ast_block), block);

        if(!currentScope()->reachable && (currentScope()->last_statement==ast_return_stmnt
                                 || currentScope()->last_statement == ast_throw_statement)) {
            currentScope()->reachable=true;
        }

        currentScope()->label_map.add(KeyPair<string,int64_t>(ifBlockEnd, __init_label_address(block.code)));


        currentScope()->addBranch(ifEndLabel, 1, block.code, pAst->getSubAst(ast_expression)->line,
                         pAst->getSubAst(ast_expression)->col);

        Ast* ast;
        for(unsigned int i = 2; i < pAst->getSubAstCount(); i++) {
            ast = pAst->getSubAst(i);
            switch(ast->getType()) {
                case ast_elseif_statement:
                    cond = parseExpression(ast->getSubAst(ast_expression));

                    out.free();
                    pushExpressionToRegister(cond, out, i64cmt);
                    block.code.inject(block.code.size(), out.code);
                    ss << generic_label_id << "condition" << ++currentScope()->uniqueLabelSerial;
                    ifCondEnd=ss.str(); ss.str("");
                    currentScope()->label_map.add(KeyPair<string,int64_t>(ifCondEnd, __init_label_address(block.code)));


                    ss << generic_label_id << ++currentScope()->uniqueLabelSerial;
                    ifBlockEnd = ss.str(); ss.str("");

                    currentScope()->addStore(ifBlockEnd, i64adx, 2, block.code,
                                    ast->getSubAst(ast_expression)->line, ast->getSubAst(ast_expression)->col);
                    block.code.push_i64(SET_Ei(i64, op_IFNE));


                    parseBlock(ast->getSubAst(ast_block), block);

                    currentScope()->label_map.add(KeyPair<string,int64_t>(ifBlockEnd, __init_label_address(block.code)));

                    if(!currentScope()->reachable && (currentScope()->last_statement==ast_return_stmnt
                                             || currentScope()->last_statement == ast_throw_statement)) {
                        currentScope()->reachable=true;
                    }

                    if(c_options.optimize && !((i+1) < pAst->getSubAstCount())) {}
                    else
                        currentScope()->addBranch(ifEndLabel, 1, block.code, ast->getSubAst(ast_expression)->line,
                                     ast->getSubAst(ast_expression)->col);
                    break;
                case ast_else_statement:
                    parseBlock(ast->getSubAst(ast_block), block);
                    break;
            }
        }

        if(!currentScope()->reachable && (currentScope()->last_statement==ast_return_stmnt
                                          || currentScope()->last_statement == ast_throw_statement)) {
            currentScope()->reachable=true;
        }
    } else {
        currentScope()->addStore(ifEndLabel, i64adx, 1, block.code, pAst->getSubAst(ast_expression)->line,
                        pAst->getSubAst(ast_expression)->col);
        block.code.push_i64(SET_Ei(i64, op_IFNE));
        parseBlock(pAst->getSubAst(ast_block), block);

        if(!currentScope()->reachable && (currentScope()->last_statement==ast_return_stmnt
                                 || currentScope()->last_statement == ast_throw_statement)) {
            currentScope()->reachable=true;
        }
    }

    block.code.push_i64(SET_Ei(i64, op_NOP));
    currentScope()->label_map.add(KeyPair<string,int64_t>(ifEndLabel, __init_label_address(block.code)));
    block.code.push_i64(SET_Ei(i64, op_NOP));
}

void RuntimeEngine::parseAssemblyBlock(Block& block, Ast* pAst) {
    string assembly = "";

    if(pAst->getEntityCount() == 1) {
        if(File::exists(pAst->getEntity(0).getToken().c_str())) {
            File::buffer __ostream;
            File::read_alltext(pAst->getEntity(0).getToken().c_str(), __ostream);

            assembly = __ostream.to_str();
            __ostream.end();
        } else {
            assembly = pAst->getEntity(0).getToken();
        }
    } else {
        for(unsigned int i = 0; i < pAst->getEntityCount(); i++) {
            assembly += pAst->getEntity(i).getToken() + "\n";
        }
    }

    Asm __vasm;
    __vasm.parse(block.code, this, assembly, pAst);
}

void RuntimeEngine::parseAssemblyStatement(Block& block, Ast* pAst) {
    KeyPair<int64_t, int64_t> assembly_section;
    assembly_section.key = block.code.size();
    if(c_options.unsafe || !origFiles.find(activeParser->sourcefile))
        parseAssemblyBlock(block, pAst->getSubAst(ast_assembly_block));
    else
        errors->createNewError(GENERIC, pAst, "calling asm without unsafe mode enabled. try recompiling your code with [-unsafe]");

    assembly_section.value = block.code.size();
    block.assembly_table.push_back(assembly_section);
}

bool RuntimeEngine::validateLocalField(std::string name, Ast* pAst) {
    KeyPair<int, Field>* field;

    if((field = currentScope()->getLocalField(name)) != NULL) {
        if(currentScope()->blocks == field->key) {
            // err redefinition of parameter
            errors->createNewError(DUPlICATE_DECLIRATION, pAst, " local variable `" + field->value.name + "`");
            return false;
        } else {
            createNewWarning(GENERIC, pAst->line, pAst->col, " local variable `" + field->value.name + "` hides previous declaration in higher scope");
            return true;
        }
    } else {
        return true;
    }
}

Field RuntimeEngine::utypeArgToField(KeyPair<string, ResolvedReference> arg) {
    Field field;
    field.name = arg.key;
    field.isArray = arg.value.array;
    field.fullName = field.name;
    field.klass = arg.value.klass;
    field.modifiers.add(PUBLIC);
    field.type = arg.value.type;

    return field;
}

void RuntimeEngine::parseUtypeArg(Ast *pAst, Scope *scope, Block &block, Expression* comparator) {
    if(pAst->hasSubAst(ast_utype_arg)) {
        KeyPair<string, ResolvedReference> utypeArg = parseUtypeArg(pAst->getSubAst(ast_utype_arg));
        Expression value(pAst), out(pAst);

        if(pAst->hasSubAst(ast_value)) {
            if(comparator != NULL)
                errors->createNewError(UNEXPECTED_SYMBOL, pAst->getSubAst(ast_value), " `;` expected");
            value = parseValue(pAst->getSubAst(ast_value));
        }

        if(validateLocalField(utypeArg.key, pAst->getSubAst(ast_utype_arg))) {
            if(utypeArg.value.type == CLASSFIELD) {
                errors->createNewError(COULD_NOT_RESOLVE, pAst->getSubAst(ast_utype_arg), " `" + utypeArg.value.field.name + "`");
            }

            KeyPair<int, Field> local;
            local.set(currentScope()->blocks, utypeArgToField(utypeArg));
            local.value.address = currentScope()->currentFunction->localVariables++;;
            local.value.local=true;
            currentScope()->locals.push_back(local);

            Expression fieldExpr = fieldToExpression(pAst, local.value);

            if(value.type != expression_unknown) {
                equals(fieldExpr, value);

                token_entity operand("=", SINGLE, 0,0, ASSIGN);
                Field *field = &currentScope()->getLocalField(utypeArg.key)->value;
                if(field->type==CLASS && field->klass->getModuleName() == "std" && field->klass->getName() == "string"
                   && value.type == expression_string && !field->isArray) {
                    constructNewNativeClass("string", "std", value, out, true);
                    out.code.push_i64(SET_Di(i64, op_POPL, field->address));
                } else if(field->type==CLASS && field->klass->getModuleName() == "std" &&
                          (field->klass->getName() == "int" || field->klass->getName() == "bool"
                           || field->klass->getName() == "char" || field->klass->getName() == "long"
                           || field->klass->getName() == "short" || field->klass->getName() == "string"
                           || field->klass->getName() == "uchar" || field->klass->getName() == "ulong"
                           || field->klass->getName() == "ushort")
                          && value.type == expression_var && !field->isArray) {
                    constructNewNativeClass(field->klass->getName(), "std", value, out, true);
                    out.code.push_i64(SET_Di(i64, op_POPL, field->address));
                } else
                    assignValue(operand, out, fieldExpr, value, pAst);
                block.code.inject(block.code.size(), out.code);
            }
        }
    }
}

int64_t RuntimeEngine::get_label(string label) {
    for(unsigned int i = 0; i < currentScope()->label_map.size(); i++) {
        if(currentScope()->label_map.get(i).key == label)
            return currentScope()->label_map.get(i).value;
    }

    return 0;
}

void RuntimeEngine::parseForStatement(Block& block, Ast* pAst) {
    Expression cond(pAst), iter(pAst);
    currentScope()->blocks++;
    currentScope()->loops++;
    currentScope()->uniqueLabelSerial++;
    currentScope()->brahchHelper.add(SCOPE_FOR_LOOP);
    stringstream ss;
    string forEndLabel, forBeginLabel, forIterLabel;

    parseUtypeArg(pAst, currentScope(), block);

    ss.str("");
    ss << for_label_begin_id << currentScope()->uniqueLabelSerial;
    forBeginLabel=ss.str();
    currentScope()->label_map.add(KeyPair<std::string, int64_t>(forBeginLabel,__init_label_address(block.code)));

    ss.str("");
    ss << for_label_end_id << currentScope()->uniqueLabelSerial;
    forEndLabel=ss.str();

    ss.str("");
    ss << for_label_iter_id << currentScope()->uniqueLabelSerial;
    forIterLabel=ss.str();

    KeyPair<string, string> loopMap(forBeginLabel, forEndLabel);
    currentScope()->loopAddressTable.push_back(loopMap);
    if(pAst->hasSubAst(ast_for_expresion_cond)) {
        Expression out(pAst);
        cond = parseExpression(pAst->getSubAst(ast_for_expresion_cond));

        // This prevents stupid things like reverse copying that can just be avoided
        if(c_options.optimize && cond.inCmtRegister) {
            out.inject(cond);

            List<int64_t> &i64 = out.code.__asm64;
            i64.pop_back();
        } else {
            pushExpressionToRegister(cond, out, i64cmt);
        }

        block.code.inject(block.code.size(), out.code);

        currentScope()->addStore(forEndLabel, i64adx, 1, block.code, pAst->line, pAst->col);
        block.code.push_i64(SET_Ei(i64, op_IFNE));
    }

    if(pAst->hasSubAst(ast_for_expresion_iter)) {
        iter = parseExpression(pAst->getSubAst(ast_for_expresion_iter));
    }

    currentScope()->blocks--;
    parseBlock(pAst->getSubAst(ast_block), block);

    if(!currentScope()->reachable && currentScope()->last_statement==ast_return_stmnt) {
        currentScope()->reachable=true;
    }

    currentScope()->label_map.add(KeyPair<std::string, int64_t>(forIterLabel,__init_label_address(block.code)));

    if(pAst->hasSubAst(ast_for_expresion_iter)) {

        block.code.inject(block.code.size(), iter.code);
        if(iter.func && iter.type != expression_void) {
            block.code.push_i64(SET_Ei(i64, op_POP));
        }
    }

    block.code.push_i64(SET_Di(i64, op_GOTO, (get_label(forBeginLabel)+1)));
    currentScope()->label_map.add(KeyPair<std::string, int64_t>(forEndLabel,__init_label_address(block.code)));

    currentScope()->loops--;
    currentScope()->brahchHelper.pop_back();
    currentScope()->loopAddressTable.pop_back();
}

void RuntimeEngine::getArrayValueOfExpression(Expression& expr, Expression& out) {
    switch(expr.type) {
        case expression_var:
            out.type=expression_var;
            out.code.push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx,0, i64ebx));
            break;
        case expression_lclass:
            out.type=expression_lclass;
            out.utype.klass = expr.utype.klass;
            out.code.push_i64(SET_Di(i64, op_MOVND, i64ebx));
            break;
        case expression_field:
            if(expr.trueType() == VAR) {
                out.type=expression_var;
                out.code.push_i64(SET_Ci(i64, op_IALOAD_2, i64ebx,0, i64ebx));
            } else if(expr.trueType() == OBJECT) {
                out.type=expression_objectclass;
                out.utype.type=OBJECT;
                out.code.push_i64(SET_Di(i64, op_MOVND, i64ebx));
            }
            else {
                out.type=expression_lclass;
                out.utype.klass = expr.utype.field.klass;
                out.code.push_i64(SET_Di(i64, op_MOVND, i64ebx));
            }
            break;
        default:
            out=expr;
            out.code.push_i64(SET_Di(i64, op_MOVND, i64ebx));
            break;
    }
}

void RuntimeEngine::assignUtypeForeach(Ast *pAst, Scope *scope, Block &block, Expression& assignExpr) {
    if(pAst->hasSubAst(ast_utype_arg)) {
        KeyPair<string, ResolvedReference> utypeArg = parseUtypeArg(pAst->getSubAst(ast_utype_arg));
        Expression out(pAst);
        out.inject(assignExpr);

        KeyPair<int, Field>* local = currentScope()->getLocalField(utypeArg.key);
        Expression fieldExpr = fieldToExpression(pAst, local->value);

        token_entity operand("=", SINGLE, 0,0, ASSIGN);
        if(assignExpr.trueType() == OBJECT && fieldExpr.trueType() == CLASS) {
            out.code.push_i64(SET_Di(i64, op_MOVI, fieldExpr.utype.field.klass->address), i64cmt);
            out.code.push_i64(SET_Di(i64, op_CAST, i64cmt));
            out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            out.code.push_i64(SET_Di(i64, op_MOVL, local->value.address));
            out.code.push_i64(SET_Ei(i64, op_POPOBJ));
        } else if(assignExpr.trueType() == OBJECT && fieldExpr.trueType() == VAR) {
            out.code.push_i64(SET_Di(i64, op_VARCAST, fieldExpr.isArray()));
            out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            out.code.push_i64(SET_Di(i64, op_MOVL, local->value.address));
            out.code.push_i64(SET_Ei(i64, op_POPOBJ));
        } else if(assignExpr.trueType() == VAR && fieldExpr.trueType() == VAR) {
            out.code.push_i64(SET_Ci(i64, op_SMOVR_2, i64ebx, 0, local->value.address));
        } else if(assignExpr.trueType() == CLASS) {
            equals(assignExpr, fieldExpr);
            out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            out.code.push_i64(SET_Di(i64, op_MOVL, local->value.address));
            out.code.push_i64(SET_Ei(i64, op_POPOBJ));
        } else if(assignExpr.trueType() == OBJECT && fieldExpr.trueType() == OBJECT) {
            equals(assignExpr, fieldExpr);
            out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
            out.code.push_i64(SET_Di(i64, op_MOVL, local->value.address));
            out.code.push_i64(SET_Ei(i64, op_POPOBJ));
        } else {
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken()
                                                                    + "` cannot be applied to expression of type `" + fieldExpr.typeToString() + "` and `" + assignExpr.typeToString() + "`");
        }
        block.code.inject(block.code.size(), out.code);
    }
}

void RuntimeEngine::parseForEachStatement(Block& block, Ast* pAst) {
    currentScope()->blocks++;
    currentScope()->loops++;
    currentScope()->uniqueLabelSerial++;
    currentScope()->brahchHelper.add(SCOPE_FOR_LOOP);
    string forBeginLabel, forEndLabel, forIterLabel;

    Expression arryExpression(parseExpression(pAst->getSubAst(ast_expression))), out(pAst);
    parseUtypeArg(pAst, currentScope(), block, &arryExpression);

    /*
     * This is stupid but we do this so we dont mess up the refrence with out local array expression variable
     */
    arryExpression = parseExpression(pAst->getSubAst(ast_expression));

    block.code.push_i64(SET_Di(i64, op_MOVI, 0), i64ebx);
    block.code.push_i64(SET_Di(i64, op_RSTORE, i64ebx));

    if(!arryExpression.isArray()) {
        errors->createNewError(GENERIC, pAst->getSubAst(ast_expression), "expression must evaluate to type array");
    }

    stringstream ss;
    ss << for_label_begin_id << currentScope()->uniqueLabelSerial;
    forBeginLabel=ss.str();

    ss.str("");
    ss << for_label_end_id << currentScope()->uniqueLabelSerial;
    forEndLabel=ss.str();

    ss.str("");
    ss << for_label_iter_id << currentScope()->uniqueLabelSerial;
    forIterLabel=ss.str();

    KeyPair<string, string> loopMap(forBeginLabel, forEndLabel);
    currentScope()->loopAddressTable.push_back(loopMap);
    currentScope()->label_map.add(KeyPair<std::string, int64_t>(forBeginLabel,__init_label_address(block.code)));

    block.code.inject(block.code.size(), arryExpression.code);

    block.code.push_i64(SET_Ci(i64, op_SMOV, i64ebx,0, 0));
    block.code.push_i64(SET_Di(i64, op_SIZEOF, i64egx));
    block.code.push_i64(SET_Ci(i64, op_LT, i64ebx,0, i64egx));
    currentScope()->addStore(forEndLabel, i64adx, 1, block.code,
                    pAst->getSubAst(ast_block)->line, pAst->getSubAst(ast_block)->col);
    block.code.push_i64(SET_Ei(i64, op_IFNE));
    getArrayValueOfExpression(arryExpression, out);
    assignUtypeForeach(pAst, currentScope(), block, out);

    currentScope()->blocks--;
    parseBlock(pAst->getSubAst(ast_block), block);

    if(!currentScope()->reachable && currentScope()->last_statement==ast_return_stmnt) {
        currentScope()->reachable=true;
    }

    currentScope()->label_map.add(KeyPair<std::string, int64_t>(forIterLabel,__init_label_address(block.code)));
    block.code.push_i64(SET_Ci(i64, op_SMOV, i64ebx,0, 0));
    block.code.push_i64(SET_Di(i64, op_INC, i64ebx));
    block.code.push_i64(SET_Ci(i64, op_SMOVR, i64ebx,0, 0));
    block.code.push_i64(SET_Di(i64, op_GOTO, (get_label(forBeginLabel)+1)));
    currentScope()->label_map.add(KeyPair<std::string, int64_t>(forEndLabel,__init_label_address(block.code)));
    block.code.push_i64(SET_Ei(i64, op_POP));

    currentScope()->loops--;
    currentScope()->brahchHelper.pop_back();
    currentScope()->loopAddressTable.pop_back();
}

void RuntimeEngine::parseWhileStatement(Block& block, Ast* pAst) {
    currentScope()->loops++;
    currentScope()->uniqueLabelSerial++;
    string whileBeginLabel, whileEndLabel;
    currentScope()->brahchHelper.add(SCOPE_WHILE_LOOP);

    Expression cond = parseExpression(pAst->getSubAst(ast_expression)), out(pAst);

    stringstream ss;
    ss << while_label_begin_id << currentScope()->uniqueLabelSerial;
    whileBeginLabel=ss.str();

    ss.str("");
    ss << while_label_end_id << currentScope()->uniqueLabelSerial;
    whileEndLabel=ss.str();

    KeyPair<string, string> loopMap(whileBeginLabel, whileEndLabel);
    currentScope()->loopAddressTable.push_back(loopMap);
    currentScope()->label_map.add(KeyPair<std::string, int64_t>(whileBeginLabel,__init_label_address(block.code)));
    pushExpressionToRegister(cond, out, i64cmt);
    block.code.inject(block.code.size(), out.code);

    currentScope()->addStore(whileEndLabel, i64adx, 1, block.code,
                    pAst->getSubAst(ast_expression)->line, pAst->getSubAst(ast_expression)->col);
    block.code.push_i64(SET_Ei(i64, op_IFNE));

    parseBlock(pAst->getSubAst(ast_block), block);

    block.code.push_i64(SET_Di(i64, op_GOTO, (get_label(whileBeginLabel)+1)));
    currentScope()->label_map.add(KeyPair<std::string, int64_t>(whileEndLabel,__init_label_address(block.code)));
    currentScope()->loops--;
    currentScope()->brahchHelper.pop_back();
    currentScope()->loopAddressTable.pop_back();
}

void RuntimeEngine::parseSwitchStatement(Block& block, Ast* pAst) {
    string switchEndLabel;
    currentScope()->switches++;
    currentScope()->brahchHelper.add(SCOPE_SWITCH);

    Expression cond = parseExpression(pAst->getSubAst(ast_expression)), out(pAst);

    stringstream ss;
    ss << switch_label_end_id << ++currentScope()->switches;
    switchEndLabel=ss.str();

    pushExpressionToRegister(cond, out, i64ebx);
    block.code.inject(block.code.size(), out.code);
    block.code.push_i64(SET_Di(i64, op_SWITCH, currentScope()->currentFunction->switchTable.size()));

    SwitchTable st;

    Ast *blck = pAst->getSubAst(ast_switch_block), *ast, *sub;
    for(long i = 0; i < blck->getSubAstCount(); i++) { // get All values processed
        ast = blck->getSubAst(i);
        switch(ast->getType()) {
            case ast_switch_declarator:
                string decl = ast->getEntity(0).getToken();
                if(decl == "case") {
                    Expression constExpr = parseExpression(ast->getSubAst(ast_expression));
                    if(!constExpr.isEnum() && !constExpr.isConstExpr()) {
                        errors->createNewError(GENERIC, ast, "a constant value is expected");
                    }
                    if(st.values.find(constantExpressionToValue(ast, constExpr))) {
                        stringstream s;
                        s << constantExpressionToValue(ast, constExpr);
                        errors->createNewError(GENERIC, ast, "switch statement contains multiple cases with value `" + s.str() + "`");
                    }
                    if(!isWholeNumber(constantExpressionToValue(ast, constExpr))) {
                        errors->createNewError(GENERIC, ast, "switch statement value must evaluate to an integer, are you missing a cast?");
                    }

                    st.values.add(constantExpressionToValue(ast, constExpr));
                    st.addresses.add(block.code.size()); // start Address for case statement
                } else {
                    if(st.defaultAddress != -1)  {
                        errors->createNewError(GENERIC, ast, "switch statement contains multiple cases with value `default`");
                    }
                    st.defaultAddress = block.code.size(); // start Address for case statement
                }

                for(long x = decl == "case" ? 1 : 0; x < ast->getSubAstCount(); x++) {
                    sub = ast->getSubAst(x);

                    if(sub->getType() == ast_block) {
                        parseBlock(sub, block);
                    } else {
                        parseStatement(block, sub->getSubAst(0));
                    }
                }

                if(!currentScope()->reachable && (currentScope()->last_statement==ast_return_stmnt
                                                  || currentScope()->last_statement == ast_throw_statement)) {
                    currentScope()->reachable=true;
                }
                break;
        }
    }

    if(st.defaultAddress == -1)  {
        st.defaultAddress = block.code.size(); // start Address for case statement
    }

    currentScope()->currentFunction->switchTable.push_back(st);
    currentScope()->label_map.add(KeyPair<std::string, int64_t>(switchEndLabel,__init_label_address(block.code)));
    currentScope()->brahchHelper.pop_back();
    currentScope()->switches--;
}

void RuntimeEngine::parseLockStatement(Block& block, Ast* pAst) {
    string lockBeginLabel, lockEndLabel;

    Expression cond = parseExpression(pAst->getSubAst(ast_expression)), out(pAst);

    stringstream ss;
    ss << generic_label_id << ++currentScope()->uniqueLabelSerial;
    lockBeginLabel=ss.str();

    ss.str("");
    ss << generic_label_id << ++currentScope()->uniqueLabelSerial;
    lockEndLabel=ss.str();

    currentScope()->label_map.add(KeyPair<std::string, int64_t>(lockBeginLabel,__init_label_address(block.code)));
    pushExpressionToPtr(cond, out, true);
    block.code.inject(block.code.size(), out.code);


    block.code.push_i64(SET_Ei(i64, op_LOCK));
    parseBlock(pAst->getSubAst(ast_block), block);

    block.code.inject(block.code.size(), out.code);
    block.code.push_i64(SET_Ei(i64, op_ULOCK));
    currentScope()->label_map.add(KeyPair<std::string, int64_t>(lockEndLabel,__init_label_address(block.code)));
}

void RuntimeEngine::parseDoWhileStatement(Block& block, Ast* pAst) {
    string whileBeginLabel, whileEndLabel;
    currentScope()->blocks++;
    currentScope()->loops++;
    currentScope()->uniqueLabelSerial++;
    currentScope()->brahchHelper.add(SCOPE_WHILE_LOOP);
    KeyPair<string, string> loopMap;

    stringstream ss;
    ss << while_label_begin_id << currentScope()->uniqueLabelSerial;
    whileBeginLabel=ss.str();
    currentScope()->label_map.add(KeyPair<std::string, int64_t>(whileBeginLabel,__init_label_address(block.code)));
    loopMap.key = whileBeginLabel;


    ss.str("");
    ss << while_label_end_id << currentScope()->uniqueLabelSerial;
    whileEndLabel=ss.str();
    loopMap.value = whileEndLabel;
    currentScope()->loopAddressTable.push_back(loopMap);
    parseBlock(pAst->getSubAst(ast_block), block);

    Expression cond = parseExpression(pAst->getSubAst(ast_expression)), out(pAst);
    pushExpressionToRegister(cond, out, i64cmt);
    block.code.inject(block.code.size(), out.code);


    currentScope()->addStore(whileBeginLabel, i64adx, 1, block.code,
                    pAst->getSubAst(ast_expression)->line, pAst->getSubAst(ast_expression)->col);
    block.code.push_i64(SET_Ei(i64, op_IFE));


    currentScope()->label_map.add(KeyPair<std::string, int64_t>(whileEndLabel,__init_label_address(block.code)));
    currentScope()->blocks--;
    currentScope()->loops--;
    currentScope()->brahchHelper.pop_back();
    currentScope()->loopAddressTable.pop_back();
}

ClassObject* RuntimeEngine::parseCatchClause(Block &block, Ast *pAst, ExceptionTable et) {
    ClassObject* klass = NULL;

    KeyPair<string, ResolvedReference> catcher = parseUtypeArg(pAst->getSubAst(ast_utype_arg_opt));

    string name =  catcher.key;
    KeyPair<int, Field>* field;
    List<AccessModifier> modCompat;
    modCompat.add(PUBLIC);

    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(pAst->line),
                                   pAst->line, pAst->col);
    Field f = Field(NULL, uniqueSerialId++, name, currentScope()->klass, modCompat, note);

    f.address = currentScope()->currentFunction->localVariables;
    f.local=true;
    currentScope()->currentFunction->localVariables++;
    if(catcher.value.type == CLASS) {
        f.klass = catcher.value.klass;
        f.type = CLASS;
    } else if(catcher.value.isVar() || catcher.value.dynamicObject()) {
        errors->createNewError(GENERIC, pAst, " field `" + catcher.value.field.name + "` is not a class");
        f.type = catcher.value.type;
    } else {
        f.type = UNDEFINED;
    }

    f.isArray = catcher.value.array;

    if(validateLocalField(name, pAst)) {
        if(catcher.value.type == CLASSFIELD) {
            errors->createNewError(COULD_NOT_RESOLVE, pAst, " `" + catcher.value.field.name + "`");
        }

        currentScope()->locals.add(KeyPair<int, Field>(currentScope()->blocks, f));
        field = currentScope()->getLocalField(name);
        et.local = f.address;
        et.className = f.klass == NULL ? "" : f.klass->getFullName();
        klass=f.klass;
        et.handler_pc = __init_label_address(block.code)+1;
        currentScope()->currentFunction->exceptions.push_back(et);
    }

    // TODO: add goto to finally block
    parseBlock(pAst->getSubAst(ast_block), block);
    return klass;
}

void RuntimeEngine::parseFinallyBlock(Block& block, Ast* pAst) {
    currentScope()->reachable=true;
    parseBlock(pAst->getSubAst(ast_block), block);
}

void RuntimeEngine::parseTryCatchStatement(Block& block, Ast* pAst) {
    ExceptionTable et;
    currentScope()->trys++;
    string catchEndLabel;
    List<ClassObject*> klasses;
    ClassObject* klass;

    et.start_pc = block.code.__asm64.size();
    parseBlock(pAst->getSubAst(ast_block), block);
    et.end_pc = block.code.__asm64.size();


    if(pAst->hasSubAst(ast_catch_clause))
        currentScope()->reachable=true;

    stringstream ss;
    ss << try_label_end_id << ++currentScope()->uniqueLabelSerial;
    catchEndLabel = ss.str();

    currentScope()->addBranch(catchEndLabel, 1, block.code, pAst->getSubAst(ast_block)->line, pAst->getSubAst(ast_block)->col);

    Ast* sub;
    for(unsigned int i = 1; i < pAst->getSubAstCount(); i++) {
        sub = pAst->getSubAst(i);

        switch(sub->getType()) {
            case ast_catch_clause:
                currentScope()->blocks++;
                klass = parseCatchClause(block, sub, et);
                currentScope()->addBranch(catchEndLabel, 1, block.code, sub->line, sub->col);

                if(klass != NULL) {
                    if(klasses.find(klass)) {
                        errors->createNewError(GENERIC, sub, "exception `" + klass->getName() + "` has already been caught");
                    } else
                        klasses.add(klass);
                }

                if(!currentScope()->reachable && (currentScope()->last_statement==ast_return_stmnt
                                                  || currentScope()->last_statement == ast_throw_statement)) {
                    currentScope()->reachable=true;
                }
                currentScope()->removeLocals(currentScope()->blocks);
                currentScope()->blocks--;
                break;
            case ast_finally_block:
                break;
        }
    }

    block.code.push_i64(SET_Ei(i64, op_NOP)); // for allignment
    block.code.push_i64(SET_Ei(i64, op_NOP));

    klasses.free();
    currentScope()->label_map.add(KeyPair<string,int64_t>(catchEndLabel, __init_label_address(block.code)));

    if(pAst->hasSubAst(ast_finally_block)) {
        FinallyTable ft;
        ft.try_start_pc=et.start_pc;
        ft.try_end_pc=et.end_pc;
        ft.start_pc=__init_label_address(block.code);
        parseFinallyBlock(block, pAst->getSubAst(ast_finally_block));
        ft.end_pc=__init_label_address(block.code);

        currentScope()->currentFunction->finallyBlocks.push_back(ft);

        if(!currentScope()->reachable && (currentScope()->last_statement==ast_return_stmnt
                                          || currentScope()->last_statement == ast_throw_statement)) {
            currentScope()->reachable=true;
        }
    }
    currentScope()->trys--;
}

void RuntimeEngine::parseThrowStatement(Block& block, Ast* pAst) {
    Expression clause = parseExpression(pAst->getSubAst(ast_expression)), out(pAst);
    currentScope()->reachable=false;
    currentScope()->last_statement=ast_throw_statement;

    if(clause.type == expression_lclass) {
        ClassObject* throwable = getClass("std", "Throwable", classes);

        if(throwable != NULL) {
            if(clause.utype.klass->hasBaseClass(throwable)) {
                pushExpressionToStack(clause, out);

                out.code.push_i64(SET_Ei(i64, op_THROW));
                block.code.inject(block.code.size(), out.code);
            } else {
                errors->createNewError(GENERIC, pAst->getSubAst(ast_expression), "class `" + clause.utype.klass->getFullName() +
                                                                           "` does not inherit `std#Throwable`");
            }
        } else {
            errors->createNewError(GENERIC, pAst->getSubAst(ast_expression), "missing core class `std#Throwable` for exception handling");
        }
    } else if(clause.type == expression_field) {
        if(clause.utype.field.type == CLASS) {
            pushExpressionToStack(clause, out);

            out.code.push_i64(SET_Ei(i64, op_THROW));
            block.code.inject(block.code.size(), out.code);
        } else {
            errors->createNewError(GENERIC, pAst->getSubAst(ast_expression), "field `" + clause.utype.field.name +
                                                                       "` is not a class");
        }
    } else
    {
        errors->createNewError(GENERIC, pAst->getSubAst(ast_expression), "expression must be of type lclass");
    }
}

// TODO: allow to be used with do while and while loops and foreach
// also make statements after this unreachable
void RuntimeEngine::parseContinueStatement(Block& block, Ast* pAst) {

    if(currentScope()->brahchHelper.size() > 0 && currentScope()->brahchHelper.last() == SCOPE_FOR_LOOP) {
        stringstream ss;
        ss << for_label_iter_id << currentScope()->loopAddressTable.size();
        string name = ss.str();
        currentScope()->addBranch(name, 1, block.code, pAst->line, pAst->col);
    } else if(currentScope()->brahchHelper.size() > 0 && currentScope()->brahchHelper.last() == SCOPE_WHILE_LOOP) {
        string name = currentScope()->loopAddressTable.last().key;
        currentScope()->addBranch(name, 1, block.code, pAst->line, pAst->col);
    } else {
        // error not in loop
        errors->createNewError(GENERIC, pAst, "continue statement outside of loop");
    }
}

void RuntimeEngine::parseBreakStatement(Block& block, Ast* pAst) {

    if(currentScope()->brahchHelper.size() > 0 && (currentScope()->brahchHelper.last() == SCOPE_FOR_LOOP ||
            currentScope()->brahchHelper.last() == SCOPE_WHILE_LOOP)) {
        string name = currentScope()->loopAddressTable.last().value;
        currentScope()->addBranch(name, 1, block.code, pAst->line, pAst->col);
    } else if(currentScope()->brahchHelper.size() > 0 && currentScope()->brahchHelper.last() == SCOPE_SWITCH) {
        stringstream name;
        name << switch_label_end_id << currentScope()->switches;
        currentScope()->addBranch(name.str(), 1, block.code, pAst->line, pAst->col);
    } else {
        // error not in loop
        errors->createNewError(GENERIC, pAst, "break statement outside of loop");
    }
}

void RuntimeEngine::parseGotoStatement(Block& block, Ast* pAst) {
    string label = pAst->getEntity(1).getToken();

    currentScope()->addBranch(label, 1, block.code, pAst->line, pAst->col);
}

bool RuntimeEngine::label_exists(string label) {
    for(unsigned int i = 0; i < currentScope()->label_map.size(); i++) {
        if(currentScope()->label_map.get(i).key == label)
            return true;
    }

    return false;
}

void RuntimeEngine::createLabel(string name, Assembler& code, int line, int col) {

    if(label_exists(name)) {
        errors->createNewError(GENERIC, line, col, "redefinition of label `" + name + "`");
    } else {
        currentScope()->label_map.add(KeyPair<string, int64_t>(name, __init_label_address(code)));
    }
}

void RuntimeEngine::parseLabelDecl(Block& block, Ast* pAst) {
    string label = pAst->getEntity(0).getToken();

    createLabel(label, block.code, pAst->getEntity(0).getLine(), pAst->getEntity(0).getColumn());
    parseStatement(block, pAst->getSubAst(ast_statement)->getSubAst(0));
}

void RuntimeEngine::parseVarDecl(Block& block, Ast* pAst) {
    List<AccessModifier> modifiers;
    int startpos=0, index = 1;

    parseAccessDecl(pAst, modifiers, startpos);

    // we need this before for multiple definition of same type
    Expression utype = parseUtype(pAst);
    parse_var:
    token_entity operand = pAst->getEntity(pAst->getEntityCount()-1);
    string name =  pAst->getEntity(pAst->getEntityCount()-(pAst->hasSubAst(ast_value) ? 2 : 1)).getToken();

    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(pAst->line),
                                   pAst->line, pAst->col);
    Field f = Field(NULL, uniqueSerialId++, name, currentScope()->klass, modifiers, note);

    f.address = currentScope()->currentFunction->localVariables++;
    f.local=true;
    f.type = utype.utype.type;
    f.owner = currentScope()->klass;
    if(utype.utype.type == CLASS) {
        f.klass = utype.utype.klass;
    } else if(utype.utype.type == TYPEVOID) {
        f.key = utype.utype.referenceName;
    }


    f.isArray = utype.utype.array;

    if(validateLocalField(name, pAst)) {
        if(utype.utype.type == CLASSFIELD) {
            errors->createNewError(COULD_NOT_RESOLVE, pAst, " `" + f.name + "`");
        }

        currentScope()->locals.add(KeyPair<int, Field>(currentScope()->blocks, f));
        Expression fieldExpr = fieldToExpression(pAst, f);

//        if(f.isObjectInMemory())
//            block.code.__asm64.push_back(SET_Di(i64, op_MOVL, f.address));

        if(pAst->hasSubAst(ast_value)) {
            Expression expression = parseValue(pAst->getSubAst(ast_value)), out(pAst);
            equals(fieldExpr, expression);

            if(f.isConst()) {
                if(expression.literal == true || expression.isConstExpr()
                   || expression.isEnum() || f.type == CLASS) {
                    /* good to go */
                    Field *local = &currentScope()->getLocalField(f.name)->value;
                    local->constant_value = constantExpressionToValue(pAst, expression);
                } else {
                    errors->createNewError(GENERIC, pAst, "constant field cannot be assigned to non-constant expression of type `" + expression.typeToString() + "`");
                }
            }

            if(f.isObjectInMemory()) {
                if(operand == "=") {

                    if(f.type==CLASS && f.klass->getModuleName() == "std" && f.klass->getName() == "string"
                        && expression.type == expression_string && !f.isArray) {
                        constructNewNativeClass("string", "std", expression, out, true);
                        out.code.push_i64(SET_Di(i64, op_POPL, f.address));
                    } else if(f.type==CLASS && f.klass->getModuleName() == "std" &&
                              (f.klass->getName() == "int" || f.klass->getName() == "bool"
                              || f.klass->getName() == "char" || f.klass->getName() == "long"
                              || f.klass->getName() == "short" || f.klass->getName() == "string"
                              || f.klass->getName() == "uchar" || f.klass->getName() == "ulong"
                              || f.klass->getName() == "ushort")
                         && expression.type == expression_var && !f.isArray) {
                        constructNewNativeClass(f.klass->getName(), "std", expression, out, true);
                        out.code.push_i64(SET_Di(i64, op_POPL, f.address));
                    } else
                        assignValue(operand, out, fieldExpr, expression, pAst);
                    block.code.inject(block.code.__asm64.size(), out.code);
                } else {
                    errors->createNewError(GENERIC, pAst, " explicit call to operator `" + operand.getToken() + "` without initilization");
                }
            } else {
                if(operand != "=") {
                    block.code.push_i64(SET_Di(i64, op_ISTOREL, f.address), 0);
                }

                assignValue(operand, out, fieldExpr, expression, pAst);
                block.code.inject(block.code.__asm64.size(), out.code);
            }
        } else {
            if(f.isConst()) {
                errors->createNewError(GENERIC, pAst, "constant field requires a value to be provided");
            }

            if(!f.isObjectInMemory()) {
                block.code.push_i64(SET_Di(i64, op_ISTOREL, f.address), 0);
            } else {
                block.code.__asm64.push_back(SET_Di(i64, op_MOVL, f.address));
                block.code.push_i64(SET_Ei(i64, op_DEL));
            }
        }
    }

    if(pAst->hasSubAst(ast_var_decl)) {
        pAst = pAst->getSubAst(ast_var_decl);
        goto parse_var;
    }

}

void RuntimeEngine::parseStatement(Block& block, Ast* pAst) {
    addLine(block, pAst);

    if(!currentScope()->reachable) {
        errors->createNewError(GENERIC, pAst, "unreachable statement");
        currentScope()->reachable=true;
    }else {
        currentScope()->last_statement=pAst->getType();
    }

    switch(pAst->getType()) {
        case ast_return_stmnt:
            parseReturnStatement(block, pAst);
            break;
        case ast_if_statement:
            parseIfStatement(block, pAst); // done
            break;
        case ast_expression: {
            Expression expr(pAst);
            expr = parseExpression(pAst);
            if(expr.func && expr.type != expression_void ||
                expr.newExpression) {
                expr.code.push_i64(SET_Ei(i64, op_POP));
            }

            block.code.inject(block.code.size(), expr.code); // done
        }
            break;
        case ast_assembly_statement:
            parseAssemblyStatement(block, pAst); // done
            break;
        case ast_for_statement:
            parseForStatement(block, pAst); // done
            break;
        case ast_foreach_statement:
            parseForEachStatement(block, pAst); // done
            break;
        case ast_while_statement:
            parseWhileStatement(block, pAst); // done
            break;
        case ast_switch_statement:
            parseSwitchStatement(block, pAst); // done
            break;
        case ast_lock_statement:
            parseLockStatement(block, pAst); // done
            break;
        case ast_do_while_statement:
            parseDoWhileStatement(block, pAst); // done
            break;
        case ast_trycatch_statement:
            parseTryCatchStatement(block, pAst); // done
            break;
        case ast_throw_statement:
            parseThrowStatement(block, pAst); // done
            break;
        case ast_continue_statement:
            parseContinueStatement(block, pAst); // done
            break;
        case ast_break_statement:
            parseBreakStatement(block, pAst); // done
            break;
        case ast_goto_statement:
            parseGotoStatement(block, pAst); // done
            break;
        case ast_label_decl:
            parseLabelDecl(block, pAst); // done
            break;
        case ast_var_decl:
            parseVarDecl(block, pAst); // done
            break;
        default: {
            stringstream err;
            err << ": unknown ast type: " << pAst->getType();
            errors->createNewError(INTERNAL_ERROR, pAst->line, pAst->col, err.str());
            break;
        }
    }
}

void RuntimeEngine::parseBlock(Ast* pAst, Block& block) {
    currentScope()->blocks++;
    currentBlock = &block;

    Ast* ast;
    for(unsigned int i = 0; i < pAst->getSubAstCount(); i++) {
        ast = pAst->getSubAst(i);

        if(ast->getType() == ast_block) {
            parseBlock(ast, block);
            continue;
        } else if(ast->getSubAstCount() > 0) {
            ast = ast->getSubAst(0);
            parseStatement(block, ast);
        }
    }

    currentScope()->removeLocals(currentScope()->blocks);

    currentScope()->blocks--;
}

void RuntimeEngine::resolveAllBranches(Block& block) {

    int64_t address, i64;
    BranchTable* bt;
    for(unsigned int i = 0; i < currentScope()->branches.size(); i++)
    {
        bt = &currentScope()->branches.get(i);

        if((address = currentScope()->getLabel(bt->labelName)) != -1) {

            if(bt->store) {
                currentScope()->currentFunction->unique_address_table.add(bt->branch_pc); // add indirect address store for optimizer

                block.code.__asm64.replace(bt->branch_pc, SET_Di(i64, op_MOVI, (bt->_offset+address)));
                block.code.__asm64.replace(bt->branch_pc+1, bt->registerWatchdog);
            } else {
                block.code.__asm64.replace(bt->branch_pc, SET_Di(i64, op_GOTO, (bt->_offset+address)));
            }
        } else
            errors->createNewError(COULD_NOT_RESOLVE, bt->line, bt->col, " `" + bt->labelName + "`");
    }


    freeList(currentScope()->branches);
}

void RuntimeEngine::reorderFinallyBlocks(Method* method) {

    if(method->finallyBlocks.size()==0)
        return;

    std::list<FinallyTable> reorderedList;
    for(unsigned int i = 0; i < method->finallyBlocks.size(); i++) {
        reorderedList.push_back(method->finallyBlocks.at(i));
    }
    reorderedList.sort([](const FinallyTable & a, const FinallyTable & b) { return a.start_pc < b.start_pc; });

    method->finallyBlocks.addAll(reorderedList);
    reorderedList.clear();
}

void RuntimeEngine::parseMethodDecl(Ast* pAst, bool delegate) {
    List<AccessModifier> modifiers;
    int startpos=1;

    parseAccessDecl(pAst, modifiers, startpos);

    List<Param> params;
    string name =  pAst->getEntity(startpos+(delegate ? 3 : 0)).getToken();
    parseMethodParams(params, parseUtypeArgList(pAst->getSubAst(ast_utype_arg_list)), pAst->getSubAst(ast_utype_arg_list));

    Method* method = currentScope()->klass->getFunction(name, params, false, false, false, true);

    if(method != NULL) {

        if(method->isStatic()) {
            addScope(Scope(STATIC_BLOCK, currentScope()->klass, method));
        } else {
            addScope(Scope(INSTANCE_BLOCK, currentScope()->klass, method));
            method->localVariables++; // for reference to self
        }

        KeyPair<int, Field> local;
        for(unsigned int i = 0; i < params.size(); i++) {

            params.get(i).field.address=method->localVariables++;
            params.get(i).field.local=true;
            local.set(currentScope()->blocks, params.get(i).field);
            currentScope()->locals.add(local);
        }

        Block fblock;
        parseBlock(pAst->getSubAst(ast_block), fblock);

        resolveAllBranches(fblock);
        reorderFinallyBlocks(method);
        method->code.__asm64.appendAll(fblock.code.__asm64);
        method->assembly_table.addAll(fblock.assembly_table);
        removeScope();
    }
}

void RuntimeEngine::parseOperatorDecl(Ast* pAst) {
    List<AccessModifier> modifiers;
    int startpos=1;

    parseAccessDecl(pAst, modifiers, startpos);

    List<Param> params;
    string name =  pAst->getEntity(pAst->getEntityCount()-1).getToken();
    parseMethodParams(params, parseUtypeArgList(pAst->getSubAst(ast_utype_arg_list)), pAst->getSubAst(ast_utype_arg_list));

    Method* method = currentScope()->klass->getOverload(stringToOp(name), params, false, false, true);

    if(method != NULL) {

        if(method->isStatic()) {
            addScope(Scope(STATIC_BLOCK, currentScope()->klass, method));
            errors->createNewError(GENERIC, pAst, "operator function `" + method->getName() + "` cannot be static");
        } else {
            addScope(Scope(INSTANCE_BLOCK, currentScope()->klass, method));
            method->localVariables++;
        }

        if(!method->hasModifier(PUBLIC)) {
            createNewWarning(GENERIC, pAst->line, pAst->col, "operator function `" + method->getName() + "` cannot be static");
        }

        KeyPair<int, Field> local;
        for(unsigned int i = 0; i < params.size(); i++) {

            params.get(i).field.address=method->localVariables++;
            params.get(i).field.local=true;
            local.set(currentScope()->blocks, params.get(i).field);
            currentScope()->locals.add(local);
        }

        Block fblock;
        parseBlock(pAst->getSubAst(ast_block), fblock);

        resolveAllBranches(fblock);
        reorderFinallyBlocks(method);
        method->code.__asm64.addAll(fblock.code.__asm64);
        removeScope();
    }
}

void RuntimeEngine::parseConstructorDecl(Ast* pAst) {
    List<AccessModifier> modifiers;
    int startpos=1;

    parseAccessDecl(pAst, modifiers, startpos);

    List<Param> params;
    parseMethodParams(params, parseUtypeArgList(pAst->getSubAst(ast_utype_arg_list)), pAst->getSubAst(ast_utype_arg_list));

    Method* method = currentScope()->klass->getConstructor(params, false, false, true);

    if(method != NULL) {
        addScope(Scope(INSTANCE_BLOCK, currentScope()->klass, method));
        method->localVariables++;

        KeyPair<int, Field> local;
        for(unsigned int i = 0; i < params.size(); i++) {

            params.get(i).field.address=method->localVariables++;
            params.get(i).field.local=true;

            local.set(currentScope()->blocks, params.get(i).field);
            currentScope()->locals.add(local);
        }

        Block fblock;

        parseBlock(pAst->getSubAst(ast_block), fblock);

        resolveAllBranches(fblock);
        reorderFinallyBlocks(method);
        method->code.__asm64.appendAll(fblock.code.__asm64);
        removeScope();
    }
}

void RuntimeEngine::analyzeClassDecl(Ast *ast) {
    Ast* block = ast->getSubAst(ast_block);
    List<AccessModifier> modifiers;
    ClassObject* klass;
    int startpos=1;

    parseAccessDecl(ast, modifiers, startpos);
    string className =  ast->getEntity(startpos).getToken();

    if(currentScope()->type == GLOBAL_SCOPE) {
        klass = getClass(currentModule, className, classes);

        setHeadClass(klass);
    }
    else {
        klass = currentScope()->klass->getChildClass(className);

        klass->setSuperClass(currentScope()->klass);
        setHeadClass(klass);
    }

    addScope(Scope(CLASS_SCOPE, klass));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        ast = block->getSubAst(i);
        CHECK_ERRORS

        switch(ast->getType()) {
            case ast_class_decl:
                analyzeClassDecl(ast);
                break;
            case ast_var_decl:
                analyzeVarDecl(ast);
                break;
            case ast_method_decl:
                parseMethodDecl(ast);
                break;
            case ast_operator_decl:
                parseOperatorDecl(ast);
                break;
            case ast_construct_decl:
                parseConstructorDecl(ast);
                break;
            case ast_delegate_post_decl: /* ignore */
                break;
            case ast_delegate_decl:
                parseMethodDecl(ast, true);
                break;
            case ast_generic_class_decl: /* ignore */
                break;
            case ast_enum_decl:
                break;
            case ast_func_prototype:
                break;
            default: {
                stringstream err;
                err << ": unknown ast type: " << ast->getType();
                errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
                break;
            }
        }
    }

    removeScope();
}

bool RuntimeEngine::equals(Expression& left, Expression& right, string msg) {

    if(left.type == expression_native) {
        errors->createNewError(UNEXPECTED_SYMBOL, left.link->line, left.link->col, " `" + left.typeToString() + "`" + msg);
        return false;
    }
    if(right.type == expression_native) {
        errors->createNewError(UNEXPECTED_SYMBOL, right.link->line, right.link->col, " `" + right.typeToString() + "`" + msg);
        return false;
    }

    switch(left.type) {
        case expression_var:
            if(right.trueType() == VAR) {
                // add 2 vars
                if(left.isArray() == right.isArray())
                    return true;
            }
            else if(right.type == expression_null) {
                return left.isArray();
            }
            break;
        case expression_null:
            if(right.trueType() == OBJECT || (right.trueType() == CLASS && right.type == expression_lclass)) {
                return true;
            } else if(right.type == expression_class) {
                errors->createNewError(GENERIC, right.link->line,  right.link->col, "Class `" + right.typeToString() + "` must be lvalue" + msg);
                return false;
            }
            break;
        case expression_field:
            if(left.trueType() == VAR) {
                // add var
                if(right.trueType() == OBJECT || right.trueType() == CLASS) {
                    if(left.trueType() == OBJECT) {
                        return true;
                    } else if(isNativeIntegerClass(right.getClass())) {
                        if(left.trueType() == VAR)
                            return true;
                    }
                } else if(right.type == expression_string || right.type == expression_null) {
                    return left.isArray();
                } else if(right.trueType() == VAR || (right.trueType() != CLASS)) {
                    if(left.trueType() == VAR) {
                        if(left.isArray() == right.isArray())
                            return true;
                    }
                }
            } else if(left.trueType() == CLASS) {
                if(right.type == expression_lclass) {
                    if(left.getClass()->assignable(right.getClass())) {
                        return true;
                    }
                } else if(right.type == expression_class) {
                    if(left.getClass()->assignable(right.getClass())) {
                        errors->createNewError(GENERIC, right.link->line,  right.link->col, "Class `" + right.typeToString() + "` must be lvalue" + msg);
                        return false;
                    }
                } else if(right.type == expression_null) {
                    return true;
                } else if(right.type == expression_field && right.utype.field.type == CLASS) {
                    if(left.getClass()->assignable(right.getClass())) {
                        return true;
                    }
                } else {
                    List<Param> params;
                    List<Expression> exprs;
                    exprs.push_back(right);

                    expressionListToParams(params, exprs);
                    if(left.utype.field.klass->getOverload(oper_EQUALS, params) != NULL)
                        return true;
                }
            }  else if(left.trueType() == OBJECT) {
                if(right.trueType() == OBJECT || right.trueType() == CLASS) {
                    return true;
                } else if(right.trueType() == VAR) {
                    if(!left.isArray() && right.isArray())
                        return true;
                }
            } else {
                // do nothing field unresolved
            }
            break;
        case expression_lclass:
            if(right.type == expression_lclass) {
                if(left.getClass()->assignable(right.getClass())) {
                    return true;
                }
            } else if(right.type == expression_field) {
                if(left.getClass()->assignable(right.getClass())) {
                    return true;
                }
            } else if(right.type == expression_class) {
                if(left.getClass()->assignable(right.getClass())) {
                    errors->createNewError(GENERIC, right.link->line,  right.link->col, "Class `" + right.typeToString() + "` must be lvalue" + msg);
                    return false;
                }
            } else if(right.type == expression_null) {
                return true;
            }
            break;
        case expression_class:
            if(right.type == expression_class) {
                if(left.getClass()->assignable(right.getClass())) {
                    return true;
                }
            }
            break;
        case expression_void:
            if(right.type == expression_void) {
                return true;
            }
            break;
        case expression_objectclass:
            if(right.trueType() == OBJECT || right.trueType() == CLASS || right.trueType() == VAR) {
                if(right.trueType() == VAR)
                    return !left.isArray() && right.isArray();
                return true;
            } else if(right.type == expression_null) {
                return true;
            }
            break;
        case expression_string:
            if(right.trueType() == VAR) {
                if(right.isArray()) {
                    return true;
                }
            }
            else if(right.type == expression_string) {
                return true;
            }
            break;
        default:
            break;
    }

    if(left.trueType() == CLASS && right.trueType() == CLASS) {
        errors->createNewError(GENERIC, right.link->line,  right.link->col, "classes in expression are not compatible, are you possibly missing a cast?");
    } else
        errors->createNewError(GENERIC, right.link->line,  right.link->col, "Expressions of type `" + left.typeToString() + "` and `" + right.typeToString() + "` are not compatible" + msg);
    return false;
}

void RuntimeEngine::addNative(token_entity operand, FieldType type, Expression& out, Expression& left, Expression& right, Ast* pAst) {
    out.type = expression_var;
    right.literal = false;
    Expression expr(pAst);

    if(left.type == expression_var) {
        equals(left, right);

        pushExpressionToStack(right, out);
        pushExpressionToRegister(left, out, i64ebx);
        out.code.push_i64(SET_Di(i64, op_LOADVAL, i64egx));
        out.code.push_i64(SET_Ci(i64, operandToOp(operand), i64ebx,0, i64egx), i64ebx);
        right.code.free();
    } else if(left.type == expression_field) {
        if(left.utype.field.isVar()) {
            equals(left, right);

            pushExpressionToStack(right, out);
            pushExpressionToRegister(left, out, i64ebx);
            out.code.push_i64(SET_Di(i64, op_LOADVAL, i64egx));
            out.code.push_i64(SET_Ci(i64, operandToOp(operand), i64ebx,0, i64egx), i64ebx);
            right.code.free();
        }
        else {
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
        }
    } else {
        errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
    }
}

bool RuntimeEngine::hasOverload(token_entity operand, Expression &right, ClassObject* klass, Ast* pAst) {
    List<Param> params;
    List<Expression> eList;
    eList.push_back(right);
    expressionListToParams(params, eList);
    return klass->getOverload(stringToOp(operand.getToken()), params, true, true) != NULL;
}

void RuntimeEngine::addClass(token_entity operand, ClassObject* klass, Expression& out, Expression& left, Expression &right, Ast* pAst) {
    List<Param> params;
    List<Expression> eList;
    OperatorOverload* overload;
    right.literal = false;

    eList.push_back(right);
    expressionListToParams(params, eList);
    if(right.charLiteral && klass->getName() == "string" && klass->getModuleName() == "std") {
        ClassObject *charClass = getClass("std", "char", classes);
        Method *constr;

        if(charClass != NULL) {
            constr = charClass->getConstructor(params, true);

            if(constr != NULL) {
                freeList(eList);
                freeList(params);

                right.type = expression_lclass;
                right.utype.klass = charClass;
                right.utype.type = CLASS;
                right.code.__asm64.insert(0, SET_Di(i64, op_NEWCLASS, charClass->address));
                right.code.push_i64(SET_Di(i64, op_RSTORE, i64ebx));
                right.code.push_i64(SET_Di(i64, op_CALL, constr->address));
                right.func = true;

                eList.push_back(right);
                expressionListToParams(params, eList);
            }
        }
    }


    if((overload = klass->getOverload(stringToOp(operand.getToken()), params, true)) != NULL) {
        // call operand
        if(!overload->isStatic())
            pushExpressionToStack(left, out);

        Expression exp = fieldToExpression(NULL, overload->getParam(0).field);
        equals(exp, eList.last());
        pushExpressionToStack(right, out);

        verifyMethodAccess(overload, pAst);
        out.type = methodReturntypeToExpressionType(overload);
        right.free();
        right.type=out.type;
        right.func=true;
        if(out.type == expression_lclass) {
            out.utype.klass = overload->klass;
            out.utype.type = CLASS;
            right.utype.klass = overload->klass;
            right.utype.type = CLASS;
        }

        out.func=true;
        out.code.push_i64(SET_Di(i64, op_CALL, overload->address));
    } else {
        errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `$operator" + operand.getToken() + "` was not found and cannot be applied to expression of type `"
                                                          + left.typeToString() + "` and `" + right.typeToString() + "`");
    }

    freeList(params);
    freeList(eList);
}

void RuntimeEngine::addClassChain(token_entity operand, ClassObject* klass, Expression& out, Expression& left, Expression &right, Ast* pAst) {
    List<Param> params;
    List<Expression> eList;
    eList.push_back(right);
    OperatorOverload* overload;
    left.literal = false;

    expressionListToParams(params, eList);

    if((overload = klass->getOverload(stringToOp(operand.getToken()), params, true)) != NULL) {
        // call operand
        if(overload->isStatic()) {
            errors->createNewError(GENERIC, pAst, "call to function `$operator" + operand.getToken() + "` is static");
        }


        if(left.func && left.type != expression_void) {
//            out.code.push_i64(SET_Di(i64, op_MOVSL, 0));
//            out.code.push_i64(SET_Di(i64, op_DEC, sp));
//
//            out.code.push_i64(SET_Di(i64, op_PUSHOBJ, 0));
        } else if(left.func) {
            errors->createNewError(GENERIC, pAst, "call to function `$operator" + operand.getToken() + "` returns void from previous operand");
        } else {
            out.code.push_i64(SET_Ei(i64, op_PUSHOBJ));
        }

        Expression exp = fieldToExpression(NULL, overload->getParam(0).field);
        equals(exp, eList.last());
        verifyMethodAccess(overload, pAst);
        pushExpressionToStack(right, out);

        out.type = methodReturntypeToExpressionType(overload);
        left.free();
        left.type=out.type;
        left.func=true;
        if(out.type == expression_lclass) {
            out.utype.klass = overload->klass;
            out.utype.type = CLASS;
            left.utype.klass = overload->klass;
            left.utype.type = CLASS;
        }

        out.func=true;
        out.code.push_i64(SET_Di(i64, op_CALL, overload->address));
    } else {
        errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() + "` cannot be applied to expression of type `"
                                                          + left.typeToString() + "` and `" + right.typeToString() + "`");
    }

    freeList(params);
    freeList(eList);
}

void RuntimeEngine::addStringConstruct(token_entity operand, ClassObject* klass, Expression& out, Expression& left, Expression &right, Ast* pAst) {
    List<Param> params;
    List<Expression> eList;
    eList.push_back(right);
    OperatorOverload* overload;
    right.literal = false;
    Expression newOut(pAst);

    expressionListToParams(params, eList);

    if((overload = klass->getOverload(stringToOp(operand.getToken()), params)) != NULL) {
        // call operand

        newOut.inject(out);
        pushExpressionToStack(right, newOut);

        Expression exp = fieldToExpression(NULL, overload->getParam(0).field);
        equals(exp, eList.last());

        verifyMethodAccess(overload, pAst);
        out.type = methodReturntypeToExpressionType(overload);
        left.free();
        left.type=out.type;
        left.func=true;
        if(out.type == expression_lclass) {
            out.utype.klass = overload->klass;
            out.utype.type = CLASS;
            left.utype.klass = overload->klass;
            left.utype.type = CLASS;
        }

        out.func=true;
        newOut.code.push_i64(SET_Di(i64, op_CALL, overload->address));

        out.code.free();
        out.inject(newOut);
    } else {
        errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() + "` cannot be applied to expression of type `"
                                                          + left.typeToString() + "` and `" + right.typeToString() + "`");
    }

    freeList(params);
    freeList(eList);
}

void RuntimeEngine::constructNewNativeClass(string k, string module, Expression &expr, Expression& out, bool update) {
    List<Expression> expressions;
    List<Param> params;
    ClassObject* klass;
    Method* fn=NULL;

    expressions.add(expr);
    expressionListToParams(params, expressions);

    if((klass = getClass(module, k, classes)) != NULL) {
        if((fn=klass->getConstructor(params, true)) != NULL) {
            if(update) {
                out.type = expression_lclass;
                out.utype.klass = klass;
                out.utype.type=CLASS;
            }

            verifyMethodAccess(fn, out.link);
            out.code.push_i64(SET_Di(i64, op_NEWCLASS, klass->address));

            for(unsigned int i = 0; i < expressions.size(); i++) {
                Expression exp = fieldToExpression(NULL, fn->getParam(i).field);
                equals(exp, expressions.get(i));

                if(fn->getParam(i).field.dynamicObject() && expressions.get(i).type == expression_var) {
                    pushExpressionToRegister(expressions.get(i), out, i64ebx);

                    out.code.push_i64(SET_Di(i64, op_MOVI, 1), i64egx);
                    out.code.push_i64(SET_Di(i64, op_MOVI, 0), i64adx);
                    out.code.push_i64(SET_Di(i64, op_NEWARRAY, i64egx));
                    out.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                    out.code.push_i64(SET_Ci(i64, op_RMOV, i64adx, 0, i64ebx));
                } else
                    pushExpressionToStack(expressions.get(i), out);
            }
            out.code.push_i64(SET_Di(i64, op_CALL, fn->address));
        } else {
            if(update)
                out = expr;
            errors->createNewError(GENERIC, expr.link->line,  expr.link->col, "Support class `" + k + "` does not have constructor for type `"
                                                                    + expr.typeToString() + "`");
        }
    } else {
        if(update)
            out = expr;

        errors->createNewError(GENERIC, expr.link->line,  expr.link->col, "Support class `" + k + "` does not have constructor for type `"
                                                                          + expr.typeToString() + "`");
    }

    freeList(params);
    freeList(expressions);
}

bool RuntimeEngine::constructNewString(Expression &stringExpr, Expression &right, token_entity operand, Expression& out, Ast* pAst) {
    List<Expression> expressions;
    List<Param> params;
    ClassObject* klass;
    Method* fn=NULL;

    expressions.add(stringExpr);
    expressionListToParams(params, expressions);

    if((klass = getClass("std", "string", classes)) != NULL) {
        if((fn=klass->getConstructor(params)) != NULL) {
            stringExpr.type = expression_lclass;
            stringExpr.utype.klass = klass;
            stringExpr.utype.type=CLASS;
            out.type = expression_lclass;
            out.utype.klass = klass;
            out.utype.type=CLASS;

            verifyMethodAccess(fn, pAst);
            out.code.push_i64(SET_Di(i64, op_NEWCLASS, klass->address));

            for(unsigned int i = 0; i < expressions.size(); i++) {
                Expression exp = fieldToExpression(NULL, fn->getParam(i).field);
                equals(exp, expressions.get(i));

                if(fn->getParam(i).field.dynamicObject() && expressions.get(i).type == expression_var) {
                    pushExpressionToRegister(expressions.get(i), out, i64ebx);

                    out.code.push_i64(SET_Di(i64, op_MOVI, 1), i64egx);
                    out.code.push_i64(SET_Di(i64, op_MOVI, 0), i64adx);
                    out.code.push_i64(SET_Di(i64, op_NEWARRAY, i64egx));
                    out.code.push_i64(SET_Di(i64, op_MOVSL, 0));
                    out.code.push_i64(SET_Ci(i64, op_RMOV, i64adx, 0, i64ebx));
                } else
                    pushExpressionToStack(expressions.get(i), out);
            }
            out.code.push_i64(SET_Di(i64, op_CALL, fn->address));

            addStringConstruct(operand, klass, out, stringExpr, right, pAst);

            out.func=true;
            freeList(params);
            freeList(expressions);
            return true;
        } else {
            out = stringExpr;
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `$operator" + operand.getToken() + "` was not found and cannot be applied to expression of type `"
                                                                    + stringExpr.typeToString() + "` and `" + right.typeToString() + "`");
        }
    } else {
        out = stringExpr;
        errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `$operator" + operand.getToken() + "` was not found and cannot be applied to expression of type `"
                                                                + stringExpr.typeToString() + "` and `" + right.typeToString() + "`");
    }

    freeList(params);
    freeList(expressions);
    return false;
}

bool RuntimeEngine::isMathOp(token_entity entity)
{
    return entity == "+" || entity == "-"
           || entity == "*" || entity == "/"
           || entity == "%";
}

bool RuntimeEngine::isAndOp(token_entity entity)
{
    return entity == "&&" || entity == "||"
           || entity == "|" || entity == "&"
           || entity == "^";
}

void RuntimeEngine::parseAddExpressionChain(Expression &out, Ast *pAst) {
    Expression leftExpr(pAst), rightExpr(pAst), peekExpr(pAst);
    int operandPtr = 0;
    double value=0;
    bool firstEval=true;
    token_entity operand;
    List<token_entity> operands;
    for(unsigned int i = 0; i < pAst->getEntityCount(); i++) {
        if(isMathOp(pAst->getEntity(i)))
            operands.add(pAst->getEntity(i));
    }

    operandPtr=0;
    for(long int i = 0; i < pAst->getSubAstCount(); i++) {
        if(firstEval) {
            leftExpr = pAst->getSubAst(i)->getType() == ast_expression ? parseExpression(pAst->getSubAst(i))
                                                                       : parseIntermExpression(pAst->getSubAst(i));
            i++;
            firstEval= false;
        }
        rightExpr = pAst->getSubAst(i)->getType() == ast_expression ? parseExpression(pAst->getSubAst(i)) :
                    parseIntermExpression(pAst->getSubAst(i));
        operand = operands.get(operandPtr++);
        double var = 0;


        switch(leftExpr.type) {
            case expression_var:
                if(rightExpr.type == expression_var) {
                    if(leftExpr.literal && rightExpr.literal) {

                        if(!addExpressions(out, leftExpr, rightExpr, operand, &var)) {
                            goto calculate;
                        }
                    } else {
                        calculate:
                        // is left leftexpr a literal?
                        pushExpressionToStack(rightExpr, out);
                        pushExpressionToRegister(leftExpr, out, i64ebx);
                        out.code.push_i64(SET_Di(i64, op_LOADVAL, i64ecx));
                        out.code.push_i64(SET_Ci(i64, operandToOp(operand), i64ebx,0, i64ecx), i64ebx);
                        out.code.push_i64(SET_Di(i64, op_RSTORE, i64ebx));

                        leftExpr.type=expression_var;
                        out.func = true;
                        leftExpr.func=true;
                        leftExpr.literal = false;
                        leftExpr.code.free();
                        var=0;
                    }

                    out.type = expression_var;
                }else if(rightExpr.type == expression_field) {
                    if(rightExpr.trueType() == VAR) {
                        // add var
                        leftEval:
                        if(leftExpr.literal && (i+1) < pAst->getSubAstCount()) {
                            peekExpr = pAst->getSubAst(i+1)->getType() == ast_expression ?
                                       parseExpression(pAst->getSubAst(i+1)) : parseIntermExpression(pAst->getSubAst(i+1));
                            Expression outtmp(pAst);

                            if(peekExpr.literal) {
                                if(addExpressions(outtmp, peekExpr, leftExpr, operand, &var)) {
                                    i++;
                                    leftExpr=outtmp;
                                    leftExpr.type=expression_var;
                                    goto leftEval;
                                }
                            }
                        }

                        addNative(operand, rightExpr.utype.field.type, out, leftExpr, rightExpr, pAst);
                        out.code.push_i64(SET_Di(i64, op_RSTORE, i64ebx));

                        leftExpr.type=expression_var;
                        out.func = true;
                        leftExpr.func=true;
                        leftExpr.literal = false;
                        leftExpr.code.free();
                    } else if(rightExpr.trueType() == CLASS) {
                        errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                          "` cannot be applied to expression of type `" + leftExpr.typeToString() + "` and `" + rightExpr.typeToString() + "`");
                    } else {
                        // do nothing field unresolved
                    }
                } else {
                    errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                      "` cannot be applied to expression of type `" + leftExpr.typeToString() + "` and `" + rightExpr.typeToString() + "`");
                }
                break;
            case expression_null:
                errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                  "` cannot be applied to expression of type `" + leftExpr.typeToString() + "` and `" + rightExpr.typeToString() + "`");
                break;
            case expression_field:
                if(leftExpr.trueType() == VAR) {
                    // add var
                    addNative(operand, leftExpr.utype.field.type, out, leftExpr, rightExpr, pAst);
                    out.code.push_i64(SET_Di(i64, op_RSTORE, i64ebx));

                    leftExpr.type=expression_var;

                    out.func = true;
                    leftExpr.func=true;
                    leftExpr.literal = false;
                    leftExpr.code.free();
                } else if(leftExpr.trueType() == CLASS) {
                    if(i <= 1) {
                        addClass(operand, leftExpr.utype.field.klass, out, leftExpr, rightExpr, pAst);
                        leftExpr=rightExpr;
                    }
                    else {
                        addClassChain(operand, leftExpr.utype.field.klass, out, leftExpr, rightExpr, pAst);
                    }
                } else {
                    // do nothing field unresolved
                }
                break;
            case expression_native:
                errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + leftExpr.typeToString() + "`");
                break;
            case expression_lclass:
                if(i <= 1) {
                    addClass(operand, leftExpr.utype.klass, out, leftExpr, rightExpr, pAst);
                    leftExpr=rightExpr;
                }
                else {
                    addClassChain(operand, leftExpr.utype.klass, out, leftExpr, rightExpr, pAst);
                }
                break;
            case expression_class:
                errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + leftExpr.typeToString() + "`");
                break;
            case expression_void:
                errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                  "` cannot be applied to expression of type `" + leftExpr.typeToString() + "` and `" + rightExpr.typeToString() + "`");
                break;
            case expression_objectclass:
                errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                  "` cannot be applied to expression of type `" + leftExpr.typeToString() + "` and `" + rightExpr.typeToString() + "`. Did you forget to apply a cast? "
                                                                          "i.e ((SomeClass)dynamic_class) " + operand.getToken() + " <data>");
                break;
            case expression_string:

                constructNewString(leftExpr, rightExpr, operand, out, leftExpr.link);
                leftExpr=out;
                break;
            default:
                break;
        }
    }
}

Expression RuntimeEngine::parseAddExpression(Ast* pAst) {
    Expression expression(pAst), right(pAst);
    token_entity operand = pAst->hasEntity(PLUS) ? pAst->getEntity(PLUS) : pAst->getEntity(MINUS);

    if(operand.getTokenType() == PLUS && pAst->getSubAstCount() == 1) {
        // left is right then add 1 to data
        right = parseExpression(pAst->getSubAst(0));
        return parseUnary(operand, right, pAst);
    } else if(operand.getTokenType() == MINUS && pAst->getSubAstCount() == 1) {
        // left is right multiply expression by -1
        right = parseExpression(pAst->getSubAst(0));
        return parseUnary(operand, right, pAst);
    }

    retry:
    if(pAst->getSubAst(ast_add_e)) {
        for(;;) { // very silly, is there a better way?
            pAst = pAst->getSubAst(ast_add_e);
            goto retry;
        }
    }
    parseAddExpressionChain(expression, pAst);

    expression.link = pAst;
    return expression;
}

Expression RuntimeEngine::parseMultExpression(Ast* pAst) {
    Expression expression(pAst), left(pAst), right(pAst);
    retry:
    if(pAst->getSubAst(ast_mult_e)) {
        for(;;) { // very silly, is there a better way?
            pAst = pAst->getSubAst(ast_mult_e);
            goto retry;
        }
    }

    token_entity operand = pAst->getEntity(0);

    if(pAst->getSubAstCount() == 1) {
        // cannot compute unary *<expression>
        errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + operand.getToken() + "`");
        return Expression(pAst);
    }


    parseAddExpressionChain(expression, pAst);

    expression.link = pAst;
    return expression;
}

bool RuntimeEngine::shiftLiteralExpressions(Expression &out, Expression &leftExpr, Expression &rightExpr,
                                      token_entity operand) {
    double var=0;
    if(operand == "<<")
        var = (int64_t)leftExpr.intValue << (int64_t)rightExpr.intValue;
    else if(operand == ">>")
        var = (int64_t)leftExpr.intValue >> (int64_t)rightExpr.intValue;

    if(isDClassNumberEncodable(var)) {
        return false;
    } else {
        double intpart;
        if( modf( var, &intpart) != 0 ) {
            // movbi
            out.code.push_i64(SET_Di(i64, op_MOVBI, ((int64_t)var)), abs(get_low_bytes(var)));
            out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64bmr));
        } else {
            // movi
            out.code.push_i64(SET_Di(i64, op_MOVI, var), i64ebx);
        }

        rightExpr.literal = true;
        rightExpr.intValue = var;
        rightExpr.code.free();
    }

    out.literal = true;
    out.intValue = var;
    return true;
}

Opcode RuntimeEngine::operandToShftOp(token_entity operand)
{
    if(operand == "<<")
        return op_SHL;
    else
        return op_SHR;
}

void RuntimeEngine::shiftNative(token_entity operand, Expression& out, Expression &left, Expression &right, Ast* pAst) {
    out.type = expression_var;
    right.type = expression_var;
    right.func=false;
    right.literal = false;

    if(left.type == expression_var) {
        equals(left, right);

        pushExpressionToRegister(right, out, i64egx);
        pushExpressionToRegister(left, out, i64ebx);
        out.code.push_i64(SET_Ci(i64, operandToShftOp(operand), i64ebx,0, i64egx), i64ebx);
        right.code.free();
    } else if(left.type == expression_field) {
        if(left.utype.field.isVar()) {
            equals(left, right);

            pushExpressionToRegister(right, out, i64egx); // no inject?
            pushExpressionToRegister(left, out, i64ebx);
            out.code.push_i64(SET_Ci(i64, operandToShftOp(operand), i64ebx,0, i64egx), i64ebx);
            right.code.free();
        }
        else {
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
        }
    } else {
        errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
    }
}

Expression RuntimeEngine::parseShiftExpression(Ast* pAst) {
    Expression out(pAst), left(pAst), right(pAst);
    token_entity operand = pAst->getEntity(0);

    if(pAst->getSubAstCount() == 1) {
        // cannot compute unary << <expression>
        errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + operand.getToken() + "`");
        return Expression(pAst);
    }

    left = parseIntermExpression(pAst->getSubAst(0));
    right = parseExpression(pAst->getSubAst(1));

    out.type = expression_var;
    switch(left.type) {
        case expression_var:
            if(right.type == expression_var) {
                if(left.literal && right.literal) {

                    if(!shiftLiteralExpressions(out, left, right, operand)) {
                        goto calculate;
                    }
                } else {
                    calculate:
                    // is left left expr a literal?
                    pushExpressionToStack(right, out);
                    pushExpressionToRegister(left, out, i64ebx);
                    out.code.push_i64(SET_Di(i64, op_LOADVAL, i64ecx));
                    out.code.push_i64(SET_Ci(i64, operandToShftOp(operand), i64ebx,0, i64ecx), i64ebx);
                }
            }
            else if(right.type == expression_field) {
                if(right.utype.field.isNative()) {
                    // add var
                    shiftNative(operand, out, left, right, pAst);
                    //addNative(operand, right.utype.field.nf, expression, left, right, pAst);
                } else if(right.utype.field.type == CLASS) {
                    addClass(operand, right.utype.field.klass, out, left, right, pAst);
                } else {
                    // do nothing field unresolved
                }
            } else if(right.type == expression_lclass) {
                addClass(operand, left.utype.klass, out, left, right, pAst);
            } else {
                errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                  "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            }
            break;
        case expression_null:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                              "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            break;
        case expression_field:
            if(left.utype.field.isVar()) {
                // add var
                shiftNative(operand, out, left, right, pAst);
            } else if(left.utype.field.type == CLASS) {
                addClass(operand, left.utype.field.klass, out, left, right, pAst);
            } else {
                // do nothing field unresolved
            }
            break;
        case expression_native:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + left.typeToString() + "`");
            break;
        case expression_lclass:
            addClass(operand, left.utype.klass, out, left, right, pAst);
            break;
        case expression_class:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + left.typeToString() + "`");
            break;
        case expression_void:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                              "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            break;
        case expression_objectclass:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                              "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`. Did you forget to apply a cast? "
                                                                      "i.e ((SomeClass)dynamic_class) " + operand.getToken() + " <data>");
            break;
        case expression_string:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                              "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            break;
        default:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                              "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            break;
    }

    out.link = pAst;
    return out;
}

void RuntimeEngine::lessThanLiteralExpressions(Expression &out, Expression &leftExpr, Expression &rightExpr,
                                         token_entity operand) {
    double var=0;
    if(operand == "<")
        var = leftExpr.intValue < rightExpr.intValue;
    else if(operand == ">")
        var = leftExpr.intValue > rightExpr.intValue;
    else if(operand == ">=")
        var = leftExpr.intValue >= rightExpr.intValue;
    else if(operand == "<=")
        var = leftExpr.intValue <= rightExpr.intValue;

    out.code.push_i64(SET_Di(i64, op_MOVI, var), i64ebx);

    rightExpr.literal = true;
    rightExpr.intValue = var;
    rightExpr.code.free();

    out.literal = true;
    out.intValue = var;
}

Opcode RuntimeEngine::operandToLessOp(token_entity operand)
{
    if(operand == "<")
        return op_LT;
    else if(operand == "<=")
        return op_LTE;
    else if(operand == ">")
        return op_GT;
    else if(operand == ">=")
        return op_GTE;
    else
        return op_LT;
}

void RuntimeEngine::lessThanNative(token_entity operand, Expression& out, Expression &left, Expression &right, Ast* pAst) {
    out.type = expression_var;
//    right.func=false;
//    right.literal = false;

    if(left.type == expression_var) {
        equals(left, right);

        pushExpressionToRegister(right, out, i64egx);
        pushExpressionToRegister(left, out, i64ebx);
        out.code.push_i64(SET_Ci(i64, operandToLessOp(operand), i64ebx,0, i64egx));
        out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
        out.inCmtRegister = true;
        right.code.free();
    } else if(left.type == expression_field) {
        if(left.utype.field.isVar()) {
            equals(left, right);

            pushExpressionToRegister(right, out, i64egx);
            pushExpressionToRegister(left, out, i64ebx);
            out.code.push_i64(SET_Ci(i64, operandToLessOp(operand), i64ebx,0, i64egx));
            out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
            out.inCmtRegister = true;
            right.code.free();
        }
        else {
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
        }
    } else {
        errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
    }
}

Expression RuntimeEngine::parseLessExpression(Ast* pAst) {
    Expression out(pAst), left(pAst), right(pAst);
    token_entity operand = pAst->getEntity(0);

    if(pAst->getSubAstCount() == 1) {
        // cannot compute unary < <expression>
        errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + operand.getToken() + "`");
        return Expression(pAst);
    }

    left = parseIntermExpression(pAst->getSubAst(0));
    right = parseExpression(pAst->getSubAst(1));

    out.type = expression_var;
    switch(left.type) {
        case expression_var:
            if(right.type == expression_var) {
                if(left.literal && right.literal) {
                    lessThanLiteralExpressions(out, left, right, operand);
                } else {
                    calculate:
                    // is left left expr a literal?
                    pushExpressionToStack(right, out);
                    pushExpressionToRegister(left, out, i64ebx);
                    out.code.push_i64(SET_Di(i64, op_LOADVAL, i64ecx));
                    out.code.push_i64(SET_Ci(i64, operandToLessOp(operand), i64ebx,0, i64ecx));
                    out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
                    out.inCmtRegister = true;
                }
            }
            else if(right.type == expression_field) {
                if(right.utype.field.isVar()) {
                    // add var
                    lessThanNative(operand, out, left, right, pAst);
                } else if(right.utype.field.type == CLASS) {
                    addClass(operand, right.utype.field.klass, out, left, right, pAst);
                } else {
                    // do nothing field unresolved
                }
            } else if(right.type == expression_lclass) {
                addClass(operand, left.utype.klass, out, left, right, pAst);
            } else {
                errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                  "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            }
            break;
        case expression_null:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                              "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            break;
        case expression_field:
            if(left.utype.field.isVar()) {
                // add var
                lessThanNative(operand, out, left, right, pAst);
            } else if(left.utype.field.type == CLASS) {
                addClass(operand, left.utype.field.klass, out, left, right, pAst);
            } else {
                // do nothing field unresolved
            }
            break;
        case expression_native:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + left.typeToString() + "`");
            break;
        case expression_lclass:
            addClass(operand, left.utype.klass, out, left, right, pAst);
            break;
        case expression_class:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + left.typeToString() + "`");
            break;
        case expression_void:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                              "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            break;
        case expression_objectclass:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                              "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`. Did you forget to apply a cast? "
                                                                      "i.e ((SomeClass)dynamic_class) " + operand.getToken() + " <data>");
            break;
        case expression_string:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                              "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            break;
        default:
            break;
    }

    out.link = pAst;
    return out;
}

bool RuntimeEngine::equalsNoErr(Expression& left, Expression& right) {

    if(left.type == expression_native) {
        return false;
    }
    if(right.type == expression_native) {
        return false;
    }

    switch(left.type) {
        case expression_var:
            if(right.type == expression_var) {
                // add 2 vars
                return true;
            }
            else if(right.type == expression_field) {
                if(right.utype.field.isNative()) {
                    if(right.utype.field.isVar()) {
                        return right.utype.field.isArray==left.isArray();
                    }
                }
            }
            break;
        case expression_null:
            if(right.type == expression_lclass || right.type == expression_objectclass) {
                return true;
            } else if(right.type == expression_class) {
                return false;
            }
            break;
        case expression_field:
            if(left.utype.field.isNative()) {
                // add var
                if(right.trueType() == OBJECT || right.trueType() == CLASS) {
                    if(left.trueType() == OBJECT) {
                        return true;
                    } else if(isNativeIntegerClass(right.getClass())) {
                        if(left.trueType() == VAR)
                            return true;
                    }
                } else if(right.type == expression_string || right.type == expression_null) {
                    return left.utype.field.isArray;
                } else if(right.trueType() == VAR || (right.trueType() != CLASS)) {
                    if(left.trueType() == VAR) {
                        return left.isArray() == right.isArray();
                    } else if(left.trueType() == OBJECT) {
                        return !left.isArray() && right.isArray();
                    }
                }
            } else if(left.utype.field.type == CLASS) {
                if(right.type == expression_lclass) {
                    if(left.utype.field.klass->assignable(right.utype.klass)) {
                        return true;
                    }
                } else if(right.type == expression_class) {
                    if(left.utype.field.klass->assignable(right.utype.klass)) {
                        return false;
                    }
                } else if(right.type == expression_field && right.utype.field.type == CLASS) {
                    if(right.utype.field.klass->assignable(left.utype.field.klass)) {
                        return true;
                    }
                } else if(right.type == expression_null) {
                    return true;
                }
            } else if(right.trueType() == VAR) {
                return left.isArray() == right.isArray();
            } else {
                // do nothing field unresolved
            }
            break;
        case expression_lclass:
            if(right.type == expression_lclass) {
                if(left.utype.klass->assignable(right.utype.klass)) {
                    return true;
                }
            } else if(right.type == expression_field) {
                if(left.utype.klass->assignable(right.utype.field.klass)) {
                    return true;
                }
            } else if(right.type == expression_class) {
                if(left.utype.klass->assignable(right.utype.klass)) {
                    return false;
                }
            }
            break;
        case expression_class:
            if(right.type == expression_class) {
                if(left.utype.klass->assignable(right.utype.klass)) {
                    return true;
                }
            }
            break;
        case expression_void:
            if(right.type == expression_void) {
                return true;
            }
            break;
        case expression_objectclass:
            if(right.trueType() == OBJECT || right.trueType() == CLASS
               || right.trueType() == VAR) {

                if(right.trueType() == VAR)
                    return !left.isArray() && right.isArray();
                return true;
            }
            break;
        case expression_string:
            if(right.trueType() == VAR) {
                if(right.isArray()) {
                    return true;
                }
            }
            else if(right.type == expression_string) {
                return true;
            }
            break;
        default:
            break;
    }

    return false;
}

void RuntimeEngine::assignValue(token_entity operand, Expression& out, Expression &left, Expression &right, Ast* pAst) {
    out.type=expression_var;
    out.literal=false;

    /*
     * We know when this is called the operand is going to be =
     */
    if((left.type == expression_var && !left.arrayElement && right.type != expression_null) || left.literal) {
        errors->createNewError(GENERIC, pAst->line, pAst->col, "expression is not assignable, expression must be of lvalue");
    } else if(left.type == expression_field) {
        if(left.utype.field.isObjectInMemory()) {
            if(left.utype.field.isArray && operand != "=" && right.type != expression_null) {
                errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken()
                                                                  + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            }

            if(operand == "=" && right.type == expression_null) {
                out.code.inject(out.code.__asm64.size(), left.code);
                out.code.push_i64(SET_Ei(i64, op_DEL));
                return;
            } else if(left.utype.field.type != CLASS && right.type == expression_string) {
                pushExpressionToStack(right, out);
                out.code.inject(out.code.__asm64.size(), left.code);

                out.code.push_i64(SET_Ei(i64, op_POPOBJ));
                return;
            } else if(operand == "==" && right.type == expression_null) {
                pushExpressionToPtr(left, out);
                if(left.func)
                    out.code.push_i64(SET_Ei(i64, op_POP));

                out.code.push_i64(SET_Ei(i64, op_CHECKNULL));
                out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
                out.inCmtRegister = true;
                return;
            } else if(operand == "==" && (right.trueType() == OBJECT
                                          || (right.utype.isField && right.utype.field.isObjectInMemory()))) {

                pushExpressionToStack(right, out);
                pushExpressionToStack(left, out);

                out.code.push_i64(SET_Di(i64, op_ITEST, i64ebx));
                out.code.push_i64(SET_Ci(i64, op_MOVR, i64cmt,0, i64ebx));
                out.inCmtRegister = true;
                return;
            } else if(operand == "!=" && right.type == expression_null) {
                pushExpressionToPtr(left, out);
                if(left.func)
                    out.code.push_i64(SET_Ei(i64, op_POP));

                out.code.push_i64(SET_Ei(i64, op_CHECKNULL));
                out.code.push_i64(SET_Ci(i64, op_NOT, i64cmt,0, i64cmt));
                out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
                out.inCmtRegister = true;
                return;
            } else if(operand == "=" && right.type == expression_var && left.utype.field.type == OBJECT) {
                pushExpressionToStack(right, out);
                out.inject(left);

                out.utype = left.utype;
                out.code.push_i64(SET_Di(i64, op_MOVI, 1), i64ebx);
                out.code.push_i64(SET_Di(i64, op_MOVI, 0), i64adx);
                out.code.push_i64(SET_Di(i64, op_NEWARRAY, i64ebx));
                out.code.push_i64(SET_Ei(i64, op_POPOBJ));
                out.code.push_i64(SET_Di(i64, op_LOADVAL, i64ebx));
                out.code.push_i64(SET_Ci(i64, op_RMOV, i64adx, 0, i64ebx));
                return;
            }else if(right.type == expression_null) {
                errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken()
                                                                  + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            }

            if(equalsNoErr(left, right)) {
                if(left.utype.field.isVar()) {}
                else if(left.utype.field.dynamicObject()) {
                    out.type=expression_objectclass;
                }else if(left.utype.field.type == CLASS) {
                    out.type=expression_lclass;
                    out.utype.klass=left.utype.field.klass;
                }

                memassign:
                if(left.trueType() == OBJECT && right.trueType() == VAR && !right.isArray()) {

                    pushExpressionToRegister(right, out, i64ecx);
                    out.inject(left);
                    
                    out.code.push_i64(SET_Di(i64, op_MOVI, 1), i64ebx);
                    out.code.push_i64(SET_Di(i64, op_NEWARRAY, i64ebx));
                    out.code.push_i64(SET_Ei(i64, op_POPOBJ));
                    out.code.push_i64(SET_Di(i64, op_MOVI, 0), i64adx);
                    out.code.push_i64(SET_Ci(i64, op_RMOV, i64adx, 0, i64ecx));
                } else {

                    pushExpressionToStack(right, out);
                    out.inject(left);
                    out.code.push_i64(SET_Ei(i64, op_POPOBJ));
                }

            } else {
                if(left.trueType() == CLASS) {
                    addClass(operand, left.getClass(), out, left, right, pAst);
                } else {
                    if(left.trueType() == CLASS && right.trueType() == CLASS) {
                        errors->createNewError(GENERIC, right.link->line,  right.link->col, "classes in expression are not compatible");
                    } else
                        errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken()
                                                                      + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
                }
            }
        } else {
            // this will b easy...
            if(left.utype.field.local) {

                if(operand == "=") {
                    if(right.literal && isWholeNumber(right.intValue)) {
                        out.code.push_i64(SET_Di(i64, op_ISTOREL, left.utype.field.address), right.intValue);
                    } else {

                        pushExpressionToRegister(right, out, i64ebx);
                        out.code.push_i64(SET_Ci(i64, op_SMOVR_2, i64ebx,0, left.utype.field.address));
                    }
                } else if(operand == "+=") {
                    pushExpressionToRegister(right, out, i64ebx);
                    out.code.push_i64(SET_Ci(i64, op_ADDL, i64ebx,0, left.utype.field.address));
                } else if(operand == "-=") {
                    pushExpressionToRegister(right, out, i64ebx);
                    out.code.push_i64(SET_Ci(i64, op_SUBL, i64ebx,0, left.utype.field.address));
                } else if(operand == "*=") {
                    pushExpressionToRegister(right, out, i64ebx);
                    out.code.push_i64(SET_Ci(i64, op_MULL, i64ebx,0, left.utype.field.address));
                } else if(operand == "/=") {
                    pushExpressionToRegister(right, out, i64ebx);
                    out.code.push_i64(SET_Ci(i64, op_DIVL, i64ebx,0, left.utype.field.address));
                } else if(operand == "%=") {
                    pushExpressionToRegister(right, out, i64ebx);
                    out.code.push_i64(SET_Ci(i64, op_MODL, i64ebx,0, left.utype.field.address));
                } else if(operand == "&=") {
                    pushExpressionToRegister(right, out, i64ebx);
                    out.code.push_i64(SET_Ci(i64, op_ANDL, i64ebx,0, left.utype.field.address));
                } else if(operand == "|=") {
                    pushExpressionToRegister(right, out, i64ebx);
                    out.code.push_i64(SET_Ci(i64, op_ORL, i64ebx,0, left.utype.field.address));
                } else if(operand == "^=") {
                    pushExpressionToRegister(right, out, i64ebx);
                    out.code.push_i64(SET_Ci(i64, op_XORL, i64ebx,0, left.utype.field.address));
                }

            } else {
                /* we need this to prevent the optimizer from screwing us */
                right.code.push_i64(SET_Ei(i64, op_NOP));

                if(left.isProtoType()) {
                    if(operand == "=") {

                        if(right.isProtoType()) {
                            if(!prototypeEquals(&left.utype.field, right.utype.getParams(), right.utype.getReturnType())) {
                                errors->createNewError(GENERIC, right.link->line,  right.link->col, "Expressions of type `fn*" + paramsToString(left.utype.field.params) +
                                                       (left.utype.field.returnType==TYPEVOID ? "" : ": " + ResolvedReference::typeToString(left.utype.field.returnType))
                                                       + "` and `" + right.typeToString() + "` are not compatible");
                            }
                        } else {
                            errors->createNewError(GENERIC, right.link->line,  right.link->col, "Expressions of type `fn*" + paramsToString(left.utype.field.params) +
                                    (left.utype.field.returnType==TYPEVOID ? "" : ": " + ResolvedReference::typeToString(left.utype.field.returnType))
                                    + "` and `" + right.typeToString() + "` are not compatible");
                        }
                    } else {
                        errors->createNewError(GENERIC, right.link->line,  right.link->col, " operator `" + operand.getToken() + "` is not allowed");
                    }
                }

                pushExpressionToStack(right, out);
                if(operand == "=")
                    out.inject(left);
                else
                    pushExpressionToRegister(left, out, i64ebx);


                out.code.push_i64(SET_Di(i64, op_MOVI, 0), i64adx);
                out.code.push_i64(SET_Di(i64, op_LOADVAL, i64ecx));

                if(operand == "=") {
                    out.code.push_i64(SET_Ci(i64, op_RMOV, i64adx,0, i64ecx));
                } else if(operand == "+=") {
                    out.code.push_i64(SET_Ci(i64, op_ADD, i64ebx,0, i64ecx), i64ecx);
                    out.code.push_i64(SET_Ci(i64, op_RMOV, i64adx,0, i64ecx));
                } else if(operand == "-=") {
                    out.code.push_i64(SET_Ci(i64, op_SUB, i64ebx,0, i64ecx), i64ecx);
                    out.code.push_i64(SET_Ci(i64, op_RMOV, i64adx,0, i64ecx));
                } else if(operand == "*=") {
                    out.code.push_i64(SET_Ci(i64, op_MUL, i64ebx,0, i64ecx), i64ecx);
                    out.code.push_i64(SET_Ci(i64, op_RMOV, i64adx,0, i64ecx));
                } else if(operand == "/=") {
                    out.code.push_i64(SET_Ci(i64, op_DIV, i64ebx,0, i64ecx), i64ecx);
                    out.code.push_i64(SET_Ci(i64, op_RMOV, i64adx,0, i64ecx));
                } else if(operand == "%=") {
                    out.code.push_i64(SET_Ci(i64, op_MOD, i64ebx,0, i64ecx), i64ecx);
                    out.code.push_i64(SET_Ci(i64, op_RMOV, i64adx,0, i64ecx));
                }
            }
        }
    } else if(left.type == expression_lclass) {
        if(operand == "=" && right.type == expression_null) {
            out.code.inject(out.code.__asm64.size(), left.code);
            out.code.push_i64(SET_Ei(i64, op_DEL));
            return;
        } else if(equalsNoErr(left, right) && right.type == expression_string) {
            pushExpressionToStack(right, out);
            out.code.inject(out.code.__asm64.size(), left.code);

            out.code.push_i64(SET_Ei(i64, op_POPOBJ));
            return;
        } else if(operand == "==" && right.type == expression_null) {
            pushExpressionToPtr(left, out);
            if(left.func)
                out.code.push_i64(SET_Ei(i64, op_POP));

            out.code.push_i64(SET_Ei(i64, op_CHECKNULL));
            out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
            out.inCmtRegister = true;
            return;
        } else if(operand == "==" && (right.trueType() == OBJECT
                                      || (right.utype.isField && right.utype.field.isObjectInMemory()))) {

            pushExpressionToStack(right, out);
            pushExpressionToStack(left, out);

            out.code.push_i64(SET_Di(i64, op_ITEST, i64ebx));
            out.code.push_i64(SET_Ci(i64, op_MOVR, i64cmt,0, i64ebx));
            out.inCmtRegister = true;
            return;
        } else if(operand == "!=" && right.type == expression_null) {
            pushExpressionToPtr(left, out);
            if(left.func)
                out.code.push_i64(SET_Ei(i64, op_POP));

            out.code.push_i64(SET_Ei(i64, op_CHECKNULL));
            out.code.push_i64(SET_Ci(i64, op_NOT, i64cmt,0, i64cmt));
            out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
            out.inCmtRegister = true;
            return;
        } else if(equalsNoErr(left, right) && operand == "=" && right.type == expression_var) {
            pushExpressionToStack(right, out);
            out.inject(left);

            out.utype = left.utype;
            out.code.push_i64(SET_Di(i64, op_MOVI, 1), i64ebx);
            out.code.push_i64(SET_Di(i64, op_MOVI, 0), i64adx);
            out.code.push_i64(SET_Di(i64, op_NEWARRAY, i64ebx));
            out.code.push_i64(SET_Ei(i64, op_POPOBJ));
            out.code.push_i64(SET_Di(i64, op_LOADVAL, i64ebx));
            out.code.push_i64(SET_Ci(i64, op_RMOV, i64adx, 0, i64ebx));
            return;
        }

        out.type=expression_lclass;
        out.utype=left.utype;
        goto memassign;
    } else if(left.type == expression_var) {
        // this must be an array element
        if(left.arrayElement) {
            Expression e(pAst);
            pushExpressionToRegister(right, out, i64ebx);
            out.code.push_i64(SET_Di(i64, op_RSTORE, i64ebx));

            if(operand == "=") {
                for(unsigned int i = left.code.size()-1; i > 0 ; i--)
                {
                    if(GET_OP(left.code.__asm64.get(i)) == op_IALOAD_2) {
                        left.code.__asm64.remove(i);
                        unsigned  int pos = i+1;

                        left.code.push_i64(SET_Di(i64, op_LOADVAL, i64egx));
                        left.code.push_i64(SET_Ci(i64, op_RMOV, i64ebx,0, i64egx));
                        break;
                    }
                }
            } else if(operand == "+=" || operand == "-=" || operand == "*="
                      || operand == "/=" || operand == "%=") {
                for(unsigned int i = left.code.size()-1; i  > 0; i++)
                {
                    if(GET_OP(left.code.__asm64.get(i)) == op_IALOAD_2) {
                        unsigned  int pos = i+2;

                        left.code.__asm64.insert(i, SET_Ci(i64, op_MOVR, i64adx, 0, i64ebx));

                        left.code.__asm64.push_back(SET_Di(i64, op_LOADVAL, i64egx) );
                        left.code.push_i64(SET_Ci(i64, assignOperandToOp(operand), i64ebx,0, i64egx), i64egx);
                        left.code.push_i64(SET_Ci(i64, op_RMOV, i64adx,0, i64egx));
//                        left.code.__asm64.insert(pos, SET_Ci(i64, op_RMOV, i64ebx,0, i64egx));
                        break;
                    }
                }
            }

            out.inject(left);
        } else if(right.type == expression_null){
            if(left.isArray() && operand == "==") {
                pushExpressionToPtr(left, out);
                if(left.func)
                    out.code.push_i64(SET_Ei(i64, op_POP));

                out.code.push_i64(SET_Ei(i64, op_CHECKNULL));
                out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
                out.inCmtRegister = true;
                return;
            }  else if(left.isArray() && operand == "!=" && right.type == expression_null) {
                pushExpressionToPtr(left, out);
                if(left.func)
                    out.code.push_i64(SET_Ei(i64, op_POP));

                out.code.push_i64(SET_Ei(i64, op_CHECKNULL));
                out.code.push_i64(SET_Ci(i64, op_NOT, i64cmt,0, i64cmt));
                out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
                out.inCmtRegister = true;
                return;
            } else {
                errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken()
                                                                        + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            }
        }
    } else if(left.type == expression_objectclass) {
        if(operand == "=" && right.type == expression_null) {
            out.code.inject(out.code.__asm64.size(), left.code);
            out.code.push_i64(SET_Ei(i64, op_DEL));
            return;
        } else if(right.type == expression_string) {
            pushExpressionToStack(right, out);
            out.code.inject(out.code.__asm64.size(), left.code);

            out.code.push_i64(SET_Ei(i64, op_POPOBJ));
            return;
        } else if(operand == "==" && right.type == expression_null) {
            pushExpressionToPtr(left, out);
            if(left.func)
                out.code.push_i64(SET_Ei(i64, op_POP));

            out.code.push_i64(SET_Ei(i64, op_CHECKNULL));
            out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
            out.inCmtRegister = true;
            return;
        } else if(operand == "!=" && right.type == expression_null) {
            pushExpressionToPtr(left, out);
            if(left.func)
                out.code.push_i64(SET_Ei(i64, op_POP));

            out.code.push_i64(SET_Ei(i64, op_CHECKNULL));
            out.code.push_i64(SET_Ci(i64, op_NOT, i64cmt,0, i64cmt));
            out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
            out.inCmtRegister = true;
            return;
        } else if(operand == "==" && (right.trueType() == OBJECT
                                       || (right.utype.isField && right.utype.field.isObjectInMemory()))) {

            pushExpressionToStack(right, out);
            pushExpressionToStack(left, out);

            out.code.push_i64(SET_Di(i64, op_ITEST, i64ebx));
            out.code.push_i64(SET_Ci(i64, op_MOVR, i64cmt,0, i64ebx));
            out.inCmtRegister = true;
            return;
        } else if(operand == "!=" && (right.trueType() == OBJECT
                                       || (right.utype.isField && right.utype.field.isObjectInMemory()))) {

            pushExpressionToStack(right, out);
            pushExpressionToStack(left, out);

            out.code.push_i64(SET_Di(i64, op_ITEST, i64ebx));
            out.code.push_i64(SET_Ci(i64, op_NOT, i64ebx,0, i64ebx));
            out.code.push_i64(SET_Ci(i64, op_MOVR, i64cmt,0, i64ebx));
            out.inCmtRegister = true;
            return;
        } else if(right.type == expression_null) {
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken()
                                                              + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
        }

        if(equalsNoErr(left, right) && operand == "=") {
            out.type=expression_objectclass;
            pushExpressionToStack(right, out);
            pushExpressionToPtr(left, out);
            if(right.trueType() == VAR) {
                out.code.push_i64(SET_Di(i64, op_LOADVAL, i64egx));
                out.code.push_i64(SET_Di(i64, op_MOVI, 0), i64ebx);
                out.code.push_i64(SET_Ci(i64, op_RMOV, i64ebx, 0, i64egx));
            } else
                out.code.push_i64(SET_Ei(i64, op_POPOBJ));
        } else {
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken()
                                                              + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
        }
    } else {
        errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken()
                                                          + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
    }

}

Opcode RuntimeEngine::operandToCompareOp(token_entity operand)
{
    if(operand == "!=")
        return op_TNE;
    else if(operand == "==")
        return op_TEST;
    else
        return op_TEST;
}

void RuntimeEngine::assignNative(token_entity operand, Expression& out, Expression &left, Expression &right, Ast* pAst) {
    out.type = expression_var;

    if(left.type == expression_var) {
        equals(left, right);

        pushExpressionToStack(left, out);
        pushExpressionToRegister(right, out, i64egx);
        out.code.push_i64(SET_Di(i64, op_LOADVAL, i64ebx));
        out.code.push_i64(SET_Ci(i64, operandToCompareOp(operand), i64ebx,0, i64egx));
        out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
        out.inCmtRegister = true;
        right.code.free();
    } else if(left.type == expression_field) {
        if(left.utype.field.isVar()) {
            equals(left, right);

            pushExpressionToStack(left, out);
            pushExpressionToRegister(right, out, i64egx);
            out.code.push_i64(SET_Di(i64, op_LOADVAL, i64ebx));
            out.code.push_i64(SET_Ci(i64, operandToCompareOp(operand), i64ebx,0, i64egx));
            out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
            out.inCmtRegister = true;
            right.code.free();
        }
        else {
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
        }
    } else {
        errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() + "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
    }
}


Expression RuntimeEngine::parseEqualExpression(Ast* pAst) {
     Expression out(pAst), left(pAst), right(pAst);
    token_entity operand = pAst->getEntity(0);

    if(pAst->getSubAstCount() == 1) {
        // cannot compute unary = <expression>
        errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + operand.getToken() + "`");
        return Expression(pAst);
    }

    left = parseIntermExpression(pAst->getSubAst(0));
    right = parseExpression(pAst->getSubAst(1));

    out.type = expression_var;
    switch(left.type) {
        case expression_var:
            if(operand.getTokenType() == ASSIGN) {
                assignValue(operand, out, left, right, pAst);
            } else {
                if(right.type == expression_var) {
                    assignNative(operand, out, left, right, pAst);
                }
                else if(right.type == expression_field) {
                    if(right.utype.field.isVar()) {
                        // add var
                        assignNative(operand, out, left, right, pAst);
                    } else if(right.utype.field.type == CLASS) {
                        addClass(operand, right.utype.field.klass, out, right, left, pAst);
                    } else {
                        // do nothing field unresolved
                    }
                } else if(right.type == expression_lclass) {
                    addClass(operand, left.utype.klass, out, right, left, pAst);
                } else if(right.type == expression_null) {
                    assignValue(operand, out, left, right, pAst);
                } else {
                    errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                      "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
                }
            }
            break;
        case expression_null:
            errors->createNewError(GENERIC, pAst->line, pAst->col, "expression is not assignable");
            break;
        case expression_field:
            if(left.utype.field.isVar() ) {
                // add var
                if(right.type == expression_null || operand.getTokenType() == ASSIGN
                        || (right.trueType() == CLASS && hasOverload(operand, left, right.getClass(), pAst))) {
                    if(right.trueType() == CLASS && hasOverload(operand, left, right.getClass(), pAst))
                        assignValue(operand, out, right, left, pAst);
                    else
                        assignValue(operand, out, left, right, pAst);
                } else {
                    assignNative(operand, out, left, right, pAst);
                }
            } else if(left.utype.field.type == CLASS) {
                if(right.type == expression_null || operand.getTokenType() == ASSIGN) {
                    assignValue(operand, out, left, right, pAst);
                } else {
                    addClass(operand, left.utype.field.klass, out, left, right, pAst);
                }
            }  else if(left.utype.field.type == OBJECT) {
                if(right.type == expression_null || operand.getTokenType() == ASSIGN
                   || operand.getTokenType() == EQEQ) {
                    assignValue(operand, out, left, right, pAst);
                } else {
                    addClass(operand, left.utype.field.klass, out, left, right, pAst);
                }
            } else {
                // do nothing field unresolved
            }
            break;
        case expression_native:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + left.typeToString() + "`");
            break;
        case expression_lclass:
            if(right.type == expression_null || operand.getTokenType() == ASSIGN) {
                assignValue(operand, out, left, right, pAst);
            } else {
                addClass(operand, left.utype.klass, out, left, right, pAst);
            }
            break;
        case expression_class:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + left.typeToString() + "`");
            break;
        case expression_void:
            errors->createNewError(GENERIC, pAst->line, pAst->col, "expression is not assignable/comparable");
            break;
        case expression_objectclass:
            if(right.trueType()==OBJECT || right.trueType()==CLASS
               || right.trueType()==VAR) {
                assignValue(operand, out, left, right, pAst);
            } else
                errors->createNewError(GENERIC, pAst->line, pAst->col, "expression is not assignable/comparable. Did you forget to apply a cast? "
                                                                         "i.e ((SomeClass)dynamic_class) " + operand.getToken() + " <data>");
            break;
        case expression_string:
            if(operand.getTokenType() == ASSIGN) {
                errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                  "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            } else {

                constructNewString(left, right, operand, out, left.link);
            }
            break;
        default:
            break;
    }

    out.link = pAst;
    return out;
}

int recursiveAndExprssions = 0;
List<long> skipAddress;
Expression RuntimeEngine::parseAndExpression(Ast* pAst) {
    Expression out(pAst), left(pAst), right(pAst);
    recursiveAndExprssions++;

    if(recursiveAndExprssions==1) {
        skipAddress.free();
    }

    token_entity operand = pAst->getEntity(0);

    if(pAst->getSubAstCount() == 1) {
        // cannot compute unary << <expression>
        errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + operand.getToken() + "`");
        return Expression(pAst);
    }

    bool value=0;
    left = pAst->getSubAst(0)->getType() == ast_expression ? parseExpression(pAst->getSubAst(0)) : parseIntermExpression(pAst->getSubAst(0));
    right = pAst->getSubAst(1)->getType() == ast_expression ? parseExpression(pAst->getSubAst(1)) : parseIntermExpression(pAst->getSubAst(1));

    out.type=expression_var;
    switch(left.type) {
        case expression_var:
            if(right.type == expression_var) {
                if(left.literal && right.literal) {
                    if(operand == "&&") {
                        value=left.intValue&&right.intValue;
                        out.code.push_i64(SET_Di(i64, op_MOVI, value), i64ebx);
                    } else if(operand == "||") {
                        value=left.intValue||right.intValue;
                        out.code.push_i64(SET_Di(i64, op_MOVI, value), i64ebx);
                    } else if(operand == "&") {
                        out.code.push_i64(SET_Di(i64, op_MOVI, left.intValue&(int64_t)right.intValue), i64ebx);
                    } else if(operand == "|") {
                        out.code.push_i64(SET_Di(i64, op_MOVI, left.intValue|(int64_t)right.intValue), i64ebx);
                    } else if(operand == "^") {
                        out.code.push_i64(SET_Di(i64, op_MOVI, left.intValue^(int64_t)right.intValue), i64ebx);
                    }

                } else {
                    if((left.literal || right.literal) && operand == "||") {
                        if(left.literal) {
                            Expression tmp(pAst);
                            pushExpressionToRegister(right, tmp, i64ebx);

                            out.code.push_i64(SET_Di(i64, op_MOVI, left.intValue!=0), i64cmt);
                            out.code.push_i64(SET_Di(i64, op_SKNE, tmp.code.size()));
                            out.inject(right);
                            out.inCmtRegister = true;
                        } else {
                            Expression tmp(pAst);
                            pushExpressionToRegister(left, tmp, i64cmt);

                            out.code.push_i64(SET_Di(i64, op_SKNE, 1));
                            out.code.push_i64(SET_Di(i64, op_MOVI, right.intValue!=0), i64cmt);
                            out.inCmtRegister = true;
                        }
                    } else {
                        // is left left a literal?
                        evaluate:
                        if(operand == "&&") {
                            Expression tmp(pAst);
                            pushExpressionToRegister(right, tmp, i64ebx);

                            pushExpressionToRegister(left, out, i64ebx);
                            out.code.push_i64(SET_Ci(i64, op_CMP, i64ebx, 0, 1));
                            out.code.push_i64(SET_Di(i64, op_SKNE, (tmp.code.size()+4)));
                            skipAddress.add(out.code.size()-1);
                            out.code.push_i64(SET_Di(i64, op_ISTORE, 1));
                            out.inject(tmp);
                            out.code.push_i64(SET_Di(i64, op_LOADVAL, i64ecx));
                            out.code.push_i64(SET_Ci(i64, op_AND, i64ecx,0, i64ebx));

                            out.inCmtRegister = true;
                        } else if(operand == "||") {
                            out.inCmtRegister = true;
                            Expression tmp(pAst);
                            pushExpressionToRegister(right, tmp, i64ebx);

                            pushExpressionToRegister(left, out, i64ebx);
                            out.code.push_i64(SET_Ci(i64, op_CMP, i64ebx, 0, 1));
                            out.code.push_i64(SET_Di(i64, op_SKPE, tmp.code.size()));
                            skipAddress.add(out.code.size()-1);
                            out.inject(tmp);
                            out.code.push_i64(SET_Ci(i64, op_CMP, i64ebx, 0, 1));
                        } else if(operand == "|") {
                            pushExpressionToRegister(left, out, i64ebx);
                            out.code.push_i64(SET_Di(i64, op_RSTORE, i64ebx));

                            pushExpressionToRegister(right, out, i64ebx);
                            out.code.push_i64(SET_Di(i64, op_LOADVAL, i64ecx));
                            out.code.push_i64(SET_Ci(i64, op_OR, i64ecx,0, i64ebx));
                            if(recursiveAndExprssions<=1)
                                out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
                            out.inCmtRegister = true;
                        } else if(operand == "&") {
                            pushExpressionToRegister(left, out, i64ebx);
                            out.code.push_i64(SET_Di(i64, op_RSTORE, i64ebx));

                            pushExpressionToRegister(right, out, i64ebx);
                            out.code.push_i64(SET_Di(i64, op_LOADVAL, i64ecx));
                            out.code.push_i64(SET_Ci(i64, op_UAND, i64ecx,0, i64ebx));
                            out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
                            out.inCmtRegister = true;
                        } else if(operand == "^") {
                            pushExpressionToRegister(left, out, i64ebx);
                            out.code.push_i64(SET_Di(i64, op_RSTORE, i64ebx));

                            pushExpressionToRegister(right, out, i64ebx);
                            out.code.push_i64(SET_Di(i64, op_LOADVAL, i64ecx));
                            out.code.push_i64(SET_Ci(i64, op_XOR, i64ecx,0, i64ebx));
                            out.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64cmt));
                            out.inCmtRegister = true;
                        }
                    }
                }

            }else if(right.type == expression_field) {
                if(right.utype.field.isVar()) {
                    // add var
                    goto evaluate;
                } else if(right.utype.field.type == CLASS) {
                    errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                            "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
                } else {
                    // do nothing field unresolved
                }
            } else {
                errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                        "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            }
            break;
        case expression_null:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                    "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            break;
        case expression_field:
            if(left.utype.field.isVar()) {
                // add var
                goto evaluate;
            } else if(left.utype.field.type == CLASS) {
                addClass(operand, left.utype.field.klass, out, left, right, pAst);
            } else {
                // do nothing field unresolved
            }
            break;
        case expression_native:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + left.typeToString() + "`");
            break;
        case expression_lclass:
            addClass(operand, out.utype.klass, out, out, right, pAst);
            break;
        case expression_class:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + left.typeToString() + "`");
            break;
        case expression_void:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                    "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            break;
        case expression_objectclass:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                    "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`. Did you forget to apply a cast? "
                                                                            "i.e ((SomeClass)dynamic_class) " + operand.getToken() + " <data>");
            break;
        case expression_string:
            // TODO: construct new string(<string>) and use that to concatonate strings
            break;
        default:
            break;
    }


    if(recursiveAndExprssions==1) {
        int64_t addr, op, size = out.code.size();
        for(long i = 0; i < skipAddress.size(); i++) {
            addr  = skipAddress.get(i);
            op = GET_OP(out.code.__asm64.get(addr));
            if(op==op_SKNE || op==op_SKPE)
                out.code.__asm64.replace(addr, SET_Di(i64, op, (size-addr)));
            else
                cout << "op " << (Opcode)op << "skipped " << "at addr " << addr << endl;
        }
    }
    out.link = pAst;
    recursiveAndExprssions--;
    return out;
}

Expression RuntimeEngine::parseAssignExpression(Ast* pAst) {
    Expression out(pAst), left(pAst), right(pAst);
    token_entity operand = pAst->getEntity(0);

    if(pAst->getSubAstCount() == 1) {
        // cannot compute unary = <expression>
        errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + operand.getToken() + "`");
        return Expression(pAst);
    }

    left = parseIntermExpression(pAst->getSubAst(0));
    right = parseExpression(pAst->getSubAst(1));

    out.type = expression_var;
    switch(left.type) {
        case expression_var:
            if(operand.getTokenType() == ASSIGN && !left.arrayElement) {
                errors->createNewError(GENERIC, pAst->line, pAst->col, "expression is not assignable");
            } else {
                if(right.type == expression_var) {
                    if(left.arrayElement)
                        assignValue(operand, out, left, right, pAst);
                    else
                        assignNative(operand, out, left, right, pAst);
                }
                else if(right.type == expression_field) {
                    if(right.utype.field.isVar()) {
                        // add var
                        assignValue(operand, out, left, right, pAst);
                    } else if(right.utype.field.type == CLASS) {
                        addClass(operand, right.utype.field.klass, out, left, right, pAst);
                    } else {
                        // do nothing field unresolved
                    }
                } else if(right.type == expression_lclass) {
                    addClass(operand, left.utype.klass, out, left, right, pAst);
                } else {
                    errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                                      "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
                }
            }
            break;
        case expression_null:
            errors->createNewError(GENERIC, pAst->line, pAst->col, "expression is not assignable");
            break;
        case expression_field:
            if(left.utype.field.isVar() || left.utype.field.dynamicObject()) {
                // add var
                assignValue(operand, out, left, right, pAst);
            } else if(left.utype.field.type == CLASS) {
                if(left.arrayElement || !hasOverload(operand, right, left.utype.field.klass, pAst)
                   || right.newExpression)
                    assignValue(operand, out, left, right, pAst);
                else
                    addClass(operand, left.utype.field.klass, out, left, right, pAst);
            } else {
                // do nothing field unresolved
            }
            break;
        case expression_native:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + left.typeToString() + "`");
            break;
        case expression_lclass:
            if(left.arrayElement)
                assignValue(operand, out, left, right, pAst);
            else
                addClass(operand, left.utype.klass, out, left, right, pAst);
            break;
        case expression_class:
            errors->createNewError(UNEXPECTED_SYMBOL, pAst->line, pAst->col, " `" + left.typeToString() + "`");
            break;
        case expression_void:
            errors->createNewError(GENERIC, pAst->line, pAst->col, "expression is not assignable");
            break;
        case expression_objectclass:
            if(left.arrayElement)
                assignValue(operand, out, left, right, pAst);
            else
                errors->createNewError(GENERIC, pAst->line, pAst->col, "expression is not assignable. Did you forget to apply a cast? "
                                                                     "i.e ((SomeClass)dynamic_class) " + operand.getToken() + " <data>");
            break;
        case expression_string:
            errors->createNewError(GENERIC, pAst->line,  pAst->col, "Binary operator `" + operand.getToken() +
                                                              "` cannot be applied to expression of type `" + left.typeToString() + "` and `" + right.typeToString() + "`");
            break;
        default:
            break;
    }

    out.link = pAst;
    return out;
}


Expression RuntimeEngine::parseBinaryExpression(Ast* pAst) {
    switch(pAst->getType()) {
        case ast_add_e:
            return parseAddExpression(pAst);
        case ast_mult_e:
            return parseMultExpression(pAst);
        case ast_shift_e:
            return parseShiftExpression(pAst);
        case ast_less_e:
            return parseLessExpression(pAst);
        case ast_equal_e:
            return parseEqualExpression(pAst);
        case ast_and_e:
            return parseAndExpression(pAst);
        case ast_assign_e:
            return parseAssignExpression(pAst);
        default:
            return Expression(pAst);
    }
}

Expression RuntimeEngine::parseQuesExpression(Ast* pAst) {
    Expression condition(pAst),condIfTrue(pAst), condIfFalse(pAst), expression(pAst), tmp(pAst);

    condition = parseIntermExpression(pAst->getSubAst(0));
    condIfTrue = parseExpression(pAst->getSubAst(1));
    condIfFalse = parseExpression(pAst->getSubAst(2));
    expression = condIfTrue;

    expression.code.__asm64.free();
    pushExpressionToRegister(condition, expression, i64cmt);

    pushExpressionToStack(condIfTrue, tmp); // so we can get accurate size
    expression.code.push_i64(SET_Ci(i64, op_LOADPC_2, i64adx,0, (tmp.code.size() + 5)));
    expression.code.push_i64(SET_Ei(i64, op_IFNE));

    expression.inject(tmp); tmp.free();
    pushExpressionToStack(condIfFalse, tmp); // so we can get accurate size
    expression.code.push_i64(SET_Di(i64, op_MOVI, 1), i64cmt);
    expression.code.push_i64(SET_Di(i64, op_SKPE, tmp.code.size()+1));
    expression.inject(tmp);


    if(equals(condIfTrue, condIfFalse)) {
        if(condIfTrue.type == expression_class) {
            errors->createNewError(GENERIC, condIfTrue.link->line,  condIfTrue.link->col, "Class `" + condIfTrue.typeToString() + "` must be lvalue");
            return condIfTrue;
        }
    }

    if(condIfFalse.type == expression_string) {
        expression.type = expression_var;
        expression.utype.array = true;
    } else if(expression.type == expression_field) {
        switch(expression.utype.field.type) {
            case VAR:
                expression.type=expression_var;
                break;
            case CLASS:
                expression.type=expression_lclass;
                expression.utype.klass=expression.utype.field.klass;
                break;
            case OBJECT:
                expression.type=expression_objectclass;
                break;
        }
        expression.utype.array = expression.utype.field.isArray;
    }

    expression.ifExpression = true;
    expression.newExpression = false;
    expression.func = true;
    return expression;
}

Expression RuntimeEngine::parseExpression(Ast *ast) {
    Expression expression(ast);
    if(ast==NULL||ast->getSubAst(0) == NULL) return expression;
    Ast *encap = ast->getSubAst(0);

    switch (encap->getType()) {
        case ast_primary_expr:
            return parsePrimaryExpression(encap);
        case ast_pre_inc_e:
            return parsePreInc(encap);
        case ast_not_e:
            return parseNotExpression(encap);
        case ast_vect_e:
            errors->createNewError(GENERIC, ast->line, ast->col,
                             "unexpected vector array expression. Did you mean 'new type { <data>, .. }'?");
            expression.link = encap;
            return expression;
        case ast_add_e:
        case ast_mult_e:
        case ast_shift_e:
        case ast_less_e:
        case ast_equal_e:
        case ast_and_e:
        case ast_assign_e:
            return parseBinaryExpression(encap);
        case ast_ques_e:
            return parseQuesExpression(encap);
        default:
            stringstream err;
            err << ": unknown ast type: " << ast->getType();
            errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
            return Expression(encap); // not an expression!
    }
}

Expression RuntimeEngine::parseValue(Ast *ast) {
    return parseExpression(ast->getSubAst(ast_expression));
}

Expression RuntimeEngine::fieldToExpression(Ast *pAst, string name) {
    Expression fieldExpr(pAst);
    KeyPair<int, Field>* field;

    if((field =currentScope()->getLocalField(name)) == NULL)
        return fieldExpr;

    fieldExpr.type = expression_field;
    fieldExpr.utype.field = field->value;
    fieldExpr.utype.type = CLASSFIELD;
    fieldExpr.utype.isField = true;
    fieldExpr.utype.referenceName = field->value.name;
    return fieldExpr;
}

Expression RuntimeEngine::fieldToExpression(Ast *pAst, Field& field) {
    Expression fieldExpr(pAst);

    int64_t i64 = 0;
    fieldExpr.type = expression_field;
    fieldExpr.utype.field = field;
    fieldExpr.utype.isField = true;
    fieldExpr.utype.type = field.type;
    fieldExpr.utype.array = field.isArray;
    fieldExpr.utype.referenceName = field.name;

    if(field.isObjectInMemory()) {
        fieldExpr.code.push_i64(SET_Di(i64, op_MOVL, field.address));
    } else {
        //fieldExpr.code.push_i64(SET_Ci(i64, op_MOVR, i64adx, 0, fp));
        fieldExpr.code.push_i64(SET_Ci(i64, op_LOADL, i64ebx, 0, field.address));
    }
    return fieldExpr;
}

void RuntimeEngine::initalizeNewClass(ClassObject* klass, Expression& out) {
    List<Param> emptyParams;
    Method* fn = klass->getConstructor(emptyParams);

    verifyMethodAccess(fn, out.link);
    out.code.push_i64(SET_Di(i64, op_NEWCLASS, klass->address));

    out.code.push_i64(SET_Di(i64, op_CALL, fn->address));
}

void RuntimeEngine::analyzeVarDecl(Ast *ast) {
    List<AccessModifier> modifiers;
    int startpos=0;
    token_entity operand;

    parseAccessDecl(ast, modifiers, startpos);

    parse_var:
    string name =  ast->getEntity(startpos).getToken();
    Field* field = currentScope()->klass->getField(name);

    if(ast->hasSubAst(ast_value)) {
        Expression expression = parseValue(ast->getSubAst(ast_value)), out(ast);
        Expression fieldExpr = fieldToExpression(ast, *field);
        fieldExpr.code.free();
        equals(fieldExpr, expression);
        operand=ast->getEntity(ast->getEntityCount()-1);

        if(!isFieldInlined(field)) {
            if(field->isStatic()) {
                fieldExpr.code.__asm64.push_back(SET_Di(i64, op_MOVG, field->owner->address));
                fieldExpr.code.__asm64.push_back(SET_Di(i64, op_MOVN, field->address));
            } else {
                fieldExpr.code.__asm64.push_back(SET_Di(i64, op_MOVL, 0));
                fieldExpr.code.__asm64.push_back(SET_Di(i64, op_MOVN, field->address));
            }

            if(field->isConst()) {
                if(expression.literal == true || (expression.type == expression_field && expression.utype.field.isConst())
                        || expression.isEnum() || field->type == CLASS || expression.type == expression_string) {
                    /* good to go */
                } else {
                    errors->createNewError(GENERIC, ast, "constant field cannot be assigned to non-constant expression of type `" + expression.typeToString() + "`");
                }
            }

            if(field->isObjectInMemory()) {
                if(operand == "=") {
                    if(field->type==CLASS && field->klass->getModuleName() == "std" && field->klass->getName() == "string"
                       && expression.type == expression_string && !field->isArray) {
                        constructNewNativeClass("string", "std", expression, out, true);
                        out.inject(fieldExpr);
                        out.code.push_i64(SET_Ei(i64, op_POPOBJ));
                    } else if(field->type==CLASS && field->klass->getModuleName() == "std" &&
                              (field->klass->getName() == "int" || field->klass->getName() == "bool"
                               || field->klass->getName() == "char" || field->klass->getName() == "long"
                               || field->klass->getName() == "short" || field->klass->getName() == "string"
                               || field->klass->getName() == "uchar" || field->klass->getName() == "ulong"
                               || field->klass->getName() == "ushort")
                              && expression.type == expression_var && !field->isArray) {
                        constructNewNativeClass(field->klass->getName(), "std", expression, out, true);
                        out.inject(fieldExpr);
                        out.code.push_i64(SET_Ei(i64, op_POPOBJ));
                    } else
                        assignValue(operand, out, fieldExpr, expression, ast);
                } else {
                    errors->createNewError(GENERIC, ast, " explicit call to operator `" + operand.getToken() + "` without initilization");
                }
            } else {
                assignValue(operand, out, fieldExpr, expression, ast);
            }

            if(field->isStatic()) {
                staticMainInserts.__asm64.appendAll(out.code.__asm64);
            } else {
                /*
                 * We want to inject the value into all constructors
                 */
                for(unsigned int i = 0; i < currentScope()->klass->constructorCount(); i++) {
                    Method* method = currentScope()->klass->getConstructor(i);
                    readjustAddresses(method, out.code.size());
                    method->code.inject(0, out.code);
                }
            }

        }

        if(ast->hasSubAst(ast_var_decl)) {
            ast = ast->getSubAst(ast_var_decl);
            startpos= 0;
            goto parse_var;
        }
    } else {

        if(field->isConst()) {
            errors->createNewError(GENERIC, ast, "constant field requires a value to be provided");
        }
    }

//
//    if(pAst->hasEntity(COMMA)) {
//
//    }
    // TODO: check for value assignment and other vars.
    // if it dosent have a var decl init it to default value in injector (if not in method)
}

void RuntimeEngine::analyzeImportDecl(Ast *pAst) {
    string import = parseModuleName(pAst);
    if(import == currentModule) {
        createNewWarning(REDUNDANT_IMPORT, pAst->line, pAst->col, " '" + import + "'");
    } else {
        if(!modules.find(import)) {
            errors->createNewError(COULD_NOT_RESOLVE, pAst->line, pAst->col,
                             " `" + import + "` ");
        } else {
            List<string> imports;
            imports.push_back(import);
            KeyPair<string, List<string>> resolveMap(activeParser->sourcefile, imports);
            importMap.push_back(resolveMap);
        }
    }
}

void RuntimeEngine::createNewWarning(error_type error, int line, int col, string xcmnts) {
    if(c_options.warnings) {
        if(c_options.werrors){
            errors->createNewError(error, line, col, xcmnts);
        } else {
            errors->createNewWarning(error, line, col, xcmnts);
        }
    }
}

void RuntimeEngine::readjustAddresses(Method *func, unsigned int _offset) {
    if(_offset == 0) return;

    // TODO: do line_table as well
    for(unsigned int i = 0; i < func->exceptions.size(); i++) {
        ExceptionTable &et = func->exceptions.get(i);

        et.start_pc+=_offset;
        et.end_pc+=_offset;
        et.handler_pc+=_offset;
    }
    for(unsigned int i = 0; i < func->finallyBlocks.size(); i++) {
        FinallyTable &ft = func->finallyBlocks.get(i);

        ft.start_pc+=_offset;
        ft.end_pc+=_offset;
        ft.try_start_pc+=_offset;
        ft.try_end_pc+=_offset;
    }

    for(unsigned int i = 0; i < func->line_table.size(); i++) {
        KeyPair<long, int64_t> &lt = func->line_table.get(i);

        lt.value+=_offset;
    }

    int64_t x64, op, addr, reg;
    for(unsigned int i = 0; i < func->code.size(); i++) {

        x64 = func->code.__asm64.get(i);

        switch (op=GET_OP(x64)) {
            case op_JNE:
            case op_JE:
            case op_GOTO:
                addr=GET_Da(x64);

                /*
                 * We only want to update data which is referencing data below us
                 */
                func->code.__asm64.replace(i, SET_Di(x64, op, (addr+_offset)));
                break;
            case op_MOVI:
                if(func->unique_address_table.find(i)) {
                    addr=GET_Da(x64);
                    func->unique_address_table.replace(func->unique_address_table.indexof(i), i+_offset);
                    func->code.__asm64.replace(i, SET_Di(x64, op_MOVI, addr+_offset));

                }
                break;
            case op_LOADPC_2:
                addr=GET_Cb(x64);
                reg=GET_Ca(x64);

                func->code.__asm64.replace(i, SET_Ci(x64, op, reg, 0, addr+_offset));
                break;
        }
    }
}

Method *RuntimeEngine::getMainMethod(Parser *p) {
    string starter_classname = "Runtime";
    string mainMethod = "__srt_init_";
    string setupMethod = "setupClasses";

    ClassObject* StarterClass = getClass("std.kernel", starter_classname, classes);
    if(StarterClass != NULL) {
        List<Param> params;
        List<AccessModifier> modifiers;
        RuntimeNote note = RuntimeNote(p->sourcefile, p->getErrors()->getLine(1), 1, 0);
        params.add(Field(OBJECT, 0, "args", StarterClass, modifiers, note));
        params.get(0).field.isArray = true;

        Method* main = StarterClass->getFunction(mainMethod , params);

        if(main == NULL) {
            errors->createNewError(GENERIC, 1, 0, "could not locate main method '" + mainMethod + "(object[])' in starter class");
        } else {
            if(!main->isStatic()) {
                errors->createNewError(GENERIC, 1, 0, "main method '" + mainMethod + "(object[])' must be static");
            }

            if(!main->hasModifier(PUBLIC)) {
                errors->createNewError(GENERIC, 1, 0, "main method '" + mainMethod + "(object[])' must be public");
            }

            List<Param> empty;
            Method *setupClasses = StarterClass->getFunction(setupMethod, empty);

            if(setupClasses == NULL) {
                errors->createNewError(GENERIC, 1, 0, "could not locate setup method '" + setupMethod + "()' in starter class");
            } else {
                if(!setupClasses->isStatic()) {
                    errors->createNewError(GENERIC, 1, 0, "setup method '" + setupMethod + "()' must be static");
                }

                if(!setupClasses->hasModifier(PRIVATE)) {
                    errors->createNewError(GENERIC, 1, 0, "setup method '" + setupMethod + "()' must be private");
                }

                // we need to look for user main method
                if(mainMethodFound) {
                    Field *userMain;
                    switch(mainSignature) {
                        case 0: { // fn main(string[]) : var;
                            userMain = StarterClass->getField("main");
                            break;
                        }
                        case 1: { // fn main2(string[]);
                            userMain = StarterClass->getField("main2");
                            break;
                        }
                        case 2: { // fn main3();
                            userMain = StarterClass->getField("main3");
                            break;
                        }
                    }

                    if(userMain != NULL) {
                        if(userMain->type == VAR && userMain->isStatic()) {
                            staticMainInserts.push_i64(SET_Di(i64, op_MOVG, StarterClass->address));
                            staticMainInserts.push_i64(SET_Di(i64, op_MOVN, userMain->address));
                            staticMainInserts.push_i64(SET_Di(i64, op_MOVI, 0), i64adx);
                            staticMainInserts.push_i64(SET_Di(i64, op_MOVI, mainAddress), i64ebx); // set main address
                            staticMainInserts.push_i64(SET_Ci(i64, op_RMOV, i64adx, 0, i64ebx));
                        } else
                            errors->createNewError(GENERIC, 1, 0, "main method prototype is invalid in runtime class");
                    } else
                        errors->createNewError(GENERIC, 1, 0, "user main method was not found");
                } else
                    errors->createNewError(GENERIC, 1, 0, "user main method was not found");

                setupClasses->code.inject(setupClasses->code.size()==0 ? 0 : setupClasses->code.size()-1, staticMainInserts);
                staticMainInserts.free();
            }


            RuntimeEngine::main = main;
        }
        return main;
    } else {
        errors->createNewError(GENERIC, 1, 0, "Could not find starter class '" + starter_classname + "' for application entry point.");
        return NULL;
    }
}

void RuntimeEngine::resolveAllMethods() {
    resolvedFields = true;
    /*
     * All fields have been processed, so we are good to fully parse utypes for method params
     */
    resolveAllFields();
    resolvedMethods = true;
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
            SEMTEX_CHECK_ERRORS

            if(i == 0 && ast->getType() == ast_module_decl) {
                add_module(currentModule = parseModuleName(ast));
                imports.push_back(currentModule);
                // add class for global methods
                createGlobalClass();
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
                case ast_interface_decl:
                    parseClassDecl(ast, true);
                    break;
                case ast_generic_class_decl:
                    parseGenericClassDecl(ast, false);
                    break;
                case ast_generic_interface_decl:
                    parseGenericClassDecl(ast, true);
                    break;
                case ast_enum_decl:
                    parseEnumDecl(ast);
                    break;
                case ast_method_decl: /* ignore */
                    break;
                case ast_var_decl:
                    addScope(Scope(CLASS_SCOPE, getClass(currentModule, globalClass, classes)));
                    parseVarDecl(ast, true);
                    removeScope();
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
            report:

            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            success = false;
            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            succeededParsers.addif(activeParser->sourcefile);
            failedParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }



    return success;
}

void RuntimeEngine::createGlobalClass() {
    List<AccessModifier> modifiers;
    modifiers.add(PUBLIC);
    ClassObject * global = getClass(currentModule, globalClass, classes);
    if(global == NULL) {
        global = addGlobalClassObject(globalClass, modifiers, NULL);
        global->address = classSize++;
        stringstream ss;
        ss << currentModule << "#" << global->getName();
        global->setFullName(ss.str());
        addDefaultConstructor(global, NULL);
        globals.add(global); // global class ref
    }

    modifiers.free();
}

void RuntimeEngine::resolveAllInterfaces() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        activeParser = parsers.get(i);
        errors = new ErrorManager(activeParser->lines, activeParser->sourcefile, true, c_options.aggressive_errors);
        currentModule = "$unknown";

        addScope(Scope(GLOBAL_SCOPE, NULL));
        for(int x = 0; x < activeParser->treesize(); x++) {
            Ast* ast = activeParser->ast_at(x);
            SEMTEX_CHECK_ERRORS

            if(x==0) {
                if(ast->getType() == ast_module_decl) {
                    add_module(currentModule = parseModuleName(ast));
                    continue;
                }
            }

            switch(ast->getType()) {
                case ast_interface_decl:
                    resolveInterfaceDecl(ast);
                    break; // TODO: proccess interfaces inside of classes
                default:
                    /* ignore */
                    break;
            }
        }

        if(errors->hasErrors()){
            report:

            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            succeededParsers.addif(activeParser->sourcefile);
            failedParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void RuntimeEngine::resolveAllDelegates() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        activeParser = parsers.get(i);
        errors = new ErrorManager(activeParser->lines, activeParser->sourcefile, true, c_options.aggressive_errors);
        currentModule = "$unknown";

        addScope(Scope(GLOBAL_SCOPE, NULL));
        for(int x = 0; x < activeParser->treesize(); x++) {
            Ast* ast = activeParser->ast_at(x);
            SEMTEX_CHECK_ERRORS

            if(x==0) {
                if(ast->getType() == ast_module_decl) {
                    add_module(currentModule = parseModuleName(ast));
                    continue;
                }
            }

            switch(ast->getType()) {
                case ast_class_decl:
                    resolveClassDeclDelegates(ast);
                    break;
                default:
                    /* ignore */
                    break;
            }
        }

        if(errors->hasErrors()){
            report:

            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            succeededParsers.addif(activeParser->sourcefile);
            failedParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void RuntimeEngine::resolveClassBases() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        activeParser = parsers.get(i);
        errors = new ErrorManager(activeParser->lines, activeParser->sourcefile, true, c_options.aggressive_errors);
        currentModule = "$unknown";

        addScope(Scope(GLOBAL_SCOPE, NULL));
        for(int x = 0; x < activeParser->treesize(); x++) {
            Ast* ast = activeParser->ast_at(x);
            SEMTEX_CHECK_ERRORS

            if(x==0) {
                if(ast->getType() == ast_module_decl) {
                    add_module(currentModule = parseModuleName(ast));
                    continue;
                }
            }

            switch(ast->getType()) {
                case ast_class_decl:
                    resolveClassBase(ast);
                    break;
                case ast_interface_decl:
                    resolveClassDecl(ast, false);
                    break;
                default:
                    /* ignore */
                    break;
            }
        }

        if(errors->hasErrors()){
            report:

            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            succeededParsers.addif(activeParser->sourcefile);
            failedParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void RuntimeEngine::resolveAllFields() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        activeParser = parsers.get(i);
        errors = new ErrorManager(activeParser->lines, activeParser->sourcefile, true, c_options.aggressive_errors);
        currentModule = "$unknown";

        addScope(Scope(GLOBAL_SCOPE, NULL));
        for(int x = 0; x < activeParser->treesize(); x++) {
            Ast* ast = activeParser->ast_at(x);
            SEMTEX_CHECK_ERRORS

            if(x==0) {
                if(ast->getType() == ast_module_decl) {
                    add_module(currentModule = parseModuleName(ast));
                    continue;
                }
            }

            switch(ast->getType()) {
                case ast_class_decl:
                    resolveClassDecl(ast, false);
                    break;
                case ast_interface_decl:
                    resolveClassDecl(ast, false);
                    break;
                case ast_generic_class_decl:
                case ast_generic_interface_decl:
                    if(resolvedFields)
                        resolveGenericClassDecl(ast, true);
                    break;
                case ast_enum_decl: /* ignore */
                    break;
                case ast_var_decl: /* ignore */
                    break;
                case ast_method_decl:
                    if(resolvedFields) {
                        addScope(Scope(CLASS_SCOPE, getClass(currentModule, globalClass, classes)));
                        resolveMethodDecl(ast, true);
                        removeScope();
                    }
                    break;
                default:
                    /* ignore */
                    break;
            }
        }

        if(errors->hasErrors()){
            report:

            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            succeededParsers.addif(activeParser->sourcefile);
            failedParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void RuntimeEngine::resolveAllGlobalFields() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        activeParser = parsers.get(i);
        errors = new ErrorManager(activeParser->lines, activeParser->sourcefile, true, c_options.aggressive_errors);
        currentModule = "$unknown";

        addScope(Scope(GLOBAL_SCOPE, NULL));
        for(int x = 0; x < activeParser->treesize(); x++) {
            Ast* ast = activeParser->ast_at(x);
            SEMTEX_CHECK_ERRORS

            if(x==0) {
                if(ast->getType() == ast_module_decl) {
                    add_module(currentModule = parseModuleName(ast));
                    continue;
                }
            }

            switch(ast->getType()) {
                case ast_class_decl:
                    resolveClassDecl(ast, false);
                    break;
                case ast_interface_decl:
                    resolveClassDecl(ast, false);
                    break;
                case ast_generic_class_decl:
                case ast_generic_interface_decl:
                    if(resolvedFields)
                        resolveGenericClassDecl(ast, true);
                    break;
                case ast_enum_decl: /* ignore */
                    break;
                case ast_var_decl:
                    addScope(Scope(CLASS_SCOPE, getClass(currentModule, globalClass, classes)));
                    resolveVarDecl(ast, false);
                    removeScope();
                    break;
                default:
                    /* ignore */
                    break;
            }
        }

        if(errors->hasErrors()){
            report:

            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            succeededParsers.addif(activeParser->sourcefile);
            failedParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void RuntimeEngine::resolveAllGenerics() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        activeParser = parsers.get(i);
        errors = new ErrorManager(activeParser->lines, activeParser->sourcefile, true, c_options.aggressive_errors);
        currentModule = "$unknown";

        addScope(Scope(GLOBAL_SCOPE, NULL));
        for(int x = 0; x < activeParser->treesize(); x++) {
            Ast* ast = activeParser->ast_at(x);
            SEMTEX_CHECK_ERRORS

            if(x==0) {
                if(ast->getType() == ast_module_decl) {
                    add_module(currentModule = parseModuleName(ast));
                    continue;
                }
            }

            switch(ast->getType()) {
                case ast_class_decl:
                    resolveClassDecl(ast, false);
                    break;
                case ast_interface_decl:
                    break;
                case ast_generic_class_decl:
                case ast_generic_interface_decl:
                    resolveGenericClassDecl(ast, false);
                    break;
                case ast_enum_decl: /* ignore */
                    break;
                case ast_method_decl: /* ignore */
                    break;
                case ast_var_decl: /* ignore */
                    break;
                default:
                    /* ignore */
                    break;
            }
        }

        if(errors->hasErrors()){
            report:

            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            succeededParsers.addif(activeParser->sourcefile);
            failedParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }

    resolvedGenerics = true;
}

void RuntimeEngine::inlineFields() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        activeParser = parsers.get(i);
        errors = new ErrorManager(activeParser->lines, activeParser->sourcefile, true, c_options.aggressive_errors);
        currentModule = "$unknown";

        addScope(Scope(GLOBAL_SCOPE, NULL));
        for(int x = 0; x < activeParser->treesize(); x++) {
            Ast* ast = activeParser->ast_at(x);
            SEMTEX_CHECK_ERRORS

            if(x==0) {
                if(ast->getType() == ast_module_decl) {
                    add_module(currentModule = parseModuleName(ast));
                    continue;
                }
            }

            switch(ast->getType()) {
                case ast_class_decl:
                    resolveClassDecl(ast, true);
                    break;
                case ast_generic_class_decl:
                    resolveGenericClassDecl(ast, true);
                    break;
                case ast_var_decl:
                    addScope(Scope(CLASS_SCOPE, getClass(currentModule, globalClass, classes)));
                    resolveVarDecl(ast, true);
                    removeScope();
                    break;
                default:
                    /* ignore */
                    break;
            }
        }

        if(errors->hasErrors()){
            report:

            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            succeededParsers.addif(activeParser->sourcefile);
            failedParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

ReferencePointer RuntimeEngine::parseReferencePtr(Ast *ast, bool getAst) {
    if(getAst)
        ast = ast->getSubAst(ast_refrence_pointer);
    bool hashfound = false, last, hash = ast->hasEntity(HASH);
    string id="";
    ReferencePointer ptr;
    bool resolveParents = ast->hasSubAst(ast_utype_list), failed = false;
    long idx = 0;
    ClassObject *parent = NULL;


    for(long i = 0; i < ast->getEntityCount(); i++) {
        id = ast->getEntity(i).getToken();
        last = i + 1 >= ast->getEntityCount();

        if(id == ".")
            continue;
        else if(id == "#") {
            hashfound = true;
            continue;
        }

        if(!failed && resolvedGenerics && resolvedMethods && ast->hasSubAst(ast_utype_list) && ast->getSubAst(idx++)->getType() == ast_utype_list){
            List<Expression> utypes;
            parseUtypeList(ast->getSubAst(idx-1), utypes);
            findAndCreateGenericClass(ptr.module, id, utypes, parent, ast);
        }

        if(hash && !hashfound && !last) {
            if(ptr.module == "")
                ptr.module =id;
            else
                ptr.module += "." + id;
        } else if(!last) {
            ptr.classHeiarchy.push_back(id);

            if(resolveParents) {
                if(parent == NULL)
                    parent = tryClassResolve(ptr.module, id, ast);
                else {
                    parent = parent->getChildClass(id);
                    failed = parent == NULL;
                }
            }
        } else {
            ptr.referenceName = id;
        }
    }

    return ptr;
}

ClassObject* RuntimeEngine::tryClassResolve(string moduleName, string name, Ast* pAst) {
    ClassObject* klass = NULL;
    if(!moduleName.empty() && (klass = getClass(moduleName, name, classes)) != NULL) {
        verifyClassAccess(klass, pAst);
        return klass;
    } else {
        for (unsigned int i = 0; i < importMap.size(); i++) {
            if (importMap.get(i).key == activeParser->sourcefile) {

                List<string> &lst = importMap.get(i).value;
                for (unsigned int x = 0; x < lst.size(); x++) {
                    if ((klass = getClass(lst.get(x), name, classes)) != NULL) {
                        verifyClassAccess(klass, pAst);
                        return klass;
                    }
                }

                break;
            }
        }

        return klass;
    }
}

ResolvedReference RuntimeEngine::resolveReferencePointer(ReferencePointer &ptr, Ast* pAst) {
    ResolvedReference reference;

    if(ptr.classHeiarchy.size() == 0) {
        ClassObject* klass = tryClassResolve(ptr.module, ptr.referenceName, pAst);
        if(klass == NULL) {
            reference.type = UNDEFINED;
            reference.referenceName = ptr.referenceName;
        } else {
            reference.type = CLASS;
            reference.klass = klass;
            reference.resolved = true;
        }
    } else {
        ClassObject* klass = tryClassResolve(ptr.module, ptr.classHeiarchy.get(0), pAst);
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
    ResolvedReference resolvedRefrence = resolveReferencePointer(ptr, ast);
    ast = ast->getType() == ast_refrence_pointer ? ast : ast->getSubAst(ast_refrence_pointer);

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
    ClassObject* klass=NULL;

    if(startpos >= ast->getEntityCount()) {
        return NULL;
    } else {
        ReferencePointer ptr = parseReferencePtr(ast);
        klass = resolveClassRefrence(ast, ptr);

        if(klass != NULL) {
            if((currentScope()->klass->getHeadClass() != NULL && currentScope()->klass->getHeadClass()->isCurcular(klass)) ||
               currentScope()->klass->match(klass) || klass->match(currentScope()->klass->getHeadClass())) {
                errors->createNewError(GENERIC, ast->getSubAst(0)->line, ast->getSubAst(0)->col,
                                 "cyclic dependency of class `" + ptr.referenceName + "` in parent class `" + currentScope()->klass->getName() + "`");
            }
        }
    }

    return klass;
}

string Expression::typeToString() {
    string s = "";
    switch(type) {
        case expression_string:
            return "var[]";
        case expression_unresolved:
            return "?";
        case expression_var:
            return string("var") + (utype.array ? "[]" : "");
        case expression_lclass:
            return utype.typeToString() + (utype.array ? "[]" : "");
        case expression_native:
            return utype.typeToString() + (utype.array ? "[]" : "");
        case expression_unknown:
            return "?";
        case expression_prototype:
            return utype.method->getFullName();
        case expression_class:
            return utype.typeToString() + (utype.array ? "[]" : "");
        case expression_void:
            return "void";
        case expression_objectclass:
            return s + "object" + (utype.array ? "[]" : "");
        case expression_field:
            return utype.typeToString() + (utype.array || utype.field.isArray ? "[]" : "");
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
    this->charLiteral=expression.charLiteral;
    this->link = expression.link;
    this->utype = expression.utype;
    this->value = expression.value;
    this->arrayElement = expression.arrayElement;
    this->inCmtRegister=expression.inCmtRegister;
    this->ifExpression=expression.ifExpression;
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
    if(field == NULL || !field->isStatic())
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
        expression.code.push_i64(SET_Di(i64, op_MOVI, value), i64ebx);
    else {
        expression.code.push_i64(SET_Di(i64, op_MOVBI, ((int64_t)value)), abs(get_low_bytes(value)));
        expression.code.push_i64(SET_Ci(i64, op_MOVR, i64ebx,0, i64bmr));
    }

    expression.type=expression_var;
    expression.literal=true;
    expression.intValue=value;
}

bool RuntimeEngine::isDClassNumberEncodable(double var) { return ((int64_t )var > DA_MAX || (int64_t )var < DA_MIN) == false; }

void RuntimeEngine::resolveClassHeiarchy(ClassObject* klass, ReferencePointer& refrence, Expression& expression, Ast* pAst, bool requreMovg, bool requireStatic) {
    int64_t i64;
    string object_name = "";
    Field* field = NULL;
    ClassObject* k;
    bool lastRefrence = false;
    long refrenceTrys = 0;

    for(unsigned int i = 1; i < refrence.classHeiarchy.size()+1; i++) {
        refrenceTrys++;
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

                verifyFieldAccess(field, pAst);
                if(requreMovg && refrenceTrys <= 1) {
                    if(field->isStatic())
                        expression.code.push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                    else
                        expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                }
                // for now we are just generating code for x.x.f not Main.x...thats static access
                if(isFieldInlined(field) && !field->isEnum) {
                    inlineVariableValue(expression, field);
                } else
                    expression.code.push_i64(SET_Di(i64, op_MOVN, field->address));

                if(lastRefrence) {
                    expression.utype.type = CLASSFIELD;
                    expression.utype.field = *field;
                    expression.utype.isField = true;
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
                        requireStatic = false;
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
            expression.code.push_i64(SET_Di(i64, op_MOVG, k->address));
            klass = k;

            if(lastRefrence) {
                expression.utype.type = CLASS;
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
            verifyFieldAccess(field, pAst);
            resolveClassHeiarchy(field->klass, refrence, expression, pAst, false, false);
            return;
    }
}

void RuntimeEngine::resolveSelfUtype(Scope* scope, ReferencePointer& reference, Expression& expression, Ast* ast) {
    if(reference.singleRefrence()) {
        ClassObject* klass=NULL;
        Field* field=NULL;

        if(currentScope()->type == GLOBAL_SCOPE) {
            /* cannot get self from global refrence */
            errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line,
                             ast->getSubAst(ast_type_identifier)->col,
                             "cannot get object `" + reference.referenceName + "` from self at global scope");

            expression.utype.type = UNDEFINED;
            expression.utype.referenceName = reference.referenceName;
            expression.type = expression_unresolved;
        } else {
            if(currentScope()->type == STATIC_BLOCK) {
                errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, "cannot get object `"
                                                + reference.referenceName + "` from self in static context");
            }

            if((field = currentScope()->klass->getField(reference.referenceName)) != NULL) {
                // field?
                expression.utype.type = CLASSFIELD;
                expression.utype.field = *field;
                expression.utype.isField = true;
                expression.type = expression_field;

                if(isFieldInlined(field)) {
                    inlineVariableValue(expression, field);
                } else {
                    if(field->isStatic())
                        expression.code.push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                    else
                        expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                    expression.code.push_i64(SET_Di(i64, op_MOVN, field->address));
                }
            } else {
                /* Un resolvable */
                errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " `" + reference.referenceName + "` " +
                                                          (reference.module.empty() ? "" : "in module {" + reference.module + "} "));

                expression.utype.type = UNDEFINED;
                expression.utype.referenceName = reference.referenceName;
                expression.type = expression_unresolved;
            }
        }
    } else if(reference.singleRefrenceModule()) {
        /* Un resolvable */
        errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " use of module {"
                                        + reference.module + "} in expression signifies global access of object");

        expression.utype.type = UNDEFINED;
        expression.utype.referenceName = reference.referenceName;
        expression.type = expression_unresolved;
    } else {
        /* field? or class? */
        ClassObject* klass=NULL;
        Field* field=NULL;
        string starter_name = reference.classHeiarchy.at(0);

        if(currentScope()->type == GLOBAL_SCOPE) {
            /* cannot get self from global refrence */
            errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, "cannot get object `"
                                            + starter_name + "` from instance at global scope");

            expression.utype.type = UNDEFINED;
            expression.utype.referenceName = reference.referenceName;
            expression.type = expression_unresolved;
        } else {
            if(reference.module != "") {
                /* Un resolvable */
                errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " use of module {" + reference.module + "} in expression signifies global access of object");

                expression.utype.type = UNDEFINED;
                expression.utype.referenceName = reference.referenceName;
                expression.type = expression_unresolved;
                return;
            }

            if(currentScope()->type == STATIC_BLOCK) {
                errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, "cannot get object `" + starter_name + "` from self in static context");

                expression.utype.type = UNDEFINED;
                expression.utype.referenceName = reference.referenceName;
                expression.type = expression_unresolved;
            } else {

                if((field = currentScope()->klass->getField(starter_name)) != NULL) {
                    if(field->isStatic())
                        expression.code.push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                    else
                        expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                    expression.code.push_i64(SET_Di(i64, op_MOVN, field->address));

                    resolveFieldHeiarchy(field, reference, expression, ast);
                } else {
                    /* Un resolvable */
                    errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " `" + reference.referenceName + "` " +
                                                              (reference.module == "" ? "" : "in module {" + reference.module + "} "));

                    expression.utype.type = UNDEFINED;
                    expression.utype.referenceName = reference.referenceName;
                    expression.type = expression_unresolved;
                }
            }
        }
    }
}

ResolvedReference RuntimeEngine::getBaseClassOrField(string name, ClassObject* start, Ast *pAst) {
    ClassObject* base;
    Field* field;
    ResolvedReference reference;

    for(;;) {
        base = start->getBaseClass();

        if(base == NULL) {
            // could not resolve
            return reference;
        }

        if(name == base->getName()) {
            reference.klass = base;
            reference.type = CLASS;
            return reference;
            // base class
        } else if((field = base->getField(name))) {
            verifyFieldAccess(field, pAst);
            reference.field = *field;
            reference.isField = true;
            reference.type = CLASSFIELD;
            return reference;
        } else {
            start = base; // recursivley assign klass to new base
        }
    }
}

void RuntimeEngine::resolveBaseUtype(Scope* scope, ReferencePointer& reference, Expression& expression, Ast* ast) {
    ClassObject* klass = currentScope()->klass, *base;
    Field* field;
    ResolvedReference ref;

    if(currentScope()->klass->getBaseClass() == NULL) {
        errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, "class `" + currentScope()->klass->getFullName() + "` does not inherit a base class");
        expression.utype.type = UNDEFINED;
        expression.utype.referenceName = reference.toString();
        expression.type = expression_unresolved;
        return;
    }

    if(reference.singleRefrence()) {
        ref = getBaseClassOrField(reference.referenceName, klass, ast);
        if(ref.type != UNDEFINED) {
            if(ref.type == CLASSFIELD) {
                // field!
                expression.utype.type = CLASSFIELD;
                expression.utype.field = ref.field;
                expression.utype.isField = true;
                expression.type = expression_field;

                if(expression.utype.field.isStatic())
                    expression.code.push_i64(SET_Di(i64, op_MOVG, expression.utype.field.owner->address));
                else
                    expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                expression.code.push_i64(SET_Di(i64, op_MOVN, expression.utype.field.address));
            } else {
                if(currentScope()->type == STATIC_BLOCK) {
                    errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, "cannot get object `" + ref.klass->getName()
                                                                                                                                         + "` from instance in static context");
                }

                if(currentScope()->klass->hasBaseClass(ref.klass)) {
                    // base class
                    expression.utype.type = CLASS;
                    expression.utype.klass = ref.klass;
                    expression.type = expression_class;

                    expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                } else {
                    // klass provided is not a base
                    errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, "class `" + reference.referenceName + "`" +
                                                                                                                                     (reference.module == "" ? "" : " in module {" + reference.module + "}")
                                                                                                                                     + " is not a base class of class " + currentScope()->klass->getName());
                    expression.utype.type = UNDEFINED;
                    expression.utype.referenceName = reference.toString();
                    expression.type = expression_unresolved;
                }
            }
        } else {
            errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " `" + reference.referenceName + "` " +
                                                      (reference.module == "" ? "" : "in module {" + reference.module + "} "));
            expression.utype.type = UNDEFINED;
            expression.utype.referenceName = reference.toString();
            expression.type = expression_unresolved;
        }
    } else if(reference.singleRefrenceModule()) {
        base = tryClassResolve(reference.module, reference.referenceName, ast);

        if(base != NULL) {
            if(currentScope()->klass->hasBaseClass(base)) {
                // base class

                expression.utype.type = CLASS;
                expression.utype.klass = base;
                expression.type = expression_class;
                expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                return;
            } else {
                // klass provided is not a base
                errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, "class `" + reference.referenceName + "`" +
                                                                                                                                 (reference.module == "" ? "" : " in module {" + reference.module + "}")
                                                                                                                                 + " is not a base class of class " + currentScope()->klass->getName());
                expression.utype.type = UNDEFINED;
                expression.utype.referenceName = reference.toString();
                expression.type = expression_unresolved;
            }
        }

        /* Un resolvable */

        errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " `" + reference.referenceName + "` " +
                                                  (reference.module == "" ? "" : "in module {" + reference.module + "} "));
        expression.utype.type = UNDEFINED;
        expression.utype.referenceName = reference.referenceName;
        expression.type = expression_unresolved;
        return;
    } else {
        base = klass->getBaseClass();
        string starter_name = reference.classHeiarchy.at(0);

        if(base != NULL) {

            if(reference.module != "") {
                // first must be class
                if((klass = tryClassResolve(reference.module, starter_name, ast)) != NULL) {
                    if(currentScope()->klass->hasBaseClass(klass)) {
                        resolveClassHeiarchy(klass, reference, expression, ast);
                    } else {
                        // klass provided is not a base
                        errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, "class `" + starter_name + "`" +
                                                                                                                                         (reference.module == "" ? "" : " in module {" + reference.module + "}")
                                                                                                                                         + " is not a base class of class " + currentScope()->klass->getName());
                        expression.utype.type = UNDEFINED;
                        expression.utype.referenceName = reference.toString();
                        expression.type = expression_unresolved;
                    }
                    return;
                } else {
                    errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " `" + starter_name + "` " +
                                                              (reference.module == "" ? "" : "in module {" + reference.module + "} "));
                    expression.utype.type = UNDEFINED;
                    expression.utype.referenceName = reference.toString();
                    expression.type = expression_unresolved;
                }
            } else {
                ref = getBaseClassOrField(starter_name, klass, ast);
                if(ref.type != UNDEFINED) {
                    if(ref.type == CLASSFIELD) {

                        if(ref.field.isStatic())
                            expression.code.push_i64(SET_Di(i64, op_MOVG, ref.field.owner->address));
                        else
                            expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                        expression.code.push_i64(SET_Di(i64, op_MOVN, ref.field.address)); // gain access to field in object

                        resolveFieldHeiarchy(&ref.field, reference, expression, ast);
                    } else {

                        resolveClassHeiarchy(ref.klass, reference, expression, ast);
                    }
                } else {
                    if((klass = getClass(reference.module, starter_name, classes)) != NULL) {
                        if(currentScope()->klass->hasBaseClass(klass)) {

                            resolveClassHeiarchy(klass, reference, expression, ast);
                        } else {
                            errors->createNewError(GENERIC, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, "class `" + starter_name + "`" +
                                                                                                                                             (reference.module == "" ? "" : " in module {" + reference.module + "}")
                                                                                                                                             + " is not a base class of class " + currentScope()->klass->getName());
                            expression.utype.type = UNDEFINED;
                            expression.utype.referenceName = reference.toString();
                            expression.type = expression_unresolved;
                        }
                    } else {
                        errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " `" + starter_name + "` " +
                                                                  (reference.module == "" ? "" : "in module {" + reference.module + "} "));
                        expression.utype.type = UNDEFINED;
                        expression.utype.referenceName = reference.toString();
                        expression.type = expression_unresolved;
                    }
                }
            }
        }else {
            errors->createNewError(COULD_NOT_RESOLVE, ast->getSubAst(ast_type_identifier)->line, ast->getSubAst(ast_type_identifier)->col, " `" + starter_name + "` " +
                                                      (reference.module == "" ? "" : "in module {" + reference.module + "} "));
            expression.utype.type = UNDEFINED;
            expression.utype.referenceName = reference.toString();
            expression.type = expression_unresolved;
        }
    }
}

void RuntimeEngine::resolveUtype(ReferencePointer& refrence, Expression& expression, Ast* pAst) {
    int64_t i64;

    expression.link = pAst;
    if(currentScope()->self) {
        resolveSelfUtype(currentScope(), refrence, expression, pAst);
    } else if(currentScope()->base) {
        resolveBaseUtype(currentScope(), refrence, expression, pAst);
    } else {
        if(refrence.singleRefrence()) {
            ClassObject* klass=NULL;
            Field* field=NULL;
            Method* fn = NULL;
            bool ambiguous = false;

            if(currentScope()->type == GLOBAL_SCOPE) {

                if((klass = tryClassResolve(refrence.module, refrence.referenceName, pAst)) != NULL) {
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
                if(currentScope()->type != CLASS_SCOPE && currentScope()->getLocalField(refrence.referenceName) != NULL) {
                    field = &currentScope()->getLocalField(refrence.referenceName)->value;
                    expression.utype.type = CLASSFIELD;
                    expression.utype.field = *field;
                    expression.utype.isField = true;
                    expression.type = expression_field;

                    if(field->isVar()) {
                        if(field->isObjectInMemory()) {
                            expression.code.push_i64(SET_Di(i64, op_MOVL, field->address));
                        } else
                            expression.code.push_i64(SET_Ci(i64, op_LOADL, i64ebx, 0, field->address));
                    }
                    else
                        expression.code.push_i64(SET_Di(i64, op_MOVL, field->address));
                }
                else if((field = currentScope()->klass->getField(refrence.referenceName, true)) != NULL) {
                    // field?
                    if(currentScope()->type == STATIC_BLOCK && !field->isStatic()) {
                        errors->createNewError(GENERIC, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col,
                                         "cannot get object `" + refrence.referenceName + "` from self in static context");
                    }

                    verifyFieldAccess(field, pAst);
                    expression.utype.type = CLASSFIELD;
                    expression.utype.field = *field;
                    expression.utype.isField = true;
                    expression.type = expression_field;

                    if(isFieldInlined(field)) {
                        inlineVariableValue(expression, field);
                    } else {
                        if(field->isStatic())
                            expression.code.push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                        else
                            expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                        expression.code.push_i64(SET_Di(i64, op_MOVN, field->address));
                    }
                } else if((fn = currentScope()->klass->getFunctionByName(refrence.referenceName, ambiguous)) != NULL) {
                    if(ambiguous)
                        createNewWarning(GENERIC, pAst->line, pAst->col, "reference to function name is ambiguous");
                    expression.utype.type = VAR;
                    expression.utype.method = fn;
                    expression.utype.isMethod = true;
                    expression.type = expression_prototype;
                    if(!fn->isStatic()) {
                        errors->createNewError(GENERIC, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, " function pointer `" + refrence.referenceName + "` " +
                                                        (refrence.module == "" ? "" : "in module {" + refrence.module + "} ") + " must be static");
                    }
                    expression.code.push_i64(SET_Di(i64, op_MOVI, fn->address), i64ebx);
                } else {
                    if((klass = tryClassResolve(refrence.module, refrence.referenceName, pAst)) != NULL) {
                        // global class ?
                        expression.utype.type = CLASS;
                        expression.utype.klass = klass;
                        expression.type = expression_class;
                    } else if((klass = currentScope()->klass->getChildClass(refrence.referenceName)) != NULL && !klass->isGeneric()) {
                        // global class ?
                        expression.utype.type = CLASS;
                        expression.utype.klass = klass;
                        expression.type = expression_class;
                    } else {
                        if(currentScope()->klass->isGeneric() && !currentScope()->klass->isProcessed()) {
                            if((klass = getClass(refrence.module, refrence.referenceName, generics)) != NULL) {
                                expression.utype.type = TYPEGENERIC;
                                expression.utype.klass = klass;
                                expression.type = expression_generic;
                                return;
                            }
                        }

                        if((klass = getClass(refrence.module, refrence.referenceName, generics)) != NULL) {
                            if(!resolvedMethods) {
                                expression.utype.type = TYPEGENERIC;
                                expression.utype.klass = klass;
                                expression.type = expression_generic;
                                return;
                            }
                            expression.utype.type = UNDEFINED;
                            expression.utype.referenceName = refrence.referenceName;
                            expression.type = expression_unresolved;

                            stringstream helpfulMessage;
                            helpfulMessage << "have you forgotten your type parameters? Were you possibly looking for class `"
                                              << klass->getFullName() << "` it requires " << klass->genericKeySize() << " generic types.";
                            errors->createNewError(COULD_NOT_RESOLVE, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, " `" + refrence.referenceName + "` " +
                                                                                                                                                             (refrence.module == "" ? "" : "in module {" + refrence.module + "} ")
                                                                                                                                                             + helpfulMessage.str());
                            return;
                        }

                        if((field = getGlobalField(refrence.referenceName)) != NULL) {
                            verifyFieldAccess(field, pAst);
                            expression.utype.type = CLASSFIELD;
                            expression.utype.field = *field;
                            expression.utype.isField = true;
                            expression.type = expression_field;

                            if(isFieldInlined(field)) {
                                inlineVariableValue(expression, field);
                            } else {
                                if(field->isStatic())
                                    expression.code.push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                                else
                                    expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                                expression.code.push_i64(SET_Di(i64, op_MOVN, field->address));
                            }

                            return;
                        } else if((field = getEnum(refrence.referenceName)) != NULL) {
                            verifyFieldAccess(field, pAst);
                            expression.utype.type = CLASSFIELD;
                            expression.utype.field = *field;
                            expression.utype.isField = true;
                            expression.type = expression_field;

                            if(field->isStatic())
                                expression.code.push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                            else
                                expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                            expression.code.push_i64(SET_Di(i64, op_MOVN, field->address));

                            return;
                        }
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
            if((klass = tryClassResolve(refrence.module, refrence.referenceName, pAst)) != NULL) {
                expression.utype.type = CLASS;
                expression.utype.klass = klass;
                expression.type = expression_class;
            } else {
                Method *fn;
                bool ambiguous = false;
                ClassObject* global = getClass(refrence.module, globalClass, globals);
                if(global != NULL && (fn = global->getFunctionByName(refrence.referenceName, ambiguous)) != NULL) {
                    if(ambiguous)
                        createNewWarning(GENERIC, pAst->line, pAst->col, "reference to function name is ambiguous");
                    expression.utype.type = VAR;
                    expression.utype.method = fn;
                    expression.utype.isMethod = true;
                    expression.type = expression_prototype;
                    if(!fn->isStatic()) {
                        errors->createNewError(GENERIC, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, " function pointer `" + refrence.referenceName + "` " +
                                                                                                                                               (refrence.module == "" ? "" : "in module {" + refrence.module + "} ") + " must be static");
                    }
                    expression.code.push_i64(SET_Di(i64, op_MOVI, fn->address), i64ebx);
                    return;
                }

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

            if(currentScope()->type == GLOBAL_SCOPE) {

                // class?
                if((klass = tryClassResolve(refrence.module, starter_name, pAst)) != NULL) {
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

                ReferencePointer p;
                Method* fn = NULL;
                bool ambiguous = false;

                p = refrence;
                string methodName = p.referenceName;

                if(splitMethodUtype(p.referenceName, p)) {
                    // accessor
                    resolveUtype(p, expression, pAst);

                    if(expression.type == expression_class || (expression.type == expression_field && expression.utype.field.type == CLASS)) {
                        klass = expression.type == expression_class ? expression.utype.klass : expression.utype.field.klass;
                        if((fn = klass->getFunctionByName(refrence.referenceName, ambiguous)) != NULL) {
                            if(ambiguous)
                                createNewWarning(GENERIC, pAst->line, pAst->col, "reference to function name is ambiguous");
                            expression.utype.type = VAR;
                            expression.utype.method = fn;
                            expression.utype.isMethod = true;
                            expression.type = expression_prototype;
                            if(!fn->isStatic()) {
                                errors->createNewError(GENERIC, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col, " function pointer `" + refrence.referenceName + "` " +
                                                                                                                                                       (refrence.module == "" ? "" : "in module {" + refrence.module + "} ") + " must be static");
                            }
                            expression.code.push_i64(SET_Di(i64, op_MOVI, fn->address), i64ebx);
                            return;
                        }
                    }
                }
                // scope_class? | scope_instance_block? | scope_static_block?
                if(refrence.module != "") {
                    if((klass = tryClassResolve(refrence.module, starter_name, pAst)) != NULL) {
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

                if(currentScope()->type != CLASS_SCOPE && currentScope()->getLocalField(starter_name) != NULL) {
                    field = &currentScope()->getLocalField(starter_name)->value;

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
                else if((field = currentScope()->klass->getField(starter_name, true)) != NULL) {
                    if(currentScope()->type == STATIC_BLOCK) {
                        errors->createNewError(GENERIC, pAst->getSubAst(ast_type_identifier)->line, pAst->getSubAst(ast_type_identifier)->col,
                                               "cannot get object `" + starter_name + "` from self in static context");
                    }

                    verifyFieldAccess(field, pAst);
                    if(field->isStatic())
                        expression.code.push_i64(SET_Di(i64, op_MOVG, field->owner->address));
                    else
                        expression.code.push_i64(SET_Di(i64, op_MOVL, 0));
                    expression.code.push_i64(SET_Di(i64, op_MOVN, field->address));
                    resolveFieldHeiarchy(field, refrence, expression, pAst);
                    return;
                } else {
                    if((klass = tryClassResolve(refrence.module, starter_name, pAst)) != NULL) {
                        resolveClassHeiarchy(klass, refrence, expression, pAst);
                        return;
                    } else if((klass = currentScope()->klass->getChildClass(starter_name)) != NULL) {
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
    ast = ast->getType() == ast_utype ? ast : ast->getSubAst(ast_utype);

    ReferencePointer ptr=parseTypeIdentifier(ast);
    Expression expression(ast);

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
    } else if(ptr.singleRefrence() && currentScope()->klass->isGeneric() && currentScope()->klass->hasGenericKey(ptr.referenceName)) {
        if(currentScope()->klass->isProcessed()) {
            expression = *currentScope()->klass->getGenericType(ptr.referenceName);
            bool isArray = false;
            if(ast->hasEntity(LEFTBRACE) && ast->hasEntity(RIGHTBRACE)) {
                isArray = true;
            }

            if(expression.isArray() && !isArray) {
                // we are fine field will just stay an array
            } else if(expression.isArray() && isArray) {
                // error
                errors->createNewError(GENERIC, ast, "Array-arrays are not supported.");
            } else if(isArray) {
                expression.utype.array = true;
            }

            expression.link = ast;
            ptr.free();
            return expression;
        }

        expression.type = expression_generic;
        expression.utype.type = TYPEGENERIC;
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

void RuntimeEngine::resolveVarDecl(Ast* ast, bool inlineField) {
    List<AccessModifier> modifiers;
    int startpos=0;

    parseAccessDecl(ast, modifiers, startpos);

    Expression expression;

    if(resolvedFields)
        expression = parseUtype(ast);
    parse_var:
    string name =  ast->getEntity(startpos).getToken();
    Field* field = currentScope()->klass->getField(name);
    if(expression.utype.type == CLASS || expression.utype.type==CLASSFIELD) {
        field->klass = expression.utype.klass;
        field->type = CLASS;
    } else if(expression.utype.type == VAR) {
        field->type = VAR;
    } else if(expression.utype.type == TYPEGENERIC) {
        field->type = TYPEGENERIC;
        field->key = expression.utype.referenceName;
        field->klass = expression.utype.klass;
        field->ast = ast;
    } else if(expression.utype.type == OBJECT) {
        field->type = OBJECT;
    } else {
        field->type = UNDEFINED;
    }

    field->isArray = expression.utype.array;
    field->owner = currentScope()->klass;
    field->address = field->owner->getFieldAddress(field);

    if(inlineField && ast->hasSubAst(ast_value)) {
        Expression expr = parseValue(ast->getSubAst(ast_value)), out(ast);

        if (field->isConst() && field->isVar() && !field->isArray && expr.literal) {
            // inline local static variables
            inline_map.add(KeyPair<string, double>(field->fullName, expr.intValue));
        }
    }

    if(ast->hasSubAst(ast_var_decl)) {
        startpos = 0;
        ast = ast->getSubAst(ast_var_decl);
        goto parse_var;
    }
}

void RuntimeEngine::resolvePrototypeDecl(Ast* ast) {
    List<AccessModifier> modifiers;
    int startpos=0;

    parseAccessDecl(ast, modifiers, startpos);

    if(modifiers.find(mCONST)) {
        errors->createNewError(GENERIC, ast, "modifier `const` is not allowed for function protypes");
    }

    string name =  ast->getEntity(++startpos).getToken();
    Field* field = currentScope()->klass->getField(name);
    parseFuncPrototype(ast, field);

    field->owner = currentScope()->klass;
    field->address = field->owner->getFieldAddress(field);
}

void RuntimeEngine::parseFuncPrototype(Ast *ast, Field *field) {
    field->prototype=true;
    field->type = VAR;

    List<Param> params;
    parseMethodParams(params, parseUtypeArgList(ast->getSubAst(ast_utype_arg_list_opt)), ast->getSubAst(ast_utype_arg_list_opt));

    field->params.addAll(params);
    Expression utype(ast);
    if(ast->hasSubAst(ast_method_return_type)) {
        utype = parseUtype(ast->getSubAst(ast_method_return_type));
        parseFieldReturnType(utype, *field);
    } else
        field->returnType = TYPEVOID;
}

void RuntimeEngine::resolveEnumVarDecl(Ast* ast) {
    List<AccessModifier> modifiers;
    int startpos=0;

    parseAccessDecl(ast, modifiers, startpos);

    string name =  ast->getEntity(startpos).getToken();
    Field* field = currentScope()->klass->getField(name);

    field->klass = currentScope()->klass;
    field->type = CLASS;

    field->isEnum = true;
    field->owner = currentScope()->klass;
    field->address = field->owner->getFieldAddress(field);

    if(ast->hasSubAst(ast_value)) {
        Expression expr = parseValue(ast->getSubAst(ast_value)), out(ast);

        if(!isWholeNumber(constantExpressionToValue(ast, expr))) {
            errors->createNewError(GENERIC, ast, "enum value must evaluate to an integer, are you missing a cast?");
        }

        if (expr.literal) {
            // inline local static variables
            inline_map.add(KeyPair<string, double>(field->fullName, expr.intValue));
            currentScope()->klass->enumValue = (long)expr.intValue+1;

            assignEnumName(ast, field, out);
            assignEnumValue(ast, field, expr, out);
        } else {
            if(!expr.isConstExpr()) {
                errors->createNewError(GENERIC, ast, "the expression being assigned to enum `" + field->fullName + "` ust be constant");
            }
            if(isFieldInlined(&expr.utype.field)) {
                inline_map.add(KeyPair<string, double>(field->fullName, getInlinedFieldValue(&expr.utype.field)));
            }

            assignEnumName(ast, field, out);
            assignEnumValue(ast, field, expr, out);
        }
    } else {
        Expression expr(ast), out(ast);
        expr.type = expression_var;
        expr.literal = true;
        expr.intValue = currentScope()->klass->enumValue;
        expr.code.push_i64(SET_Di(i64, op_MOVI, expr.intValue), i64ebx);

        inline_map.add(KeyPair<string, double>(field->fullName, currentScope()->klass->enumValue++));

        assignEnumName(ast, field, out);
        assignEnumValue(ast, field, expr, out);
    }
}

void RuntimeEngine::assignEnumValue(Ast *ast, Field *field, Expression &expr, Expression &out) {
    Field *valueField = field->klass->getField("value", true);
    if(valueField != NULL) {
        Expression valueExpr = fieldToExpression(ast, *valueField);
        valueExpr.code.free();

        List<Param> empty;
        token_entity operand("=", SINGLE, 0,0, ASSIGN);

        valueExpr.code.__asm64.push_back(SET_Di(i64, op_NEWCLASS, field->owner->address)); // create Enum class

        Method* constr = field->owner->getConstructor(empty, true);
        valueExpr.code.__asm64.push_back(SET_Di(i64, op_CALL, constr->address)); // call Enum()
        valueExpr.code.__asm64.push_back(SET_Di(i64, op_MOVG, field->owner->address));
        valueExpr.code.__asm64.push_back(SET_Di(i64, op_MOVN, field->address)); // get local
        valueExpr.code.__asm64.push_back(SET_Ei(i64, op_POPOBJ)); // call Enum()
        valueExpr.code.__asm64.push_back(SET_Di(i64, op_MOVN, valueField->address));
        assignValue(operand, out, valueExpr, expr, ast);
        staticMainInserts.inject(0, out.code); out.code.free();
    }
}

void RuntimeEngine::assignEnumName(Ast *ast, Field *field, Expression &out) {
    Field *valueField = field->klass->getField("name", true);
    if(valueField != NULL) {

        stringMap.addif(field->name);
        out.code.__asm64.push_back(SET_Di(i64, op_NEWSTRING, stringMap.indexof(field->name))); // create Enum class

        out.code.__asm64.push_back(SET_Di(i64, op_MOVG, field->owner->address));
        out.code.__asm64.push_back(SET_Di(i64, op_MOVN, field->address)); // get local
        out.code.__asm64.push_back(SET_Di(i64, op_MOVN, valueField->address));
        out.code.__asm64.push_back(SET_Ei(i64, op_POPOBJ));
        staticMainInserts.inject(0, out.code); out.code.free();
    }
}

void RuntimeEngine::assignEnumArray(Ast *ast, ClassObject *klass, Expression &out) {
    if(!(klass->getName() == "Enum" && klass->getModuleName() == "std")) {
        Field *enums = klass->getField("enums", true);
        if(enums != NULL) {

            out.code.push_i64(SET_Di(i64, op_MOVI, klass->fieldCount()-3), i64ebx); // call Enum()
            out.code.__asm64.push_back(SET_Ci(i64, op_NEWCLASSARRAY, i64ebx, 0, klass->address)); // create Enum class

            // skip the 3 first fields
            for(long i = 3; i < klass->fieldCount(); i++) {

                out.code.__asm64.push_back(SET_Di(i64, op_MOVG, klass->address));
                out.code.__asm64.push_back(SET_Di(i64, op_MOVN, klass->getField(i)->address)); // get enum field
                out.code.push_i64(SET_Ei(i64, op_PUSHOBJ)); // push object

                out.code.push_i64(SET_Di(i64, op_MOVSL, -1)); // get our array object
                out.code.push_i64(SET_Di(i64, op_MOVN, (i-3))); // select array element
                out.code.push_i64(SET_Ei(i64, op_POPOBJ)); // set object
            }

            out.code.__asm64.push_back(SET_Di(i64, op_MOVG, klass->address));
            out.code.__asm64.push_back(SET_Di(i64, op_MOVN, enums->address)); // get enums[]
            out.code.__asm64.push_back(SET_Ei(i64, op_POPOBJ));
            staticMainInserts.inject(staticMainInserts.size(), out.code);
        }
    } else {
        int i = 0;
    }
}

int RuntimeEngine::parseMethodAccessSpecifiers(List<AccessModifier> &modifiers) {
    for(long i = 0; i < modifiers.size(); i++) {
        AccessModifier modifier = modifiers.get(i);
        if(modifier == mCONST || modifier == mUNDEFINED)
            return i;
    }

    if(modifiers.get(0) <= PROTECTED) {
        if(modifiers.size() >= 3)
            return (int)(modifiers.size() - 1);
        else if(modifiers.size() == 2) {
            if(modifiers.get(1) != STATIC)
                return 1;
        }
    }
    else if(modifiers.get(0) == STATIC) {
        if(modifiers.size() >= 2)
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
       && !modifiers.find(PROTECTED)) {
        modifiers.add(PRIVATE);
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
        errors->createNewError(COULD_NOT_RESOLVE, utype.field.note.getLine(), utype.field.note.getCol(), " `" + utype.field.name + "`");
        field.type = CLASSFIELD;
        field.note = utype.field.note;
        field.modifiers.addAll(utype.field.modifiers);
    } else if(utype.type == CLASS) {
        field.type = CLASS;
        field.note = utype.klass->note;
        field.klass = utype.klass;
        field.modifiers.add(utype.klass->getAccessModifier());
    } else if(utype.type == VAR) {
        if(utype.isProtoType) {
            field = utype.prototype;
        } else
            field.type = VAR;
        field.modifiers.add(PUBLIC);
    } else if(utype.type == OBJECT) {
        field.type = OBJECT;
        field.modifiers.add(PUBLIC);
    } else if(utype.type == TYPEGENERIC) {
        field.type = TYPEGENERIC;
        field.klass = utype.klass;
        field.ast = utype.uType;
        field.key = utype.referenceName;
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
        return utype.field;

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
    utype_arg.value.uType = ast->getSubAst(0);
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
        if(ast->getSubAst(i)->getType()==ast_func_prototype) {
            parseFuncPrototype(ast->getSubAst(i), &utype_arg.value.prototype);
            utype_arg.key=ast->getSubAst(i)->getEntity(1).getToken();
            utype_arg.value.isProtoType=true;
            utype_arg.value.type=VAR;
        } else
            utype_arg = parseUtypeArg(ast->getSubAst(i));
        utype_argmap.key.push_back(utype_arg.key);
        utype_argmap.value.push_back(utype_arg.value);
        utype_arg.value = ResolvedReference();
    }

    return utype_argmap;
}

void RuntimeEngine::parseMethodReturnType(Expression& expression, Method& method) {
    method.array = expression.utype.array;
    if(expression.type == expression_class) {
        method.type = CLASS;
        method.klass = expression.utype.klass;
    } else if(expression.type == expression_generic) {
        method.type = TYPEGENERIC;
        method.key = expression.utype.referenceName;
    } else if(expression.type == expression_native) {
        method.type = expression.utype.type;
    } else {
        method.type = UNDEFINED;
        errors->createNewError(GENERIC, expression.link->line, expression.link->col, "expected class or native type for method's return value");
    }
}

void RuntimeEngine::parseFieldReturnType(Expression& expression, Field& field) {
    if(expression.type == expression_class) {
        field.returnType = CLASS;
        field.klass = expression.utype.klass;
    } else if(expression.type == expression_generic) {
        field.returnType = TYPEGENERIC;
        field.key = expression.utype.referenceName;
    } else if(expression.type == expression_native) {
        field.returnType = expression.utype.type;
    } else {
        field.returnType = UNDEFINED;
        errors->createNewError(GENERIC, expression.link->line, expression.link->col, "expected class or native type for method's return value");
    }
}

void RuntimeEngine::resolveMethodDecl(Ast* ast, bool global) {
    List<AccessModifier> modifiers;
    int startpos=1;

    if(parseAccessDecl(ast, modifiers, startpos)){
        if(global)
            createNewWarning(GENERIC, ast->line, ast->col, "access modifiers ignored on global functions");
        parseMethodAccessModifiers(modifiers, ast);
        if(global) {
            modifiers.free();
            modifiers.add(PUBLIC);
            modifiers.add(STATIC);
        }
    } else {
        if(global) {
            modifiers.add(PUBLIC);
            modifiers.add(STATIC);
        } else
            modifiers.add(PRIVATE);
    }

    List<Param> params;
    string name =  ast->getEntity(startpos).getToken();
    parseMethodParams(params, parseUtypeArgList(ast->getSubAst(ast_utype_arg_list)), ast->getSubAst(ast_utype_arg_list));

    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    Expression utype(ast);
    Method method = Method(name, currentModule, currentScope()->klass, params, modifiers, NULL, note, sourceFiles.indexof(activeParser->sourcefile), false, false);
    if(ast->hasSubAst(ast_method_return_type)) {
        utype = parseUtype(ast->getSubAst(ast_method_return_type));
        parseMethodReturnType(utype, method);
    } else
        method.type = TYPEVOID;

    if(!resolvedMethods) {
        method.ast = ast;
        if(!currentScope()->klass->isGeneric())
            method.address = methods++;
        checkMainMethodSignature(method, global);
        if(!currentScope()->klass->addFunction(method)) {
            this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                         "function `" + name + "` is already defined in the scope");
            printNote(currentScope()->klass->getFunction(name, params)->note, "function `" + name + "` previously defined here");
        }
    } else {
        for(long i = 0; i < currentScope()->klass->functionCount(); i++) {
            if(currentScope()->klass->getFunction(i)->ast == ast) {
                currentScope()->klass->getFunction(i)->getParams().addAll(params);
                return;
            }
        }
    }
}

void RuntimeEngine::resolveDelegateDecl(Ast* ast) {
    List<AccessModifier> modifiers;
    int startpos=1;

    if(parseAccessDecl(ast, modifiers, startpos)){
        parseMethodAccessModifiers(modifiers, ast);
    } else {
        modifiers.add(PUBLIC);
    }

    List<Param> params;
    string name =  ast->getEntity(startpos+3).getToken();
    parseMethodParams(params, parseUtypeArgList(ast->getSubAst(ast_utype_arg_list)), ast->getSubAst(ast_utype_arg_list));

    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    Expression utype(ast);
    Method method = Method(name, currentModule, currentScope()->klass, params, modifiers, NULL, note, sourceFiles.indexof(activeParser->sourcefile), true, false);
    if(ast->hasSubAst(ast_method_return_type)) {
        utype = parseUtype(ast->getSubAst(ast_method_return_type));
        parseMethodReturnType(utype, method);
    } else
        method.type = TYPEVOID;

    if(!resolvedMethods) {
        method.ast = ast;
        if (!currentScope()->klass->isGeneric())
            method.address = methods++;
        if (!currentScope()->klass->addFunction(method)) {
            this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                         "function `" + name + "` is already defined in the scope");
            printNote(currentScope()->klass->getFunction(name, params)->note,
                      "function `" + name + "` previously defined here");
        }
    } else {
        for(long i = 0; i < currentScope()->klass->functionCount(); i++) {
            if(currentScope()->klass->getFunction(i)->ast == ast) {
                currentScope()->klass->getFunction(i)->getParams().addAll(params);
                return;
            }
        }
    }
}

void RuntimeEngine::resolveDelegatePostDecl(Ast* ast) {
    List<AccessModifier> modifiers;
    int startpos=1;

    if(parseAccessDecl(ast, modifiers, startpos)){
        parseMethodAccessModifiers(modifiers, ast);
    } else {
        modifiers.add(PUBLIC);
    }

    List<Param> params;
    string name =  ast->getEntity(startpos+3).getToken();
    parseMethodParams(params, parseUtypeArgList(ast->getSubAst(ast_utype_arg_list)), ast->getSubAst(ast_utype_arg_list));

    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    Expression utype(ast);
    Method method = Method(name, currentModule, currentScope()->klass, params, modifiers, NULL, note, sourceFiles.indexof(activeParser->sourcefile), false, true);
    if(ast->hasSubAst(ast_method_return_type)) {
        utype = parseUtype(ast->getSubAst(ast_method_return_type));
        parseMethodReturnType(utype, method);
    } else
        method.type = TYPEVOID;

    if(!resolvedMethods) {
        method.ast = ast;
        if (!currentScope()->klass->isGeneric())
            method.address = uniqueDelegateId++;
        if (!currentScope()->klass->addFunction(method)) {
            this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                         "function `" + name + "` is already defined in the scope");
            printNote(currentScope()->klass->getFunction(name, params)->note,
                      "function `" + name + "` previously defined here");
        }
    } else {
        for(long i = 0; i < currentScope()->klass->functionCount(); i++) {
            if(currentScope()->klass->getFunction(i)->ast == ast) {
                currentScope()->klass->getFunction(i)->getParams().addAll(params);
                return;
            }
        }
    }
}

Operator RuntimeEngine::stringToOp(string op) {
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
    List<AccessModifier> modifiers;
    int startpos=2;

    if(parseAccessDecl(ast, modifiers, startpos)){
        parseMethodAccessModifiers(modifiers, ast);
    } else {
        modifiers.add(PRIVATE);
    }

    List<Param> params;
    string op =  ast->getEntity(startpos).getToken();
    parseMethodParams(params, parseUtypeArgList(ast->getSubAst(ast_utype_arg_list)), ast->getSubAst(ast_utype_arg_list));

    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    Expression utype(ast);
    OperatorOverload operatorOverload = OperatorOverload(note, currentScope()->klass, params, modifiers, NULL, stringToOp(op), sourceFiles.indexof(activeParser->sourcefile), op);
    if(ast->hasSubAst(ast_method_return_type)) {
        utype = parseUtype(ast->getSubAst(ast_method_return_type));
        parseMethodReturnType(utype, operatorOverload);
    } else
        operatorOverload.type = TYPEVOID;


    if(!resolvedMethods) {
        operatorOverload.ast = ast;
        if (!currentScope()->klass->isGeneric())
            operatorOverload.address = methods++;

        if (!currentScope()->klass->addOperatorOverload(operatorOverload)) {
            this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                         "function `" + op + "` is already defined in the scope");
            printNote(currentScope()->klass->getOverload(stringToOp(op), params)->note,
                      "function `" + op + "` previously defined here");
        }
    } else {
        for(long i = 0; i < currentScope()->klass->overloadCount(); i++) {
            if(currentScope()->klass->getOverload(i)->ast == ast) {
                currentScope()->klass->getOverload(i)->getParams().addAll(params);
                return;
            }
        }
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
    List<AccessModifier> modifiers;
    int startpos=0;


    if(parseAccessDecl(ast, modifiers, startpos)){
        parsConstructorAccessModifiers(modifiers, ast);
    } else {
        modifiers.add(PRIVATE);
    }

    List<Param> params;
    string name = ast->getEntity(startpos).getToken();

    if(name == currentScope()->klass->getName()) {
        parseMethodParams(params, parseUtypeArgList(ast->getSubAst(ast_utype_arg_list)), ast->getSubAst(ast_utype_arg_list));
        RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                       ast->line, ast->col);

        Method method = Method(name, currentModule, currentScope()->klass, params, modifiers, NULL, note, sourceFiles.indexof(activeParser->sourcefile), false, false);
        method.type = CLASS;
        method.klass = currentScope()->klass;
        method.isConstructor=true;

        if(!resolvedMethods) {
            method.ast = ast;
            if (!currentScope()->klass->isGeneric())
                method.address = methods++;
            if (!currentScope()->klass->addConstructor(method)) {
                this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                             "constructor `" + name + "` is already defined in the scope");
                printNote(currentScope()->klass->getConstructor(params)->note,
                          "constructor `" + name + "` previously defined here");
            }
        } else {
            for(long i = 0; i < currentScope()->klass->constructorCount(); i++) {
                if(currentScope()->klass->getConstructor(i)->ast == ast) {
                    currentScope()->klass->getConstructor(i)->getParams().addAll(params);
                    return;
                }
            }
        }
    } else
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                               "constructor `" + name + "` must be the same name as its parent");
}

void RuntimeEngine::addDefaultConstructor(ClassObject* klass, Ast* ast) {
    List<Param> emptyParams;
    List<AccessModifier> modifiers;
    modifiers.add(PUBLIC);

    RuntimeNote note;

    if(ast != NULL) {
        note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                    ast->line, ast->col);
    }

    if(klass->getConstructor(emptyParams, false) == NULL) {
        Method method = Method(klass->getName(), currentModule, klass, emptyParams, modifiers, NULL, note,
                sourceFiles.indexof(activeParser == NULL ? 0 : activeParser->sourcefile), false, false);

        method.isConstructor=true;
        if(!klass->isGeneric())
            method.address = methods++;
        klass->addConstructor(method);
    }
}

void RuntimeEngine::resolveInterfaceDecl(Ast* ast) {
    Ast* block = ast->getSubAst(ast_block), *trunk;
    List<AccessModifier> modifiers;
    ClassObject* klass;
    int startpos=1;

    if(parseAccessDecl(ast, modifiers, startpos)){
        parseClassAccessModifiers(modifiers, ast);
    } else {
        modifiers.add(PROTECTED);
    }

    string name =  ast->getEntity(startpos).getToken();

    if(currentScope()->type == GLOBAL_SCOPE) {
        klass = getClass(currentModule, name, classes);
    }
    else {
        klass = currentScope()->klass->getChildClass(name);
    }

    addScope(Scope(CLASS_SCOPE, klass));
    ClassObject *base = parseBaseClass(ast, ++startpos);
    if(base != NULL) {
        // verify that base is an interface
        if(!base->isInterface()) {
            stringstream err;
            err << "interface '" << klass->getName() << "' must inherit another interface class";
            errors->createNewError(GENERIC, ast->line, ast->col, err.str()); err.str("");
        } else
            klass->setBaseClass(base->getSerial() == klass->getSerial() ? NULL : base);
    }

    stringstream err;
    for(long i = 0; i < block->getSubAstCount(); i++) {
        trunk = block->getSubAst(i);
        CHECK_ERRORS

        switch(trunk->getType()) {
            case ast_class_decl: /* ignore */
                break;
            case ast_var_decl: /* ignore */
                break;
            case ast_method_decl: /* ignore */
                break;
            case ast_operator_decl: /* ignore */
                break;
            case ast_construct_decl: /* ignore */
                break;
            case ast_delegate_post_decl: /* ignore */
                break;
            case ast_delegate_decl:
                err << "interfaces do not allow delegate implementations";
                errors->createNewError(GENERIC, trunk->line, trunk->col, err.str()); err.str("");
                break;
            case ast_interface_decl:
                resolveInterfaceDecl(trunk);
                break;
            default:
                err << ": unknown ast type: " << trunk->getType();
                errors->createNewError(INTERNAL_ERROR, trunk->line, trunk->col, err.str()); err.str("");
                break;
        }
    }
    removeScope();
}

void RuntimeEngine::validateDelegates(ClassObject *host, ClassObject *klass, Ast *ast, bool useBase) {
    List<Method*> delegatesPosts = klass->getDelegatePosts(false);
    List<Method*> delegates = host->getDelegates();

    // enforce rule to have delegates in class
    if(delegatesPosts.size() > 0) {
        Method *func;
        stringstream err;
        for(int i = 0; i < delegatesPosts.size(); i++) {
            if((func=host->getFunction(delegatesPosts.get(i)->getName(), delegatesPosts.get(i)->getParams())) == NULL) {

                error:
                err << "class '" << host->getName() << "' must implement delegate method 'delegate::"
                    << delegatesPosts.get(i)->getName() << paramsToString(delegatesPosts.get(i)->getParams())
                    << "'";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str()); err.str("");
            } else {
                if(!func->delegate) {
                    goto error;
                } else if(!func->sameModifiers(delegatesPosts.get(i))) {
                    err << "class '" << host->getName() << "' must implement delegate method 'delegate::"
                        << delegatesPosts.get(i)->getName() << paramsToString(delegatesPosts.get(i)->getParams())
                        << "' with the same access modifiers as the prototype";
                    errors->createNewError(GENERIC, ast->line, ast->col, err.str()); err.str("");
                } else if(func->type != delegatesPosts.get(i)->type ||
                        (delegatesPosts.get(i)->type == CLASS && !delegatesPosts.get(i)->klass->assignable(func->klass))
                        || func->array != delegatesPosts.get(i)->array) {
                    err << "class '" << host->getName() << "' must implement delegate method 'delegate::"
                        << delegatesPosts.get(i)->getName() << paramsToString(delegatesPosts.get(i)->getParams())
                        << "' with the same return value '" <<
                                (delegatesPosts.get(i)->type == CLASS ? delegatesPosts.get(i)->klass->getFullName()
                                                                      : ResolvedReference::typeToString(delegatesPosts.get(i)->type))
                        << (delegatesPosts.get(i)->array ? "[]" : "")  << "' as the prototype";
                    errors->createNewError(GENERIC, ast->line, ast->col, err.str()); err.str("");
                }
            }
        }
    }

    // verify that all delegates are legitimate
    if(delegates.size() > 0) {
        Method *func;
        for(int i = 0; i < delegates.size(); i++) {
            if((func = klass->getDelegateFunction(delegates.get(i)->getName(), delegates.get(i)->getParams(), true, true)) != NULL) {
                if(func->delegatePost) {
                    // were good
                    delegates.get(i)->delegateAddress = func->address;
                } else {
                    // not a delegate
                    if(useBase && klass->getBaseClass() != NULL)
                        continue;

                    stringstream err;
                    err << "class(s) inherited by class '" << host->getName() << "' has no prototype of delegate method 'delegate::"
                        << delegates.get(i)->getName() << paramsToString(delegates.get(i)->getParams())
                        << "'";
                    errors->createNewError(GENERIC, ast->line, ast->col, err.str());
                }
            }
        }
    }
}

void RuntimeEngine::resolveClassDeclDelegates(Ast* ast) {
    Ast* block = ast->getSubAst(ast_block), *trunk;
    List<AccessModifier> modifiers;
    ClassObject* klass;
    int startpos=1;

    parseAccessDecl(ast, modifiers, startpos);
    string name =  ast->getEntity(startpos).getToken();

    if(currentScope()->type == GLOBAL_SCOPE) {
        klass = getClass(currentModule, name, classes);
    }
    else {
        klass = currentScope()->klass->getChildClass(name);
    }

    ClassObject *base = klass->getBaseClass();
    if(base != NULL) {
        validateDelegates(klass, base, ast, false);
    }

    for(long i = 0; i < klass->interfaceCount(); i++) {
        ClassObject *intf = klass->getInterface(i);

        validateDelegates(klass, intf, ast, intf->getBaseClass() != NULL);
        if(intf->getBaseClass() != NULL) {
            for(;;) {

                intf = intf->getBaseClass();
                validateDelegates(klass, intf, ast, intf->getBaseClass() != NULL);

                if(intf->getBaseClass() == NULL)
                    break;
            }
        }
    }

    if(klass->interfaceCount() > 0) {

        // look for duplicates
        for(long i = 0; i < klass->interfaceCount(); i++) {
            if(klass->duplicateInterface(klass->getInterface(i))) {
                stringstream err;
                err << "duplicate class '" << klass->getInterface(i)->getName() << "'";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            }
        }
    }

    addScope(Scope(CLASS_SCOPE, klass));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        trunk = block->getSubAst(i);
        CHECK_ERRORS

        switch(trunk->getType()) {
            case ast_class_decl:
                resolveClassDeclDelegates(trunk);
                break;
            case ast_var_decl: /* ignore */
                break;
            case ast_method_decl: /* ignore */
                break;
            case ast_operator_decl: /* ignore */
                break;
            case ast_construct_decl: /* ignore */
                break;
            case ast_delegate_post_decl: /* ignore */
                break;
            case ast_delegate_decl: /* ignore */
                break;
            case ast_interface_decl: /* ignore */
                break;
            case ast_generic_class_decl: /* ignore */
                break;
            case ast_enum_decl: /* ignore */
                break;
            case ast_func_prototype: /* ignore */
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << trunk->getType();
                errors->createNewError(INTERNAL_ERROR, trunk->line, trunk->col, err.str());
                break;
        }
    }
    removeScope();
}

void RuntimeEngine::resolveClassBase(Ast* ast) {
    Ast* block = ast->getSubAst(ast_block), *trunk;
    List<AccessModifier> modifiers;
    ClassObject* klass;
    int startpos=1;

    parseAccessDecl(ast, modifiers, startpos);
    string name =  ast->getEntity(startpos).getToken();

    if(currentScope()->type == GLOBAL_SCOPE) {
        klass = getClass(currentModule, name, classes);
    }
    else {
        klass = currentScope()->klass->getChildClass(name);
    }

    addScope(Scope(CLASS_SCOPE, klass));
    ClassObject *base = parseBaseClass(ast, ++startpos);

    if(!klass->isInterface() && base != NULL && base->isInterface()) {
        stringstream err;
        err << "classes can only inherit other classes, do 'class Dog base Animal : Traits {} ' instead";
        errors->createNewError(GENERIC, ast->line, ast->col, err.str());
    } else {
        if(base != NULL)
            klass->setBaseClass(base->getSerial() == klass->getSerial() ? NULL : base);
    }

    if(ast->hasSubAst(ast_reference_identifier_list)) {
        List<ClassObject*> interfaces = parseRefrenceIdentifierList(ast->getSubAst(ast_reference_identifier_list));

        for(long i = 0; i < interfaces.size(); i++) {
            if(interfaces.get(i) != NULL && !interfaces.get(i)->isInterface()) {
                stringstream err;
                err << "class `" + interfaces.get(i)->getName() + "` is not an interface";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            }
        }
        klass->setInterfaces(interfaces);
    }

    for(long i = 0; i < block->getSubAstCount(); i++) {
        trunk = block->getSubAst(i);
        CHECK_ERRORS

        switch(trunk->getType()) {
            case ast_class_decl:
                resolveClassBase(trunk);
                break;
            case ast_interface_decl:
                resolveClassBase(trunk);
                break;
        }
    }

    removeScope();
}

void RuntimeEngine::resolveClassDecl(Ast* ast, bool inlineField, bool forEnum) {
    Ast* block = ast->getSubAst(ast_block), *trunk;
    List<AccessModifier> modifiers;
    ClassObject* klass;
    int startpos=1;

    parseAccessDecl(ast, modifiers, startpos);
    string name =  ast->getEntity(startpos).getToken();

    if(currentScope()->type == GLOBAL_SCOPE) {
        klass = getClass(currentModule, name, classes);
    }
    else {
        klass = currentScope()->klass->getChildClass(name);
    }

    if(!inlineField && resolvedFields && !forEnum && klass->address == -1)
        klass->address = classSize++;

    klass->setAst(ast);
    addScope(Scope(CLASS_SCOPE, klass));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        trunk = block->getSubAst(i);
        CHECK_ERRORS

        switch(trunk->getType()) {
            case ast_class_decl:
                resolveClassDecl(trunk, inlineField, forEnum);
                break;
            case ast_var_decl:
                if(!forEnum && resolvedGenerics && !resolvedFields || inlineField)
                    resolveVarDecl(trunk, inlineField);
                break;
            case ast_method_decl:
                if(!forEnum && resolvedGenerics && !inlineField && resolvedFields)
                    resolveMethodDecl(trunk);
                break;
            case ast_operator_decl:
                if(!forEnum && resolvedGenerics && !inlineField && resolvedFields)
                    resolveOperatorDecl(trunk);
                break;
            case ast_construct_decl:
                if(!forEnum  && resolvedGenerics&& !inlineField && resolvedFields)
                    resolveConstructorDecl(trunk);
                break;
            case ast_delegate_post_decl:
                if(!forEnum && resolvedGenerics && !inlineField && resolvedFields)
                    resolveDelegatePostDecl(trunk);
                break;
            case ast_delegate_decl:
                if(!forEnum && resolvedGenerics && !inlineField && resolvedFields)
                    resolveDelegateDecl(trunk);
                break;
            case ast_interface_decl:
                if(resolvedGenerics)
                    resolveClassDecl(trunk, inlineField);
                break;
            case ast_generic_class_decl:
                if(!resolvedGenerics)
                    resolveGenericClassDecl(trunk, inlineField, forEnum);
                break;
            case ast_enum_decl: /* ignore */
                if(forEnum)
                    resolveEnumDecl(trunk);
                break;
            case ast_func_prototype:
                if(!forEnum && resolvedGenerics && resolvedFields)
                    resolvePrototypeDecl(trunk);
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << trunk->getType();
                errors->createNewError(INTERNAL_ERROR, trunk->line, trunk->col, err.str());
                break;
        }
    }

    if(!inlineField && resolvedFields)
        addDefaultConstructor(klass, ast);
    removeScope();
}

void RuntimeEngine::resolveEnumDecl(Ast* ast) {
    Ast* block = ast->getSubAst(ast_enum_identifier_list), *trunk;
    List<AccessModifier> modifiers;
    ClassObject* klass;
    int startpos=1;

    parseAccessDecl(ast, modifiers, startpos);
    string name =  ast->getEntity(startpos).getToken();

    if(currentScope()->type == GLOBAL_SCOPE) {
        klass = getClass(currentModule, name, classes);
    }
    else {
        klass = currentScope()->klass->getChildClass(name);
    }

    klass->address = classSize++;
    ClassObject *base = tryClassResolve("std", "Enum", ast);

    if(base != NULL && base->isInterface()) {
        stringstream err;
        err << "support class for enums found to b an interface";
        errors->createNewError(GENERIC, ast->line, ast->col, err.str());
    } else {
        if(base != NULL)
            klass->setBaseClass(base->getSerial() == klass->getSerial() ? NULL : base);
        else {
            stringstream err;
            err << "support class for enums not found";
            errors->createNewError(GENERIC, ast->line, ast->col, err.str());
        }
    }
    addScope(Scope(CLASS_SCOPE, klass));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        trunk = block->getSubAst(i);
        CHECK_ERRORS

        switch(trunk->getType()) {
            case ast_class_decl: /* ignore */
                break;
            case ast_var_decl: /* ignore */
                break;
            case ast_method_decl: /* ignore */
                break;
            case ast_operator_decl: /* ignore */
                break;
            case ast_construct_decl: /* ignore */
                break;
            case ast_delegate_post_decl: /* ignore */
                break;
            case ast_delegate_decl: /* ignore */
                break;
            case ast_interface_decl: /* ignore */
                break;
            case ast_generic_class_decl: /* ignore */
                break;
            case ast_enum_decl: /* ignore */
                break;
            case ast_enum_identifier:
                resolveEnumVarDecl(trunk);
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << trunk->getType();
                errors->createNewError(INTERNAL_ERROR, trunk->line, trunk->col, err.str());
                break;
        }
    }

    Expression out(block);
    assignEnumArray(block, klass, out);
    addDefaultConstructor(klass, ast);
    removeScope();
}

void RuntimeEngine::resolveClassEnumDecl(Ast* ast) {
    Ast* block = ast->getSubAst(ast_block), *trunk;
    List<AccessModifier> modifiers;
    ClassObject* klass;
    int startpos=1;

    parseAccessDecl(ast, modifiers, startpos);
    string name =  ast->getEntity(startpos).getToken();

    if(currentScope()->type == GLOBAL_SCOPE) {
        klass = getClass(currentModule, name, classes);
    }
    else {
        klass = currentScope()->klass->getChildClass(name);
    }

    addScope(Scope(CLASS_SCOPE, klass));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        trunk = block->getSubAst(i);
        CHECK_ERRORS

        switch(trunk->getType()) {
            case ast_class_decl: /* ignore */
                resolveClassEnumDecl(trunk);
                break;
            case ast_var_decl: /* ignore */
                break;
            case ast_method_decl: /* ignore */
                break;
            case ast_operator_decl: /* ignore */
                break;
            case ast_construct_decl: /* ignore */
                break;
            case ast_delegate_post_decl: /* ignore */
                break;
            case ast_delegate_decl: /* ignore */
                break;
            case ast_interface_decl: /* ignore */
                break;
            case ast_generic_class_decl: /* ignore */
                break;
            case ast_enum_decl: /* ignore */
                resolveEnumDecl(trunk);
                break;
            case ast_enum_identifier: /* ignore */
                break;
            default:
                /* ignore */
                break;
        }
    }

    Expression out(block);
    addDefaultConstructor(klass, ast);
    removeScope();
}

void RuntimeEngine::resolveGenericClassDecl(Ast* ast, bool inlineField, bool forEnum) {
    Ast* block = ast->getSubAst(ast_block), *trunk;
    List<AccessModifier> modifiers;
    ClassObject* klass;
    int startpos=1;

    parseAccessDecl(ast, modifiers, startpos);
    string name =  ast->getEntity(startpos).getToken();

    if(currentScope()->type == GLOBAL_SCOPE) {
        klass = getClass(currentModule, name, generics);
    }
    else {
        klass = currentScope()->klass->getChildClass(name);
    }

    addScope(Scope(CLASS_SCOPE, klass));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        trunk = block->getSubAst(i);
        CHECK_ERRORS

        switch(trunk->getType()) {
            case ast_class_decl:
                resolveClassDecl(trunk, inlineField, forEnum);
                break;
            case ast_var_decl:
                if(!forEnum && (!resolvedFields || inlineField))
                    resolveVarDecl(trunk, inlineField);
                break;
            case ast_method_decl:
                if(!forEnum && !inlineField)
                    resolveMethodDecl(trunk);
                break;
            case ast_operator_decl:
                if(!forEnum && !inlineField)
                    resolveOperatorDecl(trunk);
                break;
            case ast_construct_decl:
                if(!forEnum && !inlineField)
                    resolveConstructorDecl(trunk);
                break;
            case ast_delegate_post_decl:
                if(!forEnum && !inlineField)
                    resolveDelegatePostDecl(trunk);
                break;
            case ast_delegate_decl:
                if(!forEnum && !inlineField)
                    resolveDelegateDecl(trunk);
                break;
            case ast_interface_decl:
                if(resolvedGenerics)
                    resolveClassDecl(trunk, inlineField);
                break;
            case ast_generic_class_decl:
                if(resolvedFields)
                    resolveGenericClassDecl(ast, true, forEnum);
                break;
            case ast_enum_decl: /* ignore */
                if(forEnum)
                    resolveEnumDecl(ast);
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << trunk->getType();
                errors->createNewError(INTERNAL_ERROR, trunk->line, trunk->col, err.str());
                break;
        }
    }

    if(!inlineField && !resolvedGenerics)
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

bool RuntimeEngine::classExists(string module, string name, List<ClassObject*> &classes) {
    ClassObject* klass = NULL;
    for(unsigned int i = 0; i < classes.size(); i++) {
        klass = classes.get(i);
        if(klass->getName() == name) {
            if(module != "")
                return klass->getModuleName() == module;
            return true;
        }
    }

    return false;
}

bool RuntimeEngine::addClass(ClassObject* klass) {
    if(!classExists(klass->getModuleName(), klass->getName(), classes)) {
        classes.add(klass);
        return true;
    }
    delete klass;
    return false;
}

bool RuntimeEngine::addGeericClass(ClassObject* klass) {
    if(!classExists(klass->getModuleName(), klass->getName(), generics)) {
        generics.add(klass);
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

ClassObject *RuntimeEngine::getClass(string module, string name, List<ClassObject*> &classes) {
    ClassObject* klass = NULL;
    for(unsigned int i = 0; i < classes.size(); i++) {
        klass = classes.get(i);
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
    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(pAst==NULL ? 0 : pAst->line),
                                   pAst==NULL ? 0 : pAst->line, pAst==NULL ? 0 : pAst->col);

    if(!this->addClass(new ClassObject(name, currentModule, this->uniqueSerialId++,modifiers.get(0), note))){

        this->errors->createNewError(PREVIOUSLY_DEFINED, pAst->line, pAst->col, "class `" + name +
                                                                          "` is already defined in module {" + currentModule + "}");

        printNote(this->getClass(currentModule, name, classes)->note, "class `" + name + "` previously defined here");
        return getClass(currentModule, name, classes);
    } else
        return getClass(currentModule, name, classes);
}

ClassObject *RuntimeEngine::addGlobalGenericClassObject(string name, List<AccessModifier>& modifiers, Ast *pAst) {
    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(pAst->line),
                                   pAst->line, pAst->col);

    if(!this->addGeericClass(new ClassObject(name, currentModule, this->uniqueSerialId++,modifiers.get(0), note))){

        this->errors->createNewError(PREVIOUSLY_DEFINED, pAst->line, pAst->col, "class `" + name +
                                                                                "` is already defined in module {" + currentModule + "}");

        printNote(this->getClass(currentModule, name, generics)->note, "class `" + name + "` previously defined here");
        return getClass(currentModule, name, generics);
    } else
        return getClass(currentModule, name, generics);
}

ClassObject *RuntimeEngine::addChildClassObject(string name, List<AccessModifier>& modifiers, Ast *ast, ClassObject* super) {
    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    if(!super->addChildClass(new ClassObject(name,
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

void RuntimeEngine::parseClassDecl(Ast *ast, bool isInterface)
{
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

    if(currentScope()->klass == NULL) {
        currentClass = addGlobalClassObject(className, modifiers, ast);

        stringstream ss;
        ss << currentModule << "#" << currentClass->getName();
        currentClass->setFullName(ss.str());
    }
    else {
        currentClass = addChildClassObject(className, modifiers, ast, currentScope()->klass);

        stringstream ss;
        ss << currentScope()->klass->getFullName() << "." << currentClass->getName();
        currentClass->setFullName(ss.str());
    }

    currentClass->setIsInterface(isInterface);
    addScope(Scope(CLASS_SCOPE, currentClass));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        ast = block->getSubAst(i);
        CHECK_ERRORS

        switch(ast->getType()) {
            case ast_class_decl:
                parseClassDecl(ast);
                break;
            case ast_var_decl:
                parseVarDecl(ast);
                break;
            case ast_method_decl: /* Will be parsed later */
                break;
            case ast_func_prototype:
                parseProtypeDecl(ast);
                break;
            case ast_operator_decl: /* Will be parsed later */
                break;
            case ast_construct_decl: /* Will be parsed later */
                break;
            case ast_delegate_post_decl: /* Will be parsed later */
                break;
            case ast_delegate_decl: /* Will be parsed later */
                break;
            case ast_interface_decl:
                parseClassDecl(ast, true);
                break;
            case ast_generic_class_decl:
                parseGenericClassDecl(ast, false);
                break;
            case ast_generic_interface_decl:
                parseGenericClassDecl(ast, true);
                break;
            case ast_enum_decl:
                parseEnumDecl(ast);
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

void RuntimeEngine::parseEnumDecl(Ast *ast)
{
    Ast* block = ast->getSubAst(ast_enum_identifier_list);
    List<AccessModifier> modifiers;
    ClassObject* currentClass;
    int startPosition=1;

    if(parseAccessDecl(ast, modifiers, startPosition)){
        parseClassAccessModifiers(modifiers, ast);
    } else {
        modifiers.add(PROTECTED);
    }

    string className =  ast->getEntity(startPosition).getToken();

    if(currentScope()->klass == NULL) {
        currentClass = addGlobalClassObject(className, modifiers, ast);
        enums.addif(currentClass);
        stringstream ss;
        ss << currentModule << "#" << currentClass->getName();
        currentClass->setFullName(ss.str());
    }
    else {
        currentClass = addChildClassObject(className, modifiers, ast, currentScope()->klass);

        stringstream ss;
        ss << currentScope()->klass->getFullName() << "." << currentClass->getName();
        currentClass->setFullName(ss.str());
        enums.addif(currentClass);
    }

    currentClass->setIsEnum(true);
    addScope(Scope(CLASS_SCOPE, currentClass));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        ast = block->getSubAst(i);
        CHECK_ERRORS

        switch(ast->getType()) {
            case ast_class_decl: /* invalid */
                break;
            case ast_var_decl: /* invalid */
                break;
            case ast_method_decl: /* invalid */
                break;
            case ast_operator_decl: /* invalid */
                break;
            case ast_construct_decl: /* invalid */
                break;
            case ast_delegate_post_decl: /* invalid */
                break;
            case ast_delegate_decl: /* invalid */
                break;
            case ast_interface_decl: /* invalid */
                break;
            case ast_generic_class_decl: /* invalid */
                break;
            case ast_generic_interface_decl: /* invalid */
                break;
            case ast_enum_identifier:
                parseEnumVar(ast);
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

void RuntimeEngine::parseGenericClassDecl(Ast *ast, bool isInterface)
{
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
    List<string> identifierList;
    parseIdentifierList(ast, identifierList);

    if(currentScope()->klass == NULL) {
        currentClass = addGlobalGenericClassObject(className, modifiers, ast);

        stringstream ss;
        ss << currentModule << "#" << currentClass->getName();
        currentClass->setFullName(ss.str());
    }
    else {
        currentClass = addChildClassObject(className, modifiers, ast, currentScope()->klass);

        stringstream ss;
        ss << currentScope()->klass->getFullName() << "." << currentClass->getName();
        currentClass->setFullName(ss.str());
    }

    for(long i = 0; i < identifierList.size(); i++) {
        currentClass->addGenericKey(identifierList.get(i));
    }

    currentClass->setIsInterface(isInterface);
    currentClass->setIsGeneric(true);
    currentClass->setAst(ast);
    addScope(Scope(CLASS_SCOPE, currentClass));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        ast = block->getSubAst(i);
        CHECK_ERRORS

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
            case ast_delegate_post_decl: /* Will be parsed later */
                break;
            case ast_delegate_decl: /* Will be parsed later */
                break;
            case ast_interface_decl:
                parseClassDecl(ast, true);
                break;
            case ast_generic_class_decl:
                parseGenericClassDecl(ast, false);
                break;
            case ast_generic_interface_decl:
                parseGenericClassDecl(ast, true);
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

void RuntimeEngine::parseVarDecl(Ast *ast, bool global)
{
    List<AccessModifier> modifiers;
    int startpos=0;

    if(parseAccessDecl(ast, modifiers, startpos)){
        if(global) {
            if(!modifiers.find(mCONST))
                createNewWarning(GENERIC, ast->line, ast->col, "access modifiers ignored on global functions");
        }
        parseVarAccessModifiers(modifiers, ast);
        if(global) {
            if(modifiers.find(mCONST)) {
                modifiers.free();
                modifiers.add(PUBLIC);
                modifiers.add(STATIC);
                modifiers.add(mCONST);
            } else {
                modifiers.free();
                modifiers.add(PUBLIC);
                modifiers.add(STATIC);
            }
        }
    } else {
        if(global) {
            modifiers.add(PUBLIC);
            modifiers.add(STATIC);
        } else
            modifiers.add(PRIVATE);
    }

    parse_var:
    string name =  ast->getEntity(startpos).getToken();
    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    if(!currentScope()->klass->addField(Field(NULL, uniqueSerialId++, name, NULL, modifiers, note))) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                               "field `" + name + "` is already defined in the scope");
        printNote(note, "field `" + name + "` previously defined here");
    }

    if(ast->hasSubAst(ast_var_decl)) {
        startpos = 0;
        ast = ast->getSubAst(ast_var_decl);
        goto parse_var;
    }
}

void RuntimeEngine::parseEnumVar(Ast *ast)
{
    List<AccessModifier> modifiers;
    modifiers.add(PUBLIC);
    modifiers.add(STATIC);
    modifiers.add(mCONST);

    string name =  ast->getEntity(0).getToken();
    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    if(!currentScope()->klass->addField(Field(NULL, uniqueSerialId++, name, NULL, modifiers, note))) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                               "enum `" + name + "` is already defined in the scope");
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
        modifiers.add(PRIVATE);
    }
}

string RuntimeEngine::getString(long index) {
    string str = stringMap.get(index);
    stringstream ss;
    int size;

    if(str.size() <= 35) {
        size = str.size();
    } else
        size = 35;

    ss << "\"";
    char c;
    for(int i = 0; i < size; i++) {
        c = str.at(i);

        switch (c) {
            case '\n':
                ss << "\\n";
                break;
            case '\t':
                ss << "\\t";
                break;
            case '\b':
                ss << "\\b";
                break;
            case '\v':
                ss << "\\v";
                break;
            case '\r':
                ss << "\\r";
                break;
            case '\f':
                ss << "\\f";
                break;
            case '\\':
                ss << "\\";
                break;
            default:
                ss << c;
                break;
        }
    }
    if(str.size() > 35)
        ss << "...\"";
    else
        ss << "\"";
    return ss.str();
}

string copychars(char c, int t) {
    native_string s;
    int it = 0;

    while (it++ < t)
        s += c;

    return s.str();
}

string i64_tostr(int64_t i64)
{
    string str;
    i64_t mi;
    SET_i64(mi, i64);

    str+=(uint8_t)GET_i32w(mi.A);
    str+=(uint8_t)GET_i32x(mi.A);
    str+=(uint8_t)GET_i32y(mi.A);
    str+=(uint8_t)GET_i32z(mi.A);

    str+=(uint8_t)GET_i32w(mi.B);
    str+=(uint8_t)GET_i32x(mi.B);
    str+=(uint8_t)GET_i32y(mi.B);
    str+=(uint8_t)GET_i32z(mi.B);
    return str;
}

std::string RuntimeEngine::generate_manifest() {
    stringstream manifest;

    manifest << (char)manif;
    manifest << ((char)0x02); manifest << c_options.out << ((char)nil);
    manifest << ((char)0x4); manifest << c_options.vers << ((char)nil);
    manifest << ((char)0x5); manifest << c_options.debug ? 1 : 0;
    manifest << ((char)0x6); manifest << i64_tostr(main->address) << ((char)nil);
    manifest << ((char)0x7); manifest << i64_tostr(methods) << ((char)nil);
    manifest << ((char)0x8); manifest << i64_tostr(classSize) << ((char)nil);
    manifest << ((char)0x9 ); manifest << file_vers << ((char)nil);
    manifest << ((char)0x0c); manifest << i64_tostr(stringMap.size()) << ((char)nil);
    manifest << ((char)0x0e); manifest << c_options.target << ((char)nil);
    manifest << ((char)0x0f); manifest << sourceFiles.size() << ((char)nil);
    manifest << '\n' << (char)eoh;

    return manifest.str();
}

std::string RuntimeEngine::generate_header() {
    stringstream header;
    header << (char)file_sig << "SEF"; header << copychars(nil, zoffset);
    header << (char)digi_sig1 << (char)digi_sig2 << (char)digi_sig3;

    header << generate_manifest();
    return header.str();
}

std::string RuntimeEngine::field_to_stream(Field& field) {
    stringstream fstream;

    fstream << ((char)data_field);
    fstream << field.name << ((char)nil);
    fstream << field.address << ((char)nil);
    fstream << field.type << ((char)nil);
    fstream << (field.modifiers.find(STATIC) ? 1 : 0) << ((char)nil);
    fstream << (field.isArray ? 1 : 0) << ((char)nil);
    fstream << (field.type == CLASS ? field.klass->address : -1) << ((char)nil);
    fstream << endl;

    return fstream.str();
}

std::string RuntimeEngine::class_to_stream(ClassObject& klass) {
    stringstream kstream;

    kstream << (char)data_class;
    kstream << (klass.getSuperClass() == NULL ? -1 : klass.getSuperClass()->address) << ((char)nil);
    kstream << (klass.getBaseClass() == NULL ? -1 : klass.getBaseClass()->address) << ((char)nil);
    kstream << i64_tostr(klass.address);
    kstream << klass.getFullName() << ((char)nil);
    kstream << klass.getTotalFieldCount() << ((char)nil);
    kstream << klass.getTotalFunctionCount() << ((char)nil);
    kstream << klass.interfaceCount() << ((char)nil);
    int funcs = klass.getTotalFunctionCount();

    ClassObject* base = klass.getBaseClass();
    while(base != NULL) {
        for(long long i = 0; i < base->fieldCount(); i++) {
            kstream << field_to_stream(*base->getField(i));
        }

        base = base->getBaseClass();
    }

    for(long long i = 0; i < klass.fieldCount(); i++) {
        kstream << field_to_stream(*klass.getField(i));
    }

    /* Constructors */
    for(long long i = 0; i < klass.constructorCount(); i++) {
        kstream << (char)data_method;
        kstream << i64_tostr(klass.getConstructor(i)->address) << ((char)nil);
        allMethods.add(klass.getConstructor(i));
    }

    base = klass.getBaseClass();
    while(base != NULL) {
        for(long long i = 0; i < base->constructorCount(); i++) {
            kstream << (char)data_method;
            kstream << i64_tostr(base->getConstructor(i)->address) << ((char)nil);
        }

        base = base->getBaseClass();
    }

    /* Methods */
    for(long long i = 0; i < klass.functionCount(true); i++) {
        if(!klass.getFunction(i)->delegatePost) {
            kstream << (char)data_method;
            kstream << i64_tostr(klass.getFunction(i)->address) << ((char)nil);
            allMethods.add(klass.getFunction(i));
        }
    }

    base = klass.getBaseClass();
    while(base != NULL) {
        for(long long i = 0; i < base->functionCount(true); i++) {
            if(!base->getFunction(i)->delegatePost) {
                kstream << (char) data_method;
                kstream << i64_tostr(base->getFunction(i)->address) << ((char) nil);
            }
        }

        base = base->getBaseClass();
    }

    /* Overloads */
    for(long long i = 0; i < klass.overloadCount(); i++) {
        kstream << (char)data_method;
        kstream << i64_tostr(klass.getOverload(i)->address) << ((char)nil);
        allMethods.add(klass.getOverload(i));
    }

    base = klass.getBaseClass();
    while(base != NULL) {
        for(long long i = 0; i < base->overloadCount(); i++) {
            kstream << (char)data_method;
            kstream << i64_tostr(base->getOverload(i)->address) << ((char)nil);
        }

        base = base->getBaseClass();
    }

    /* Interfaces */
    for(long long i = 0; i < klass.interfaceCount(); i++) {
        kstream << (char)data_interface;
        kstream << i64_tostr(klass.getInterface(i)->address) << ((char)nil);
    }

    for(long long i = 0; i < klass.childClassCount(); i++) {
        if(klass.getChildClass(i)->isGeneric()) continue;
        kstream << class_to_stream(*klass.getChildClass(i)) << endl;
    }

    return kstream.str();
}

std::string RuntimeEngine::generate_data_section() {
    stringstream data_sec;
    for(int64_t i = 0; i < classes.size(); i++) {
        data_sec << class_to_stream(*classes.get(i)) << endl;
    }

    for(int64_t i = 0; i < sourceFiles.size(); i++) {
        data_sec << (char)data_file;
        data_sec << sourceFiles.get(i) << ((char)nil);
    }

    data_sec << "\n"<< "\n" << (char)eos;

    List<Method*> reorderedList;
    int64_t iter=0;

    readjust:
    for(int64_t i = 0; i < allMethods.size(); i++) {
        Method *func = allMethods.get(i);
        if(func->address == iter) {
            iter++;
            reorderedList.add(func);
            if(iter < allMethods.size())
                goto readjust;
        }
    }

    allMethods.addAll(reorderedList);
    reorderedList.free();

    return data_sec.str();
}

std::string RuntimeEngine::generate_string_section() {
    stringstream strings;

    for(int64_t i = 0; i < stringMap.size(); i++) {
        string s = stringMap.get(i);
        strings << (char)data_string;
        strings << i64_tostr(i) << ((char)nil) << stringMap.get(i) << ((char)nil);
    }

    strings << "\n"<< "\n" << (char)eos;

    return strings.str();
}

std::string RuntimeEngine::method_to_stream(Method* method) {
    stringstream func;

    for(unsigned int i = 0; i < method->paramCount(); i++) {
        func << method->getParam(i).field.type << ((char)nil);
        func << (method->getParam(i).field.isArray ? 1 : 0) << ((char)nil);
    }

    for(long i = 0; i < method->code.__asm64.size(); i++) {
        func << i64_tostr(method->code.__asm64.get(i));
    }
    return func.str();
}

std::string RuntimeEngine::generate_text_section() {
    stringstream text;

    text << (char)stext;

    for(long i = 0; i < allMethods.size(); i++) {
        Method* f = allMethods.get(i);
        text << (char)data_method;
        text << i64_tostr(f->address);
        text << (0) << ((char)nil);
        text << f->getName() << ((char)nil);
        text << f->getFullName() << ((char)nil);
        text << f->sourceFileLink << ((char)nil);
        text << i64_tostr(f->owner->address);
        text << i64_tostr(f->paramCount());
        text << i64_tostr(f->isStatic() ? f->localVariables : ((f->localVariables>0) ? f->localVariables-1 : 0));
        text << i64_tostr(f->code.__asm64.size());
        text << (f->isStatic() ? 1 : 0) << ((char)nil);
        text << (f->type!=TYPEVOID || f->isConstructor ? 1 : 0) << ((char)nil);
        text << i64_tostr(f->delegateAddress);

        int returnAddress;

        if(f->isStatic()) {
            if(f->paramCount() != 0) {
                if(f->type!=TYPEVOID) {
                    returnAddress=(f->paramCount()-1);
                } else {
                    returnAddress = f->paramCount();
                }
            } else {
                if(f->type!=TYPEVOID) {
                    returnAddress = -1;
                } else {
                    returnAddress = 0;
                }
            }
        } else {
            if(f->paramCount() != 0) {
                if(f->type!=TYPEVOID || f->isConstructor) {
                    returnAddress=f->paramCount();
                } else {
                    returnAddress=f->paramCount()+1;
                }
            } else {
                if(f->type!=TYPEVOID || f->isConstructor) {
                    returnAddress = 0;
                } else {
                    returnAddress = 1;
                }
            }
        }

        text << i64_tostr(returnAddress);

        text << f->line_table.size() << ((char)nil);
        for(unsigned int x = 0; x < f->line_table.size(); x++) {
            text << i64_tostr(f->line_table.get(x).key);
            text << i64_tostr(f->line_table.get(x).value);
        }

        text << f->switchTable.size() << ((char)nil);
        for(unsigned int x = 0; x < f->switchTable.size(); x++) {
            SwitchTable &st = f->switchTable.get(x);

            text << st.addresses.size() << ((char)nil);
            for(long z = 0; z < st.addresses.size(); z++) {
                text << i64_tostr(st.addresses.get(z));
            }

            text << st.values.size() << ((char)nil);
            for(long z = 0; z < st.values.size(); z++) {
                text << i64_tostr(st.values.get(z));
            }
            text << i64_tostr(st.defaultAddress);
        }

        text << f->exceptions.size() << ((char)nil);
        for(unsigned int x = 0; x < f->exceptions.size(); x++) {
            ExceptionTable &et=f->exceptions.get(x);
            text << i64_tostr(f->exceptions.get(x).handler_pc);
            text << i64_tostr(f->exceptions.get(x).end_pc);
            text << f->exceptions.get(x).className.str() << ((char)nil);
            text << i64_tostr(f->exceptions.get(x).local);
            text << i64_tostr(f->exceptions.get(x).start_pc);
        }

        text << f->finallyBlocks.size() << ((char)nil);
        for(unsigned int x = 0; x < f->finallyBlocks.size(); x++) {
            FinallyTable &ft=f->finallyBlocks.get(x);
            text << i64_tostr(ft.start_pc);
            text << i64_tostr(ft.end_pc);
            text << i64_tostr(ft.try_start_pc);
            text << i64_tostr(ft.try_end_pc);
        }
    }

    for(long i = 0; i < allMethods.size(); i++) {
        text << (char)data_byte;
        text << method_to_stream(allMethods.get(i)) << endl;
    }
    text << "\n" << (char)eos;
    return text.str();
}

std::string RuntimeEngine::generate_meta_section() {
    stringstream meta;

    meta << (char)smeta;

    for(unsigned long i = 0; i < parsers.size(); i++) {
        Parser* p = parsers.get(i);
        meta << (char)data_file;
        meta << p->getData() << (char)0x0;
        meta << endl;
    }

    meta << "\n" << (char)eos;
    return meta.str();
}

Zlib zlib;

void RuntimeEngine::generate() {
    File::buffer _ostream, txt_scz;
    _ostream.begin();
    txt_scz.begin();

    _ostream << generate_header() ;
    _ostream << (char)sdata;
    _ostream << generate_data_section();

    if(c_options.optimize) {
        long optimized = 0;
        Optimizer optimizer;
        for(unsigned int i = 0; i < allMethods.size(); i++)
        {
            Method* method = allMethods.get(i);
            optimizer.optimize(method);
            optimized += optimizer.getOptimizedOpcodes();
        }

        optimizationResult = optimized;
        if(c_options.debugMode)
            cout << "Total instructions optimized out: " << optimized << endl;
    }

    for(unsigned int i = 0; i < allMethods.size(); i++)
    {
        Method* method = allMethods.get(i);

        if(method->code.size() == 0 || GET_OP(method->code.__asm64.last()) != op_RET) {
            if(method->isConstructor) {
                method->code.push_i64(SET_Di(i64, op_MOVL, 0));
                method->code.push_i64(SET_Ei(i64, op_RETURNOBJ));
                method->code.push_i64(SET_Ei(i64, op_RET));
            } else if(method->type == VAR) {
                method->code.push_i64(SET_Di(i64, op_MOVI, 0), i64ebx);
                method->code.push_i64(SET_Di(i64, op_RETURNVAL, i64ebx));
                method->code.push_i64(SET_Ei(i64, op_RET));
            } else if(method->type == TYPEVOID) {
                method->code.push_i64(SET_Ei(i64, op_RET));
            } else if(method->type == OBJECT || method->type == CLASS) {
                method->code.push_i64(SET_Ei(i64, op_PUSHNIL));
                method->code.push_i64(SET_Ei(i64, op_POPOBJ));
                method->code.push_i64(SET_Ei(i64, op_RETURNOBJ)); // return null;
                method->code.push_i64(SET_Ei(i64, op_RET));
            } else {
                method->code.push_i64(SET_Ei(i64, op_RET));
            }

        }
    }

    _ostream << generate_string_section();

    txt_scz << generate_text_section();
    if(txt_scz.size() >= data_compress_threshold) {
        _ostream << (char)data_compress;
        stringstream __outbuf__;

        Zlib::AUTO_CLEAN=(true);
        zlib.Compress_Buffer2Buffer(txt_scz.to_str(), __outbuf__, ZLIB_LAST_SEGMENT);

        string buf = __outbuf__.str(); __outbuf__.str("");
        _ostream << i64_tostr(buf.size());
        _ostream << buf; buf = "";
    } else
        _ostream << txt_scz;
    txt_scz.end();

    if(c_options.debug) {
        _ostream << generate_meta_section();
    }

    // ToDo: create line tabel and meta data

    if(c_options.objDump)
        createDumpFile();

    if(File::write(c_options.out.c_str(), _ostream)) {
        cout << progname << ": error: failed to write out to executable " << c_options.out << endl;
    }
    _ostream.end();
    allMethods.free();
}


string RuntimeEngine::find_method(int64_t id) {
    for(unsigned int i = 0; i < allMethods.size(); i++) {
        if(allMethods.get(i)->address == id) {
            stringstream ss;
            ss << allMethods.get(i)->getFullName();
            ss << paramsToString(allMethods.get(i)->getParams());
            return ss.str();
        }
    }
    return "";
}

string RuntimeEngine::find_class(int64_t id) {
    for(unsigned int i = 0; i < classes.size(); i++) {
        if(classes.get(i)->address == id)
            return classes.get(i)->getFullName();
        else {
            ClassObject &klass = *classes.get(i);
            for(unsigned int x = 0; x < klass.childClassCount(); x++) {
                if(klass.getChildClass(x)->address == id)
                    return klass.getChildClass(x)->getFullName();
            }
        }
    }
    return "";
}

void RuntimeEngine::createDumpFile() {
    File::buffer _ostream;
    _ostream.begin();

    _ostream << "Object Dump file:\n" << "################################\n\n";
    stringstream ss;
    ss << "Optimizer: Did optimize " << (int)c_options.optimize << " result -> "
             << optimizationResult << '\n' << '\n';
    _ostream << ss.str();

    for(int64_t i = 0; i < classes.size(); i++) {
        stringstream ss;
        ClassObject &k = *classes.get(i);
        ss << "\n@" << k.address << " " << classes.get(i)->getFullName();
        ss << " fields: " << classes.get(i)->getTotalFieldCount() << " methods: "
           << classes.get(i)->getTotalFunctionCount();
        _ostream << ss.str();

        for(int64_t x = 0; x < k.childClassCount(); x++) {
            stringstream s;
            ClassObject *klass = k.getChildClass(x);
            if(klass->isGeneric()) continue;
            s << "\n@" << klass->address << " " << klass->getFullName();
            s << " fields: " << classes.get(i)->getTotalFieldCount() << " methods: "
               << classes.get(i)->getTotalFunctionCount();
            _ostream << s.str();

        }
    }

    _ostream << "\n\n";
    for(int64_t i = 0; i < stringMap.size(); i++) {
        stringstream ss;
        ss << getString(i);
        ss << "\n";
        _ostream << ss.str();
    }

    _ostream << "\n\n";
    for(unsigned int i =0; i < allMethods.size(); i++) {
        Method* method = allMethods.get(i);
        int64_t  iter = 0, line = 0;

        stringstream tmp;
        tmp << "func:@" << method->address;

        _ostream << tmp.str() << " [" << method->getFullName() << "] "
                 << method->note.getNote("") << "\n\n";
        _ostream << method->getName() << ":\n";

        if(method->exceptions.size() > 0) {
            stringstream sstream;
            sstream << "Exception Table: " << endl << "[\n";
            for(long int x = 0; x < method->exceptions.size(); x++) {
                sstream << "start_pc: " << method->exceptions.get(x).start_pc << endl;
                sstream << "end_pc: " << method->exceptions.get(x).end_pc << endl;
                sstream << "handler_pc: " << method->exceptions.get(x).handler_pc << endl;
                sstream << "local: " << method->exceptions.get(x).local << endl;
                if((x + 1) < method->exceptions.size())
                    sstream  << endl << endl;
            }
            sstream << "]\n";

            _ostream << sstream.str();
        }

        for(unsigned int x = 0; x < method->code.size(); x++) {
            stringstream ss;
            int64_t x64=method->code.__asm64.get(x);
            if(iter < method->line_table.size() && x >= method->line_table.get(iter).value) {
                line = method->line_table.get(iter).key;
                ss << "line: " <<  method->line_table.get(iter++).key << ' ';
                ss <<std::hex << "[0x" << x << std::dec << "] " << x << ":" << '\t';
            } else {
                if(line <=9) ss << "        ";
                else if(line <=99) ss << "         ";
                else if(line <=999) ss << "          ";
                else if(line <=9999) ss << "           ";
                else if(line <=99999) ss << "            ";
                ss <<std::hex << "[0x" << x << std::dec << "] " << x << ":" << '\t';
            }

            switch(GET_OP(x64)) {
                case op_NOP:
                {
                    ss<<"nop";
                    _ostream << ss.str();
                    break;
                }
                case op_INT:
                {
                    ss<<"int 0x" << std::hex << GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_MOVI:
                {
                    ss<<"movi #" << GET_Da(x64) << ", ";
                    ss<< Asm::registrerToString(method->code.__asm64.get(++x)) ;
                    _ostream << ss.str();
                    break;
                }
                case op_RET:
                {
                    ss<<"ret";
                    _ostream << ss.str();
                    break;
                }
                case op_HLT:
                {
                    ss<<"hlt";
                    _ostream << ss.str();
                    break;
                }
                case op_NEWARRAY:
                {
                    ss<<"newarry ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_CAST:
                {
                    ss<<"cast ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_VARCAST:
                {
                    ss<<"vcast ";
                    ss<< GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_MOV8:
                {
                    ss<<"mov8 ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_MOV16:
                {
                    ss<<"mov16 ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_MOV32:
                {
                    ss<<"mov32 ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_MOV64:
                {
                    ss<<"mov64 ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                } case op_MOVU8:
                {
                    ss<<"movu8 ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_MOVU16:
                {
                    ss<<"movu16 ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_MOVU32:
                {
                    ss<<"movu32 ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_MOVU64:
                {
                    ss<<"movu64 ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_RSTORE:
                {
                    ss<<"rstore ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_ADD:
                {
                    ss<<"add ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    ss<< " -> ";
                    ss<< Asm::registrerToString(method->code.__asm64.get(++x));
                    _ostream << ss.str();
                    break;
                }
                case op_SUB:
                {
                    ss<<"sub ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    ss<< " -> ";
                    ss<< Asm::registrerToString(method->code.__asm64.get(++x));
                    _ostream << ss.str();
                    break;
                }
                case op_MUL:
                {
                    ss<<"mul ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    ss<< " -> ";
                    ss<< Asm::registrerToString(method->code.__asm64.get(++x));
                    _ostream << ss.str();
                    break;
                }
                case op_DIV:
                {
                    ss<<"div ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    ss<< " -> ";
                    ss<< Asm::registrerToString(method->code.__asm64.get(++x));
                    _ostream << ss.str();
                    break;
                }
                case op_MOD:
                {
                    ss<<"mod ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    ss<< " -> ";
                    ss<< Asm::registrerToString(method->code.__asm64.get(++x));
                    _ostream << ss.str();
                    break;
                }
                case op_IADD:
                {
                    ss<<"iadd ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", #";
                    ss<< GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_ISUB:
                {
                    ss<<"isub ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", #";
                    ss<< GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_IMUL:
                {
                    ss<<"imul ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", #";
                    ss<< GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_IDIV:
                {
                    ss<<"idiv ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", #";
                    ss<< GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_IMOD:
                {
                    ss<<"imod ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", #";
                    ss<< GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_POP:
                {
                    ss<<"pop";
                    _ostream << ss.str();
                    break;
                }
                case op_INC:
                {
                    ss<<"inc ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_DEC:
                {
                    ss<<"dec ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_MOVR:
                {
                    ss<<"movr ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_IALOAD:
                {
                    ss<<"iaload ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_BRH:
                {
                    ss<<"brh";
                    _ostream << ss.str();
                    break;
                }
                case op_IFE:
                {
                    ss<<"ife";
                    _ostream << ss.str();
                    break;
                }
                case op_IFNE:
                {
                    ss<<"ifne";
                    _ostream << ss.str();
                    break;
                }
                case op_JE:
                {
                    ss<<"je " << GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_JNE:
                {
                    ss<<"jne " << GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_LT:
                {
                    ss<<"lt ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_GT:
                {
                    ss<<"gt ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_LTE:
                {
                    ss<<"lte ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_GTE:
                {
                    ss<<"gte ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_MOVL:
                {
                    ss<<"movl " << GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_POPL:
                {
                    ss<<"popl " << GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_MOVSL:
                {
                    ss<<"movsl #";
                    ss<< GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_MOVBI:
                {
                    ss<<"movbi #" << GET_Da(x64) << ", #";
                    ss<< method->code.__asm64.get(++x);
                    _ostream << ss.str();
                    break;
                }
                case op_SIZEOF:
                {
                    ss<<"sizeof ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_PUT:
                {
                    ss<<"put ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_PUTC:
                {
                    ss<<"_putc ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_CHECKLEN:
                {
                    ss<<"chklen ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_GOTO:
                {
                    ss<<"goto @" << GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_LOADPC:
                {
                    ss<<"loadpc ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_PUSHOBJ:
                {
                    ss<<"pushobj";
                    _ostream << ss.str();
                    break;
                }
                case op_DEL:
                {
                    ss<<"del";
                    _ostream << ss.str();
                    break;
                }
                case op_CALL:
                {
                    ss<<"call @" << GET_Da(x64) << " // <";
                    ss << find_method(GET_Da(x64)) << ">";
                    _ostream << ss.str();
                    break;
                }
                case op_CALLD:
                {
                    ss<<"calld ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_NEWCLASS:
                {
                    ss<<"new_class @" << GET_Da(x64);
                    ss << " // "; ss << find_class(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_MOVN:
                {
                    ss<<"movn #" << GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_SLEEP:
                {
                    ss<<"sleep ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_TEST:
                {
                    ss<<"test ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_TNE:
                {
                    ss<<"tne ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_LOCK:
                {
                    ss<<"_lck ";
                    _ostream << ss.str();
                    break;
                }
                case op_ULOCK:
                {
                    ss<<"ulck";
                    _ostream << ss.str();
                    break;
                }
                case op_EXP:
                {
                    ss<<"exp ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_MOVG:
                {
                    ss<<"movg @"<< GET_Da(x64);
                    ss << " // @"; ss << find_class(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_MOVND:
                {
                    ss<<"movnd ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_NEWOBJARRAY:
                {
                    ss<<"newobj_arry ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_NOT: //c
                {
                    ss<<"not ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_SKIP:// d
                {
                    ss<<"skip @";
                    ss<< GET_Da(x64);
                    ss << " // pc = " << (x + GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_LOADVAL:
                {
                    ss<<"loadval ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_SHL:
                {
                    ss<<"shl ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    ss<< " -> ";
                    ss<< Asm::registrerToString(method->code.__asm64.get(++x));
                    _ostream << ss.str();
                    break;
                }
                case op_SHR:
                {
                    ss<<"shr ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    ss<< " -> ";
                    ss<< Asm::registrerToString(method->code.__asm64.get(++x));
                    _ostream << ss.str();
                    break;
                }
                case op_SKPE:
                {
                    ss<<"skpe ";
                    ss<<GET_Da(x64);
                    ss << " // pc = " << (x + GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_SKNE:
                {
                    ss<<"skne ";
                    ss<<GET_Da(x64);
                    ss << " // pc = " << (x + GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_CMP:
                {
                    ss<<"cmp ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_AND:
                {
                    ss<<"and ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_UAND:
                {
                    ss<<"uand ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_OR:
                {
                    ss<<"or ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_XOR:
                {
                    ss<<"xor ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_THROW:
                {
                    ss<<"throw ";
                    _ostream << ss.str();
                    break;
                }
                case op_CHECKNULL:
                {
                    ss<<"checknull";
                    _ostream << ss.str();
                    break;
                }
                case op_RETURNOBJ:
                {
                    ss<<"returnobj";
                    _ostream << ss.str();
                    break;
                }
                case op_NEWCLASSARRAY:
                {
                    ss<<"new_classarray ";
                    ss<< GET_Ca(x64);
                    ss<< " ";
                    ss << " // "; ss << find_class(GET_Cb(x64)) << "[]";
                    _ostream << ss.str();
                    break;
                }
                case op_NEWSTRING:
                {
                    ss<<"newstr @" << GET_Da(x64) << " // ";
                    ss << getString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_ADDL:
                {
                    ss<<"addl ";
                    ss<< Asm::registrerToString(GET_Ca(x64)) << ", @";
                    ss<<GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_SUBL:
                {
                    ss<<"subl ";
                    ss<< Asm::registrerToString(GET_Ca(x64)) << ", @";
                    ss<<GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_MULL:
                {
                    ss<<"mull ";
                    ss<< Asm::registrerToString(GET_Ca(x64)) << ", @";
                    ss<<GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_DIVL:
                {
                    ss<<"divl ";
                    ss<< Asm::registrerToString(GET_Ca(x64)) << ", @";
                    ss<<GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_MODL:
                {
                    ss<<"modl #";
                    ss<< Asm::registrerToString(GET_Ca(x64)) << ", @";
                    ss<<GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_IADDL:
                {
                    ss<<"iaddl ";
                    ss<< GET_Ca(x64) << ", @";
                    ss<<GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_ISUBL:
                {
                    ss<<"isubl #";
                    ss<< GET_Ca(x64) << ", @";
                    ss<<GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_IMULL:
                {
                    ss<<"imull #";
                    ss<< GET_Ca(x64) << ", @";
                    ss<<GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_IDIVL:
                {
                    ss<<"idivl #";
                    ss<< GET_Ca(x64) << ", @";
                    ss<<GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_IMODL:
                {
                    ss<<"imodl #";
                    ss<< GET_Ca(x64) << ", @";
                    ss<<GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_LOADL:
                {
                    ss<<"loadl ";
                    ss<< Asm::registrerToString(GET_Ca(x64)) << ", fp+";
                    ss<<GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_IALOAD_2:
                {
                    ss<<"iaload_2 ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_POPOBJ:
                {
                    ss<<"popobj";
                    _ostream << ss.str();
                    break;
                }
                case op_SMOVR:
                {
                    ss<<"smovr ";
                    ss<< Asm::registrerToString(GET_Ca(x64)) << ", sp+";
                    if(GET_Cb(x64)<0) ss<<"[";
                    ss<<GET_Cb(x64);
                    if(GET_Cb(x64)<0) ss<<"]";
                    _ostream << ss.str();
                    break;
                }
                case op_SMOVR_2:
                {
                    ss<<"smovr_2 ";
                    ss<< Asm::registrerToString(GET_Ca(x64)) << ", fp+";
                    if(GET_Cb(x64)<0) ss<<"[";
                    ss<<GET_Cb(x64);
                    if(GET_Cb(x64)<0) ss<<"]";
                    _ostream << ss.str();
                    break;
                }
                case op_ANDL:
                {
                    ss<<"andl ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_ORL:
                {
                    ss<<"orl ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_XORL:
                {
                    ss<<"xorl ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_RMOV:
                {
                    ss<<"rmov ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_SMOV:
                {
                    ss<<"smov ";
                    ss<< Asm::registrerToString(GET_Ca(x64)) << ", sp+";
                    if(GET_Cb(x64)<0) ss<<"[";
                    ss<<GET_Cb(x64);
                    if(GET_Cb(x64)<0) ss<<"]";
                    _ostream << ss.str();
                    break;
                }
                case op_LOADPC_2:
                {
                    ss<<"loadpc_2 ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", pc+";
                    ss<< GET_Cb(x64);
                    ss<< " // " << Asm::registrerToString(GET_Ca(x64))
                            << " = " << (x + GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_RETURNVAL:
                {
                    ss<<"return_val ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_ISTORE:
                {
                    ss<<"istore ";
                    ss<< GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_ISTOREL:
                {
                    ss<<"istorel ";
                    ss<< method->code.__asm64.get(++x) << ", fp+";
                    ss<<GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_IPUSHL:
                {
                    ss<<"ipushl #";
                    ss<< GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_PUSHL:
                {
                    ss<<"pushl ";
                    ss<< GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_PUSHNIL:
                {
                    ss<<"pushnil ";
                    _ostream << ss.str();
                    break;
                }
                case op_GET:
                {
                    ss<<"get ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_ITEST:
                {
                    ss<<"itest ";
                    ss<< Asm::registrerToString(GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_INVOKE_DELEGATE:
                {
                    ss<<"invoke_delegate ";
                    ss<< GET_Ca(x64);
                    ss<< ", ";
                    ss<< GET_Cb(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_INVOKE_DELEGATE_STATIC:
                {
                    ss<<"invoke_delegate_static ";
                    ss<< Asm::registrerToString(GET_Ca(x64));
                    ss<< ", ";
                    ss<< Asm::registrerToString(GET_Cb(x64));
                    _ostream << ss.str();
                    break;
                }
                case op_ISADD:
                {
                    ss<<"isadd ";
                    ss<< GET_Ca(x64) << ", sp+";
                    if(GET_Cb(x64)<0) ss<<"[";
                    ss<<GET_Cb(x64);
                    if(GET_Cb(x64)<0) ss<<"]";
                    _ostream << ss.str();
                    break;
                }
                case op_IPOPL:
                {
                    ss<<"ipopl ";
                    ss<< GET_Da(x64);
                    _ostream << ss.str();
                    break;
                }
                case op_SWITCH:
                {
                    ss<<"switch ";
                    ss<< GET_Da(x64);
                    ss<< " // " << getSwitchTable(method, GET_Da(x64));
                    _ostream << ss.str();
                    break;
                }
                default:
                    ss << "? (" << GET_OP(x64) << ")";
                    _ostream << ss.str();
                    break;
            }

            _ostream << "\n";
        }

        _ostream << '\n';
    }

    if(File::write((c_options.out + ".ojd").c_str(), _ostream)) {
        cout << progname << ": error: failed to write out to dump file " << c_options.out << endl;
    }
    _ostream.end();
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
    freeListPtr(classes);
    freeListPtr(generics);
    freeList(scopeMap);
    sourceFiles.free();
    lastNoteMsg.clear();
    exportFile.clear();
    noteMessages.free();
    inline_map.free();
    stringMap.free();
}

bool RuntimeEngine::isWholeNumber(double value) {
    if (floor(value) == value)
        return true;
    else
        return false;
}

void RuntimeEngine::verifyClassAccess(ClassObject *klass, Ast* pAst) {
    if(klass->getAccessModifier() == PRIVATE) {
        Scope *scope = currentScope();
        if(scope->klass->hasBaseClass(klass) || klass == scope->klass || klass == scope->klass->getSuperClass()) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to private class `" + klass->getFullName() + "`");
        }
    } else if(klass->getAccessModifier() == PROTECTED) {
        Scope *scope = currentScope();
        if(scope->klass->hasBaseClass(klass) || klass == scope->klass || klass == scope->klass->getSuperClass()
                || scope->klass->getModuleName() == klass->getModuleName()) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to protected class `" + klass->getFullName() + "`");
        }
    } else {
        // access granted
    }
}

void RuntimeEngine::verifyMethodAccess(Method *fn, Ast* pAst) {
    if(fn->hasModifier(PRIVATE)) {
        if(fn->getParentClass() == currentScope()->klass || fn->getParentClass() == currentScope()->klass->getSuperClass()) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to private method `" + fn->getFullName() + paramsToString(fn->getParams()) + "`");
        }
    } else if(fn->hasModifier(PROTECTED)) {
        if(currentScope()->klass->hasBaseClass(fn->getParentClass()) || fn->getParentClass() == currentScope()->klass || fn->getParentClass() == currentScope()->klass->getSuperClass()
                || currentScope()->klass->getModuleName() == fn->getParentClass()->getModuleName()) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to protected method `" + fn->getFullName() + paramsToString(fn->getParams()) + "`");
        }
    } else {
        // access granted
    }
}

void RuntimeEngine::verifyFieldAccess(Field *field, Ast* pAst) {
    if(field->modifiers.find(PRIVATE)) {
        if(field->owner == currentScope()->klass || field->owner == currentScope()->klass->getSuperClass()) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to private field `" + field->fullName + "`");
        }
    } else if(field->modifiers.find(PROTECTED)) {
        if(currentScope()->klass->hasBaseClass(field->owner) || field->owner == currentScope()->klass || field->owner == currentScope()->klass->getSuperClass()
           || currentScope()->klass->getModuleName() == field->owner->getModuleName()) {
        } else {
            errors->createNewError(GENERIC, pAst, " invalid access to protected method `" + field->fullName + "`");
        }
    } else {
        // access granted
    }
}

void RuntimeEngine::isMemoryObject(Expression &expression, Ast *pAst) {
    if(expression.newExpression || expression.isArray() || expression.trueType() == OBJECT
       || expression.trueType() == CLASS) {
    } else {
        errors->createNewError(GENERIC, pAst, "expression must evaluate to an object");
    }
}

void RuntimeEngine::parseIdentifierList(Ast *pAst, List<string> &idList) {
    pAst = pAst->getSubAst(ast_identifier_list);

    for(long i = 0; i < pAst->getEntityCount(); i++) {
        string Key = pAst->getEntity(i).getToken();
        if(Key != ",") {
            idList.push_back(Key);
        }
    }
}

void RuntimeEngine::parseUtypeList(Ast *pAst, List<Expression> &list) {

    for(long i = 0; i < pAst->getSubAstCount(); i++) {
        Expression utype = parseUtype(pAst->getSubAst(i));
        list.push_back(utype);
    }
}

int recursion = 0;
void RuntimeEngine::findAndCreateGenericClass(std::string module, string &klass, List<Expression> &utypes,
                                              ClassObject* parent, Ast *pAst) {
    for(long i = 0; i < utypes.size(); i++) {
        if(utypes.get(i).type == expression_unresolved || utypes.get(i).utype.type == UNDEFINED
            || utypes.get(i).utype.type == TYPEGENERIC) {
            return;
        }
    }

    recursion++;
    ClassObject *generic = parent != NULL ? parent->getChildClass(klass) : getClass(module, klass, generics);
    if(generic == NULL && parent == NULL) {
        generic = currentScope()->klass->getChildClass(klass);
    }

    if(generic != NULL) {
        if(utypes.size() == generic->genericKeySize()) {
            stringstream name;

            // build unique class name
            name << "<";
            for(long i = 0; i < utypes.size(); i++) {
                name << utypes.get(i).typeToString();

                if((i+1) < utypes.size()) {
                    name << ",";
                }
            }
            name << ">";
            klass = generic->getName() + name.str();
            ClassObject *klassLocation = parent != NULL ? parent->getChildClass(klass) : getClass(module, klass, classes);

            if(klassLocation == NULL) {

                List<AccessModifier> modifiers;
                modifiers.add(generic->getAccessModifier());
                ClassObject *newClass = NULL;

                if(parent == NULL)
                    newClass = addGlobalClassObject(klass, modifiers, pAst);
                else {
                    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(pAst->line),
                                                   pAst->line, pAst->col);

                    newClass = new ClassObject(klass, currentModule, this->uniqueSerialId++,modifiers.get(0), note);
                    if(!parent->addChildClass(newClass)) {
                        return;
                    }
                }

                // traverse class
                RuntimeNote note = newClass->note;

                // set up error system for temporary convinent error reporting
                string file = errors->filname;
                List<string> oldLines;
                oldLines.addAll(errors->lines);

                for(long i = 0; i < parsers.size(); i++) {
                    if(parsers.get(i)->sourcefile == generic->note.getFile()) {
                        errors->lines.addAll(parsers.get(i)->lines);
                    }
                }

                *newClass = *generic;

                errors->filname = newClass->note.getFile();
                newClass->note = note;
                newClass->setFullName(newClass->getFullName() + name.str());
                newClass->setName(klass);
                newClass->setIsProcessed(true);

                long long currErrors = errors->getUnfilteredErrorCount() ,newErrors;

                addScope(Scope(CLASS_SCOPE, newClass));
                traverseGenericClass(newClass, utypes, pAst);
                removeScope();

                analyzeGenericClass(newClass);

                newClass->setIsGeneric(false);
                errors->lines.addAll(oldLines);
                errors->filname = file;
                newErrors = errors->getUnfilteredErrorCount()-currErrors;

                if(newErrors > 0) {
                    // this helps the user find where they went wrong
                    printNote(newClass->note, "in generic `" + newClass->getName() + "`");
                }
            }
        } // parseUtype() will handle unresolved error
    } // parseUtype() will handle unresolved error

    recursion--;
}

void RuntimeEngine::traverseGenericClass(ClassObject *klass, List<Expression> &utypes, Ast* pAst) {
    klass->setAddress(this->classSize++);
    for(long i = 0; i < utypes.size(); i++) {
        klass->addGenericType(&utypes.get(i));
    }

    for(long i = 0; i < klass->constructorCount(); i++) {
        Method *constr = klass->getConstructor(i);
        traverseMethod(klass, constr, pAst);
    }

    for(long i = 0; i < klass->functionCount(); i++) {
        Method *func = klass->getFunction(i);
        traverseMethod(klass, func, pAst);
    }

    for(long i = 0; i < klass->overloadCount(); i++) {
        Method *func = klass->getOverload(i);
        traverseMethod(klass, func, pAst);
    }

    for(long i = 0; i < klass->fieldCount(); i++) {
        Field *field = klass->getField(i);
        traverseField(klass, field, pAst);
    }

    for(long i = 0; i < klass->childClassCount(); i++) {
        ClassObject *child = klass->getChildClass(i);
        traverseGenericClass(child, utypes, pAst);
    }

    for(long i = 0; i < klass->interfaceCount(); i++) {
        ClassObject *child = klass->getInterface(i);
        traverseGenericClass(child, utypes, pAst);
    }
}

void RuntimeEngine::traverseMethod(ClassObject *klass, Method *func, Ast* pAst) {
    func->address = methods++;
    func->owner=klass;

    if(func->type == TYPEGENERIC) {
        Expression* utype = klass->getGenericType(func->key);

        if(utype == NULL)
            errors->createNewError(GENERIC, pAst, "I screwed up traversing your generic class please let me know!!");
        else {
            bool arry = func->array;
            parseMethodReturnType(*utype, *func);

            if(arry && !utype->utype.array) {
                func->array = true;
                // we are fine field will just stay an array
            } else if(arry && utype->utype.array) {
                // error
                errors->createNewError(GENERIC, pAst, "Array-arrays are not supported.");
            } else if(!arry) {
            }
        }
    }

    List<Param> *params = &func->getParams();
    for(long i = 0; i < params->size(); i++) {
        Field *field = &params->get(i).field;
        traverseField(klass, field, pAst);
    }
}

void RuntimeEngine::traverseField(ClassObject *klass, Field *field, Ast* pAst) {
    if(field->type != TYPEGENERIC) {
        field->owner = klass;
        return;
    }
    field->owner=klass;

    Expression* utype = klass->getGenericType(field->key);
    Expression expr(NULL);

    if(field->klass != NULL) {
        List<Expression> utypes;
        expr = parseUtype(field->ast);
        utype = &expr;

        if(utype->type == expression_unresolved)
            return;
        utypes.push_back(*utype);
        string typeName = expr.utype.klass->getName();
        findAndCreateGenericClass(field->klass->getModuleName(), typeName, utypes, NULL, pAst);
    }

    if(utype == NULL) {
       // field->type = utype->utype.type;
        errors->createNewError(GENERIC, pAst, "I screwed up traversing your generic class please let me know!!");
    }
    else {

        field->owner = klass;
        field->type = utype->utype.type;
        if(utype->utype.type == CLASS) {
            field->klass = utype->utype.klass;
        } else if(utype->utype.type == TYPEVOID) {
            field->key = utype->utype.referenceName;
        }

        if(field->isArray && !utype->utype.array) {
            // we are fine field will just stay an array
        } else if(field->isArray && utype->utype.array) {
            // error
            errors->createNewError(GENERIC, pAst, "Array-arrays are not supported.");
        } else if(!field->isArray) {
            field->isArray = utype->utype.array;
        }
    }
}

void RuntimeEngine::analyzeGenericClass(ClassObject *generic)
{
    Ast *ast = generic->getAst();
    Ast* block = ast->getLastSubAst();
    int startpos = 1;
    List<AccessModifier> modifiers;

    addScope(Scope(CLASS_SCOPE, generic));
    parseAccessDecl(generic->getAst(), modifiers, startpos);
    ClassObject *base = parseBaseClass(ast, ++startpos);

    if(base != NULL && base->isInterface() && !generic->isInterface()) {
        stringstream err;
        err << "classes can only inherit other classes, do 'class Dog base Animal : Traits {} ' instead";
        errors->createNewError(GENERIC, ast->line, ast->col, err.str());
    } else {
        if(base != NULL)
            generic->setBaseClass(base->getSerial() == generic->getSerial() ? NULL : base);
        else {
            generic->setBaseClass(getClass("std", "Object", classes));
        }
    }

    if(ast->hasSubAst(ast_reference_identifier_list)) {
        List<ClassObject*> interfaces = parseRefrenceIdentifierList(ast->getSubAst(ast_reference_identifier_list));

        for(long i = 0; i < interfaces.size(); i++) {
            if(interfaces.get(i) != NULL && !interfaces.get(i)->isInterface()) {
                stringstream err;
                err << "class `" + interfaces.get(i)->getName() + "` is not an interface";
                errors->createNewError(GENERIC, ast->line, ast->col, err.str());
            }
        }
        generic->setInterfaces(interfaces);
    }

    for(long i = 0; i < block->getSubAstCount(); i++) {
        ast = block->getSubAst(i);
        CHECK_ERRORS

        switch(ast->getType()) {
            case ast_class_decl:
                analyzeClassDecl(ast);
                break;
            case ast_var_decl:
                analyzeVarDecl(ast);
                break;
            case ast_method_decl:
                parseMethodDecl(ast);
                break;
            case ast_operator_decl:
                parseOperatorDecl(ast);
                break;
            case ast_construct_decl:
                parseConstructorDecl(ast);
                break;
            case ast_delegate_post_decl: /* ignore */
                break;
            case ast_delegate_decl:
                parseMethodDecl(ast, true);
                break;
            case ast_generic_class_decl: /* ignore */
                break;
            default: {
                stringstream err;
                err << ": unknown ast type: " << ast->getType();
                errors->createNewError(INTERNAL_ERROR, ast->line, ast->col, err.str());
                break;
            }
        }
    }
    removeScope();

}

void RuntimeEngine::resolveAllEnums() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        activeParser = parsers.get(i);
        errors = new ErrorManager(activeParser->lines, activeParser->sourcefile, true, c_options.aggressive_errors);
        currentModule = "$unknown";

        addScope(Scope(GLOBAL_SCOPE, NULL));
        for(int x = 0; x < activeParser->treesize(); x++) {
            Ast* ast = activeParser->ast_at(x);
            SEMTEX_CHECK_ERRORS

            if(x==0) {
                if(ast->getType() == ast_module_decl) {
                    add_module(currentModule = parseModuleName(ast));
                    continue;
                }
            }

            switch(ast->getType()) {
                case ast_enum_decl:
                    resolveEnumDecl(ast);
                    break;
                case ast_class_decl:
                    resolveClassEnumDecl(ast);
                    break;
                default:
                    /* ignore */
                    break;
            }
        }

        if(errors->hasErrors()){
            report:

            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            succeededParsers.addif(activeParser->sourcefile);
            failedParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

double RuntimeEngine::constantExpressionToValue(Ast *pAst, Expression &constExpr) {
    if(constExpr.literal) {
        return constExpr.intValue;
    } else if(constExpr.isEnum()) {
        return getInlinedFieldValue(&constExpr.utype.field);
    } else if(constExpr.isConstExpr()) {
        return constExpr.utype.field.constant_value;
    } else {
        errors->createNewError(GENERIC, pAst, "could not get the immutable value from expression of type `" + constExpr.typeToString() + "`");
    }
    return 0;
}

bool RuntimeEngine::isExpressionConvertableToNativeClass(Field *f, Expression &exp) {
    if(f->type==CLASS && f->klass->getModuleName() == "std" &&
           (f->klass->getName() == "int" || f->klass->getName() == "bool"
            || f->klass->getName() == "char" || f->klass->getName() == "long"
            || f->klass->getName() == "short" || f->klass->getName() == "string"
            || f->klass->getName() == "uchar" || f->klass->getName() == "ulong"
            || f->klass->getName() == "ushort")) {
        if(f->klass->getName() == "string") {
            return exp.type == expression_string || (exp.trueType() == VAR && exp.isArray());
        } else {
            return (exp.trueType() == VAR && !exp.isArray());
        }
    }
    return false;
}

bool RuntimeEngine::isNativeIntegerClass(ClassObject *klass) {
    if(klass == NULL) return false;
    return (klass->getModuleName() == "std" &&
            (klass->getName() == "int" || klass->getName() == "short"    ||
             klass->getName() == "bool" || klass->getName() == "long"    ||
             klass->getName() == "char" || klass->getName() == "uint"    ||
             klass->getName() == "ushort" || klass->getName() == "ubool" ||
             klass->getName() == "ulong" || klass->getName() == "uchar"));
}

void RuntimeEngine::resolveAllGenericMethodsParams() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        activeParser = parsers.get(i);
        errors = new ErrorManager(activeParser->lines, activeParser->sourcefile, true, c_options.aggressive_errors);
        currentModule = "$unknown";

        addScope(Scope(GLOBAL_SCOPE, NULL));
        for(int x = 0; x < activeParser->treesize(); x++) {
            Ast* ast = activeParser->ast_at(x);
            SEMTEX_CHECK_ERRORS

            if(x==0) {
                if(ast->getType() == ast_module_decl) {
                    add_module(currentModule = parseModuleName(ast));
                    continue;
                }
            }

            switch(ast->getType()) {
                case ast_class_decl:
                    resolveClassDecl(ast, false);
                    break;
                case ast_interface_decl:
                    resolveClassDecl(ast, false);
                    break;
                case ast_generic_class_decl:
                case ast_generic_interface_decl:
                    resolveGenericClassDecl(ast, true);
                    break;
                case ast_enum_decl: /* ignore */
                    break;
                default:
                    /* ignore */
                    break;
            }
        }

        if(errors->hasErrors()){
            report:

            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            succeededParsers.addif(activeParser->sourcefile);
            failedParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void RuntimeEngine::inheritObjectClass() {
    ClassObject *objectClass = getClass("std", "Object", classes);

    if(objectClass != NULL) {
        for(size_t i = 0; i < classes.size(); i++) {
            if(classes.get(i)->getBaseClass() == NULL) {
                if(classes.get(i)->getModuleName() == "std"
                   && classes.get(i)->getName() == "Object") {}
                else {
                    classes.get(i)->setBaseClass(objectClass);
                }
            }
        }
    }
}

void RuntimeEngine::resolveAllGenericMethodsReturns() {
    for(unsigned long i = 0; i < parsers.size(); i++) {
        activeParser = parsers.get(i);
        errors = new ErrorManager(activeParser->lines, activeParser->sourcefile, true, c_options.aggressive_errors);
        currentModule = "$unknown";

        addScope(Scope(GLOBAL_SCOPE, NULL));
        for(int x = 0; x < activeParser->treesize(); x++) {
            Ast* ast = activeParser->ast_at(x);
            SEMTEX_CHECK_ERRORS

            if(x==0) {
                if(ast->getType() == ast_module_decl) {
                    add_module(currentModule = parseModuleName(ast));
                    continue;
                }
            }

            switch(ast->getType()) {
                case ast_class_decl:
                    resolveAllGenericMethodsReturns(ast);
                    break;
                case ast_interface_decl:
                    resolveAllGenericMethodsReturns(ast);
                    break;
                case ast_generic_class_decl:
                case ast_generic_interface_decl:
                    break;
                case ast_enum_decl: /* ignore */
                    break;
                default:
                    /* ignore */
                    break;
            }
        }

        if(errors->hasErrors()){
            report:

            errorCount+= errors->getErrorCount();
            unfilteredErrorCount+= errors->getUnfilteredErrorCount();

            failedParsers.addif(activeParser->sourcefile);
            succeededParsers.removefirst(activeParser->sourcefile);
        } else {
            succeededParsers.addif(activeParser->sourcefile);
            failedParsers.removefirst(activeParser->sourcefile);
        }

        errors->free();
        delete (errors); this->errors = NULL;
        removeScope();
    }
}

void RuntimeEngine::resolveAllGenericMethodsReturns(Ast *ast) {
    Ast* block = ast->getSubAst(ast_block), *trunk;
    List<AccessModifier> modifiers;
    ClassObject* klass;
    int startpos=1;

    parseAccessDecl(ast, modifiers, startpos);
    string name =  ast->getEntity(startpos).getToken();

    if(currentScope()->type == GLOBAL_SCOPE) {
        klass = getClass(currentModule, name, classes);
    }
    else {
        klass = currentScope()->klass->getChildClass(name);
    }

    long operatorCount=0, constructorCount=0, methodCount=0;
    addScope(Scope(CLASS_SCOPE, klass));
    for(long i = 0; i < block->getSubAstCount(); i++) {
        trunk = block->getSubAst(i);
        CHECK_ERRORS

        switch(trunk->getType()) {
            case ast_class_decl:
                resolveAllGenericMethodsReturns(trunk);
                break;
            case ast_var_decl: /* ignore */
                break;
            case ast_method_decl:
                resolveGenericMethodsReturn(trunk, operatorCount, constructorCount, methodCount, _method);
                break;
            case ast_operator_decl:
                resolveGenericMethodsReturn(trunk, operatorCount, constructorCount, methodCount, _operator);
                break;
            case ast_construct_decl:
                resolveGenericMethodsReturn(trunk, operatorCount, constructorCount, methodCount, _constructor);
                break;
            case ast_delegate_post_decl:
                resolveGenericMethodsReturn(trunk, operatorCount, constructorCount, methodCount, _method);
                break;
            case ast_delegate_decl:
                resolveGenericMethodsReturn(trunk, operatorCount, constructorCount, methodCount, _method);
                break;
            case ast_interface_decl:
                resolveAllGenericMethodsReturns(trunk);
                break;
            case ast_generic_class_decl: /* ignore */
                break;
            case ast_enum_decl: /* ignore */
                break;
            case ast_func_prototype: /* ignore */
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << trunk->getType();
                errors->createNewError(INTERNAL_ERROR, trunk->line, trunk->col, err.str());
                break;
        }
    }

    removeScope();
}

void RuntimeEngine::resolveGenericMethodsReturn(Ast *ast, long &operators, long &constructors, long &methods, method_type type) {
    List<AccessModifier> modifiers;
    int startpos;

    switch(type) {
        case _method:
            startpos = 1;
            break;
        case _operator:
            startpos=2;
            break;
        case _constructor:
            startpos = 0;
            break;
    }

    if(parseAccessDecl(ast, modifiers, startpos)){
        parseMethodAccessModifiers(modifiers, ast);
    } else {
        modifiers.add(PRIVATE);
    }

    List<Param> params;
    string name =  ast->getEntity(startpos).getToken();
    parseMethodParams(params, parseUtypeArgList(ast->getSubAst(ast_utype_arg_list)), ast->getSubAst(ast_utype_arg_list));

    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    Expression utype(ast);
    Method *method = NULL;

    switch(type) {
        case _method:
            if(methods >= currentScope()->klass->functionCount())
                return;

            method = currentScope()->klass->getFunction(methods++);
            break;
        case _operator:
            if(operators >= currentScope()->klass->overloadCount())
                return;

            method = currentScope()->klass->getOverload(operators++);
            break;
        case _constructor:
            if(constructors >= currentScope()->klass->constructorCount())
                return;

            method = currentScope()->klass->getConstructor(constructors++);
            break;
    }

    if(method != NULL && method->type == TYPEGENERIC) {
        utype = parseUtype(ast->getSubAst(ast_method_return_type));
        parseMethodReturnType(utype, *method);
    }

}

void RuntimeEngine::parseProtypeDecl(Ast *ast) {
    List<AccessModifier> modifiers;
    int startpos=0;


    if(parseAccessDecl(ast, modifiers, startpos)){
        parseVarAccessModifiers(modifiers, ast);
    } else {
        modifiers.add(PRIVATE);
    }

    string name =  ast->getEntity(++startpos).getToken();
    RuntimeNote note = RuntimeNote(activeParser->sourcefile, activeParser->getErrors()->getLine(ast->line),
                                   ast->line, ast->col);

    if(!currentScope()->klass->addField(Field(NULL, uniqueSerialId++, name, NULL, modifiers, note))) {
        this->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                     "field `" + name + "` is already defined in the scope");
        printNote(note, "field `" + name + "` previously defined here");
    }
}
