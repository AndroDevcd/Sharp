//
// Created by bknun on 9/12/2017.
//

#ifndef SHARP_RUNTIME_H
#define SHARP_RUNTIME_H

#include "../../stdimports.h"
#include "List.h"
#include "parser/Parser.h"
#include "ClassObject.h"

class RuntimeEngine {
public:
    RuntimeEngine(const string exportFile, List<Parser*> &parsers)
    :
            exportFile(exportFile),
            modules(),
            parsers(),
            classSerialId(0)
    {
        this->parsers.addAll(parsers);

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

    static unsigned long classSerialId;
private:
    List<Parser*> parsers;
    List<string> modules;
    string exportFile;
    ErrorManager* errors;
    Parser* activeParser;
    string currentModule;

    void compile();
    bool preprocess();

    bool module_exists(string name);
    void add_module(string name);
    string astToString(Ast *ast);
    string getModuleName(Ast *ast);

    void processClassDecl(Ast *ast, ClassObject *parent);
};

#define progname "bootstrap"
#define progvers "0.2.6"

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
