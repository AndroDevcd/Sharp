//
// Created by bknun on 9/12/2017.
//

#ifndef SHARP_RUNTIME_H
#define SHARP_RUNTIME_H

#include "../../stdimports.h"
#include "List.h"
#include "parser/Parser.h"
#include "ClassObject.h"

struct Scope;
class ReferencePointer;
class ResolvedReference;

class RuntimeEngine {
public:
    RuntimeEngine(const string exportFile, List<Parser*> &parsers)
    :
            exportFile(exportFile),
            modules(),
            parsers(),
            failedParsers(),
            succeededParsers(),
            sourceFiles(),
            scopeMap(),
            classes(),
            errorCount(0),
            unfilteredErrorCount(0),
            importMap(),
            noteMessages(),
            resolvedFields(false),
            classSize(0)
    {
        this->parsers.addAll(parsers);
        uniqueSerialId = 0;

        for(int i = 0; i < parsers.size(); i++)
        {
            Parser *p = parsers.get(i);
            if(!p->parsed) {
                return;
            }
        }

        compile();
    }

    template<class T>
    static void freeList(List<T> &lst)
    {
        for(unsigned int i = 0; i < lst.size(); i++)
        {
            lst.get(i).free();
        }
        lst.free();
    }

    template<class T>
    static void freeList(list<T> &lst)
    {
        for(T item : lst)
        {
            item.free();
        }
        lst.clear();
    }

    List<string> failedParsers;
    List<string> succeededParsers;
    static unsigned long uniqueSerialId;
    long errorCount, unfilteredErrorCount;

    void generate();

    void cleanup();

private:
    List<Parser*> parsers;
    List<string> modules;
    List<string> sourceFiles;
    List<Scope> scopeMap;
    List<ClassObject> classes;
    List<keypair<string, List<string>>>  importMap;
    string exportFile;
    ErrorManager* errors;
    Parser* activeParser;
    string currentModule;
    bool resolvedFields;
    unsigned long classSize;

    /* One off variables */
    RuntimeNote lastNote;
    string lastNoteMsg;
    List<string> noteMessages;
    int64_t i64;

    void compile();

    bool preprocess();

    bool module_exists(string name);

    void add_module(string name);

    string parseModuleName(Ast *ast);

    void parseClassDecl(Ast *ast);

    Scope* addScope(Scope scope);

    bool isTokenAccessDecl(token_entity token);

    AccessModifier entityToModifier(token_entity entity);

    list<AccessModifier> parseAccessModifier(Ast *ast);

    bool parseAccessDecl(Ast *ast, List<AccessModifier> &modifiers, int &startpos);

    void parseClassAccessModifiers(List<AccessModifier> &modifiers, Ast *ast);

    ClassObject *addGlobalClassObject(string name, List<AccessModifier> &modifiers, Ast *pAst);

    bool addClass(ClassObject klass);

    bool classExists(string module, string name);

    void printNote(RuntimeNote &note, string msg);

    ClassObject *getClass(string module, string name);

    ClassObject *addChildClassObject(string name, List<AccessModifier> &modifiers, Ast *ast, ClassObject *super);

    void removeScope();

    void parseVarDecl(Ast *ast);

    void parseVarAccessModifiers(List<AccessModifier> &modifiers, Ast *ast);

    int parseVarAccessSpecifiers(List<AccessModifier> &modifiers);

    void resolveAllFields();

    void resolveClassDecl(Ast *ast);

    ReferencePointer parseReferencePtr(Ast *ast);

    ResolvedReference resolveReferencePointer(ReferencePointer &ptr);

    ClassObject *tryClassResolve(string moduleName, string name);

    ClassObject *resolveClassRefrence(Ast *ast, ReferencePointer &ptr);

    bool expectReferenceType(ResolvedReference refrence, FieldType expectedType, bool method, Ast *ast);

    ClassObject *parseBaseClass(Ast *ast, int startpos);
};

class ReferencePointer {
public:
    ReferencePointer() {
        classHeiarchy.init();
        module = "";
        referenceName = "";
    }

    void operator=(ReferencePointer ptr) {
        this->module = ptr.module;
        this->referenceName = ptr.referenceName;

        this->classHeiarchy.addAll(ptr.classHeiarchy);
    }

    void free() {
        classHeiarchy.free();
        referenceName.clear();
        module.clear();
    }

    bool singleRefrence() {
        return module == "" && classHeiarchy.size() == 0;
    }

    bool singleRefrenceModule() {
        return module != "" && classHeiarchy.size() == 0;
    }

    void print() {
        // dev code
        cout << "refrence pointer -----" << endl;
        cout << "id: " << referenceName << endl;
        cout << "mod: " << module << endl;
        cout << "class: ";
        for(int i = 0; i < classHeiarchy.size(); i++)
            cout << classHeiarchy.at(i) << ".";
        cout << endl;
    }

    string module;
    List<string> classHeiarchy;
    string referenceName;

    string toString() {
        stringstream ss;
        if(module != "")
            ss << module << "#";
        for(int i = 0; i < classHeiarchy.size(); i++)
            ss << classHeiarchy.at(i) << ".";
        ss << referenceName << endl;
        return ss.str();
    }
};

struct BranchTable {
    BranchTable()
            :
            branch_pc(0),
            line(0),
            col(0),
            store(false),
            registerWatchdog(0),
            offset(0),
            labelName("")
    {
    }

    int64_t branch_pc;          // where was the branch initated in the code
    string labelName;           // the label we were trying to access
    int line, col;

    bool store;                 // is this a store instruction/
    int registerWatchdog;      // if this is a store instruction tell me what register to put the data in
    long offset;                // any offset to the address label

    void free() {
        labelName.clear();
    }
};

enum ScopeType {
    GLOBAL_SCOPE,
    CLASS_SCOPE,
    INSTANCE_BLOCK,
    STATIC_BLOCK
};

struct Scope {
    Scope()
            :
            type(GLOBAL_SCOPE),
            klass(NULL),
            self(false),
            base(false),
            currentFunction(NULL),
            blocks(0),
            loops(0),
            trys(0),
            uniqueLabelSerial(0),
            reachable(true),
            last_statement(0)
    {
        locals.init();
        label_map.init();
        branches.init();
    }

    Scope(ScopeType type, ClassObject* klass)
            :
            type(type),
            klass(klass),
            self(false),
            base(false),
            currentFunction(NULL),
            blocks(0),
            loops(0),
            trys(0),
            uniqueLabelSerial(0),
            reachable(true),
            last_statement(0)
    {
        locals.init();
        label_map.init();
        branches.init();
    }

    Scope(ScopeType type, ClassObject* klass, Method* func)
            :
            type(type),
            klass(klass),
            self(false),
            base(false),
            currentFunction(func),
            blocks(0),
            loops(0),
            trys(0),
            uniqueLabelSerial(0),
            reachable(true),
            last_statement(0)
    {
        locals.init();
        label_map.init();
        branches.init();
    }

    keypair<int, Field>* getLocalField(string field_name) {
        if(locals.size() == 0) return NULL;

        for(long long i = locals.size()-1; i >= 0; i--) {
            if(locals.at(i).value.name == field_name) {
                return &locals.get(i);
            }
        }
        return NULL;
    }

    int getLocalFieldIndex(string field_name) {
        for(long long i = locals.size()-1; i >= 0; i--) {
            if(locals.at(i).value.name == field_name) {
                return i;
            }
        }
        return -1;
    }

    int64_t getLabel(std::string name) {
        for(unsigned int i = 0; i < label_map.size(); i++) {
            if(label_map.get(i).key == name)
                return label_map.get(i).value;
        }
        return -1;
    }

    void addBranch(string label, long offset, Assembler& assembler, int line, int col) {
        BranchTable bt;
        assembler.__asm64.add(0);               // add empty instruction for branch later
        bt.branch_pc = assembler.__asm64.size()-1;
        bt.line=line;
        bt.col=col;
        bt.labelName = label;
        bt.offset=offset;
        branches.push_back(bt);
    }

    void addStore(string label, int _register, long offset, Assembler& assembler, int line, int col) {
        BranchTable bt;
        bt.branch_pc=assembler.__asm64.size();
        assembler.__asm64.add(0);               // add empty instruction for storing later
        assembler.__asm64.add(0);
        bt.line=line;
        bt.col=col;
        bt.labelName = label;
        bt.store=true;
        bt.offset = offset;
        bt.registerWatchdog=_register;
        branches.push_back(bt);
    }

    ScopeType type;
    ClassObject* klass;
    Method* currentFunction;
    List<keypair<int, Field>> locals;
    List<keypair<std::string, int64_t>> label_map;
    List<BranchTable> branches;
    int blocks;
    long loops, trys, uniqueLabelSerial, last_statement;
    bool self, base, reachable;

    void free() {
        locals.free();
        label_map.free();
    }

    void removeLocals(int block) {
        if(locals.size() == 0) return;

        readjust:
        for(long long i = locals.size()-1; i >= 0; i--) {
            if(locals.at(i).key==block) {
                locals.remove(i);
                goto readjust;
            }
        }
    }

    void removeLocals(string name) {
        if(locals.size() == 0) return;

        readjust:
        for(long long i = locals.size()-1; i >= 0; i--) {
            if(locals.at(i).value.name==name) {
                locals.remove(i);
                return;
            }
        }
    }
};

class ResolvedReference {
public:
    ResolvedReference()
            :
            type(UNDEFINED),
            field(NULL),
            method(NULL),
            klass(NULL),
            oo(NULL),
            referenceName(""),
            array(false),
            resolved(false),
            isMethod(false)
    {
    }

    static string typeToString(FieldType type) {
        if(type==CLASS)
            return "class";
        else if(type==OBJECT)
            return "object";
        else if(type==VAR)
            return "var";
        else if(type==TYPEVOID)
            return "void";
        else if(type==UNDEFINED)
            return "undefined";
        else
            return "unresolved";
    }

    string typeToString() {
        if(isMethod)
            return "method";
        else if(type==CLASS)
            return "class";
        else if(type==OBJECT)
            return "object";
        else if(type==VAR)
            return "var";
        else if(type==TYPEVOID)
            return "void";
        else if(type==UNDEFINED)
            return "undefined";
        else
            return "unresolved";
    }

    string referenceName;
    bool array, isMethod, resolved;
    FieldType type;
    ClassObject* klass;
    Field* field;
    Method* method;
    OperatorOverload* oo;
};

#define currentScope() (scopeMap.empty() ? NULL : &scopeMap.last())


#define progname "bootstrap"
#define progvers "0.2.10"

struct options {
    ~options()
    {
        out.clear();
        vers.clear();
    }

    /*
     * Activate aggressive error reporting for the compiler.
     */
    bool aggressive_errors = false;

    /*
     * Only compile all the files
     */
    bool compile = false;

    /*
     * Output file to write to
     */
    string out = "out";

    /*
     * Application version
     */
    string vers = "1.0";

    /*
     * Disable warnings
     */
    bool warnings = true;

    /*
     * Optimize code
     */
    bool optimize = false;

    /*
     * Set code to be debuggable (flag only used in manifest creation)
     */
    bool debug = true;

    /*
     * Strip debugging info (if-applicable)
     */
    bool strip = false;

    /*
     * Enable warnings as errors
     */
    bool werrors = false;

    /*
     * Allow unsafe code
     */
    bool unsafe = false;

    /*
     * Easter egg to enable magic mode
     */
    bool magic = false;

    /*
     * Easter egg to enable debug mode
     *
     * Allows you to see a little information
     * on what the compiler is doing
     */
    bool debugMode = false;

    /*
     * Dump object code
     */
    bool objDump = false;

    /*
     * Maximum errors the compiler will allow
     */
    unsigned long error_limit = 1000;

    /*
     * Machine platform target to run on
     */
    int target = versions.ALPHA;
};

int _bootstrap(int argc, const char* argv[]);
void rt_error(string message);
void printVersion();
std::string to_lower(string s);
bool all_integers(string int_string);
void exec_runtime(List<string>& files);

extern options c_options;
#define opt(v) strcmp(argv[i], v) == 0

#endif //SHARP_RUNTIME_H
