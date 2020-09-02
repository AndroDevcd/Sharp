//
// Created by BraxtonN on 10/6/2019.
//

#include <cmath>
#include <sys/stat.h>
#include <dirent.h>
#include "main.h"
#include "frontend/tokenizer/tokenizer.h"
#include "frontend/parser/Parser.h"
#include "backend/Compiler.h"
#include "../runtime/symbols/string.h"
#include "../util/zip/zlib.h"


/**
 * This array represents the map of all the warning types that are enabled/disabled
 * once a warning is fired it will refrence this map to check if it should fired
 *
 * all warnings will always be enabled by default
 */
bool warning_map[] = {
        true,    // general warnings
        true,    // waccess
        true,   // wambig
        true,   // wdecl
        true,   // wmain
        true,   // wcast
        true    // winit
};

options c_options;
void compile(List<native_string>&);

bool ends_with(std::string value, std::string ending)
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
                files.__new().init();
                files.last() = file;
            }
        }
        closedir (dir);
    } else {
        /* could not open directory */
        cout << "warning: could not find library files in path `" << path.str() << "`" << endl;
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
    cout <<               "    -s                strip debugging info and unused classes"               << endl;
    cout <<               "    -O                optimize the output code"                              << endl;
    cout <<               "    -obf              obfuscate user code"                                   << endl;
    cout <<               "    -L<path>          library directory path"                                << endl;
    cout <<               "    -w                disable all warnings"                                  << endl;
    cout <<               "    -errlmt<count>    set max errors the compiler allows before quitting"    << endl;
    cout <<               "    -v<version>       set the application version"                           << endl;
    cout <<               "    -unsafe -u        allow unsafe code"                                     << endl;
    cout <<               "    -nativedir -nd <dir> set output native code directory"                   << endl;
    cout <<               "    -target           target the specified platform of sharp to run on"      << endl;
    cout <<               "    -release -r       generate a release build exe"                          << endl;
    cout <<               "    --hw              display help message for warning options"              << endl;
    cout <<               "    --h -?            display this help message"                             << endl;
}

void help_warn() {
    cout << "Usage: sharpc " << "{OPTIONS} SOURCE FILE(S)" << std::endl;
    cout << "Source file must have a .sharp extion to be compiled.\n" << endl;
    cout << "Please note that not all warnings will be able to be disabled individually.\n" << endl;
    cout << "[-options]\n\n    -w                disable all warnings"                                  << endl;
    cout <<               "    -winit            disable class initialization warnings"                 << endl;
    cout <<               "    -waccess          disable access modifier warnings (public, static, etc.)" << endl;
    cout <<               "    -wambig           disable ambiguous symbol warnings"                     << endl;
    cout <<               "    -wdecl            disable object declaration warnings"                   << endl;
    cout <<               "    -wmain            disable multiple main method warnings"                 << endl;
    cout <<               "    -wcast            disable type cast warnings"                            << endl;
    cout <<               "    -werror           enable warnings as errors"                             << endl;
    cout <<               "    --hw              display this help message"                             << endl;
}

void error(string message) {
    cout << "sharpc:  error: " << message << endl;
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

bool startsWith(string &str, string prefix)
{
    if(prefix.size() > str.size())
        return false;

    long index = 0;
    for(char ch : prefix) {
        if(ch != str.at(index++))
            return false;
    }
    return true;
}

int _bootstrap(int argc, const char* argv[])
{
    if(argc < 2) {
        help();
        return 1;
    }


    initalizeErrors();
    List<native_string> files;
    for (int i = 1; i < argc; ++i) {
        args_:
        string arg(argv[i]);
        if(opt("-a")){
            c_options.aggressive_errors = true;
        }
        else if(opt("-c")){
            c_options.compile = true;
        }
        else if(opt("-o")){
            if(i+1 >= argc)
                error("output file required after option `-o`");
            else
                c_options.out = string(argv[++i]);
        }
        else if(opt("-nativedir") || opt("-nd")){
            if(i+1 >= argc)
                error("output directory required after option `" + arg + "`");
            else
                c_options.nativeCodeDir  = string(argv[++i]);
        }
        else if(opt("-L")){
            if(i+1 >= argc)
                error("library directory required after option `-L`");
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
        else if(opt("-obf")){
            c_options.obfuscate = true;
        }
        else if(opt("--h") || opt("-?")){
            help();
            exit(0);
        }
        else if(opt("--hw")){
            help_warn();
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
                error("file version required after option `-target`");
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
                        error("unknown target " + x);
                    }
                }
            }
        }
        else if(opt("-w")){
            warning_map[__WGENERAL] = false;
        }
        else if(opt("-waccess")){
            warning_map[__WACCESS] = false;
        }
        else if(opt("-wambig")){
            warning_map[__WAMBIG] = false;
        }
        else if(opt("-wdecl")){
            warning_map[__WDECL] = false;
        }
        else if(opt("-wmain")){
            warning_map[__WMAIN] = false;
        }
        else if(opt("-wcast")){
            warning_map[__WCAST] = false;
        }
        else if(opt("-winit")){
            warning_map[__WINIT] = false;
        }
        else if(opt("-v")){
            if(i+1 >= argc)
                error("file version required after option `-v`");
            else
                c_options.vers = string(argv[++i]);
        }
        else if(opt("-werror")){
            c_options.werrors = true;
            warning_map[__WGENERAL] = true;
        }
        else if(opt("-errlmt")) {
            if(i+1 >= argc)
                error("error limit required after option `-errlmt`");
            else {
                std::string lmt = std::string(argv[++i]);
                if (all_integers(lmt)) {
                    c_options.error_limit = strtoul(lmt.c_str(), NULL, 0);

                    if (c_options.error_limit > 100000) {
                        error("cannot set the max errors allowed higher than (100,000) - " + lmt);
                    } else if (c_options.error_limit <= 10) {
                        error("cannot have an error limit less than 10 ");
                    }
                } else {
                    error("invalid error limit set " + lmt);
                }
            }
        }
        else if(string(argv[i]).at(0) == '-'){
            error("invalid option `" + string(argv[i]) + "`, try bootstrap --h");
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

#ifdef WIN32_
    native_string path("C:/Program Files/Sharp/include");
#endif
#ifdef POSIX_
    native_string path("/usr/include/sharp/");
#endif

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
        string file = files.get(i).str();

        if(!File::exists(file.c_str())){
            error("file `" + file + "` doesnt exist!");
        }
        if(!File::endswith(".sharp", file)){
            error("file `" + file + "` is not a sharp file!");
        }
    }

    return compile(files);
}

int compile(List<native_string> &files)
{
    List<parser*> parsers;
    parser* currParser = NULL;
    tokenizer* currTokenizer = NULL;
    File::buffer buf;
    size_t errors=0, unfilteredErrors=0;
    long succeeded=0, failed=0, panic=0;

    for(unsigned int i = 0; i < files.size(); i++)
    {
        string file = files.get(i).str();
        buf.begin();

        File::read_alltext(file.c_str(), buf);
        if(buf.empty()) {
            error("file `" + file + "` is empty.");
        } else
        {
            if(c_options.debugMode)
                cout << "tokenizing " << file << endl;

            currTokenizer = new tokenizer(buf.to_str(), file);
            if(currTokenizer->getErrors()->hasErrors())
            {
                currTokenizer->getErrors()->printErrors();

                errors+= currTokenizer->getErrors()->getErrorCount();
                unfilteredErrors+= currTokenizer->getErrors()->getUnfilteredErrorCount();

                currTokenizer->free();
                delete currTokenizer;
                failed++;
            } else {
                if(c_options.debugMode)
                    cout << "parsing " << file << endl;

                currParser = new parser(currTokenizer);
                if(currParser->getErrors()->hasErrors())
                {
                    currParser->getErrors()->printErrors();

                    errors+= currParser->getErrors()->getErrorCount() == 0 ? currParser->getErrors()->getUnfilteredErrorCount() : currParser->getErrors()->getErrorCount();
                    unfilteredErrors+= currParser->getErrors()->getUnfilteredErrorCount();
                    failed++;

                    if(currParser->panic) {
                        currParser->free();
                        delete currParser;
                        panic = 1;
                        goto end;
                    }

                    currParser->free();
                    delete currParser;
                } else {
                    parsers.push_back(currParser);
                    succeeded++;
                }
            }

            end:
            buf.end();

            if(panic==1) {
                cout << "Detected more than " << c_options.error_limit << "+ errors, quitting.";
                break;
            }
        }
    }

    if(!panic && errors == 0 && unfilteredErrors == 0) {
        if(c_options.debugMode)
            cout << "performing syntax analysis on project files"<< endl;

        Compiler engine(c_options.out, parsers);

        failed += engine.failedParsers.size();
        succeeded = files.size() - failed;

        errors+=engine.errors->getErrorCount();
        unfilteredErrors+=engine.errors->getUnfilteredErrorCount();
        if(errors == 0 && unfilteredErrors == 0) {
            if(!c_options.compile)
                engine.generate();
        }

        engine.cleanup();
    }
    else {
        for(unsigned long i = 0; i < parsers.size(); i++) {
            parser* parser = parsers.get(i);
            parser->getTokenizer()->free();
            delete parser->getTokenizer();
            parser->free();
            delete(parser);
        }
        parsers.free();
    }

    cout << endl << "==========================================================\n" ;
    cout << "Errors: " << (c_options.aggressive_errors ? unfilteredErrors : errors) << " Succeeded: "
         << succeeded << " Failed: " << failed << " Total: " << files.size() << endl;
    cout << std::flush << std::flush;
    return (failed == 0 && (c_options.aggressive_errors ? unfilteredErrors : errors) == 0) ? 0 : 1;
}