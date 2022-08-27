//
// Created by BNunnally on 8/26/2021.
//


#include <cmath>
#include <sys/stat.h>
#include <dirent.h>
#include "frontend/tokenizer/tokenizer.h"
#include "../util/zip/zlib.h"
#include "../util/File.h"
#include "settings/settings.h"
#include "settings/settings_processor.h"
#include "sharp_file.h"
#include "taskdelegator/task_delegator.h"
#include "../util/time.h"
#include "backend/finalizer/finalizer.h"

int compile();

void help() {
    cout << "Usage: sharpc " << "{OPTIONS} SOURCE FILE(S)"                                                     << endl;
    cout << "Source file must have a .sharp extension to be compiled.\n"                                       << endl;
    cout << "[-options]\n\n    -V                   print compiler version and exit"                           << endl;
    cout <<               "    -showversion         print compiler version and continue"                       << endl;
    cout <<               "    -o<file>             set the output object file"                                << endl;
    cout <<               "    -c                   compile only and do not generate exe"                      << endl;
    cout <<               "    -a                   enable aggressive error reporting"                         << endl;
    cout <<               "    -O -O1               basic optimization of code"                                << endl;
    cout <<               "    -O2                  max level optimization of code"                            << endl;
    cout <<               "    -L<path>             library directory path"                                    << endl;
    cout <<               "    -w                   disable all warnings"                                      << endl;
    cout <<               "    --clean              flush compiler cache and recompile the project fresh"      << endl;
    cout <<               "    -run                 run compiled source after successful compilation"          << endl;
    cout <<               "    -g -green            run compiler in \"green\" mode to consume less power"      << endl;
    cout <<               "    -dbg -debug          enables debugging in source code (enabled by default)"     << endl;
    cout <<               "    -ignoredir<dir>      include a directory to ignore"                             << endl;
    cout <<               "    -nativedir -nd <dir> set output native code directory"                          << endl;
    cout <<               "    -ignore<file>        include a file to ignore"                                  << endl;
    cout <<               "    -p                   enable project mode compilation"                           << endl;
    cout <<               "    -obf                 obfuscate user code"                                       << endl;
    cout <<               "    -P<path>             enable project mode compilation on a specific path"        << endl;
    cout <<               "    --new-project<name>  create a new sharp project"                                << endl;
    cout <<               "    -errlmt<count>       set max errors the compiler allows before quitting"        << endl;
    cout <<               "    -v<version>          set the application version"                               << endl;
    cout <<               "    -target              target the specified platform of sharp to run on"          << endl;
    cout <<               "    -release -r          generate a release build exe"                              << endl;
    cout <<               "    --hw                 display help message for warning options"                  << endl;
    cout <<               "    --h -?               display this help message"                                 << endl;
}

void help_warn() {
    cout << "Usage: sharpc {OPTIONS} SOURCE FILE(S)"                                                       << endl;
    cout << "Source file must have a .sharp extion to be compiled.\n"                                      << endl;
    cout << "Please note that general warnings can only be disabled via disabling all warnings.\n"                << endl;
    cout << "[-options]\n\n    -w                disable all warnings"                                     << endl;
    cout <<               "    -winit            disable class initialization warnings"                    << endl;
    cout <<               "    -waccess          disable access modifier warnings (public, static, etc.)"  << endl;
    cout <<               "    -wambig           disable ambiguous symbol warnings"                        << endl;
    cout <<               "    -wdecl            disable object declaration warnings"                      << endl;
    cout <<               "    -wmain            disable multiple main method warnings"                    << endl;
    cout <<               "    -wcast            disable type cast warnings"                               << endl;
    cout <<               "    -wnull            disable null/nullable type warnings"                               << endl;
    cout <<               "    -wdep             disable dependency injection warnings"                    << endl;
    cout <<               "    -werror           enable warnings as errors"                                << endl;
    cout <<               "    --hw              display this help message"                                << endl;
}

void create_project() {
    string dir = options.new_project_dir;
    options.project_name = File::name(dir);
    string div;

#ifdef __WIN32
    div = "\\";
#else
    div = "/";
#endif

    if(File::exists(dir.c_str())) {
        cout << "could not create project, a " << ((File::getFileAttrs(dir) & FILE_DIRECTORY) ? "directory" : "file") << " with the name " << dir << " already exists!" << endl;
        exit(1);
    }

    if(File::makeDir(dir) != 0) {
        cout << "failed to create project directory: " << dir << endl;
        exit(1);
    }

    // project files
    string srcDir = dir + div + "src";
    string buildDir = dir + div + "build";
    string cacheDir = dir + div + "build" + div + "cache";
    string outputsDir = dir + div + "build" + div + "outputs";
    string filesDir = dir + div + "build" + div + "cache" + div + "files";
    string main_file = dir + div + "src" + div + "main.sharp";
    string settings_file = dir + div + SETTINGS_FILE;

    // template files
    string main_template_file;
    string settings_template_file;

    if(File::makeDir(srcDir) || File::makeDir(buildDir)
        || File::makeDir(cacheDir) || File::makeDir(outputsDir)
        || File::makeDir(filesDir)) {
        cout << "failed to create all necessary directories for project: " << options.project_name << endl;
        exit(1);
    }

#ifdef __WIN32
    main_template_file = "C:\\Program Files\\Sharp\\proj-template\\main.sharp";
    settings_template_file = "C:\\Program Files\\Sharp\\proj-template\\"; settings_template_file += SETTINGS_FILE;
#else
    main_template_file = "/usr/include/sharp/proj-template/main.sharp";
    settings_template_file = "/usr/include/sharp/$settings_file_name";
#endif

    string data;
    File::buffer out;
    if(File::exists(main_template_file.c_str())) {
        File::read_alltext(main_template_file.c_str(), out);
        data = out.to_str();
        out.end();
    } else {
        data = "mod main;\n\ndef main() {\n  println(\"hello, world!\");\n}\n";
    }

    if(File::write(main_file.c_str(), data) != 0) {
        cout << "failed to create the main file for project: " << dir << endl;
        exit(1);
    }

    if(File::exists(settings_template_file.c_str())) {
        File::read_alltext(settings_template_file.c_str(), out);
        data = out.to_str();
        out.end();
    } else {
        data = "{\n  \"name\": \"" + options.project_name + "\",\n  \"version\": \"1.0\",\n  \"output\": \"" + options.project_name + "\",\n"
               + "  \"ignore_folders\": [\n     \"build\"\n  ]\n}\n";
    }

    if(File::write(settings_file.c_str(), data) != 0) {
        cout << "failed to create the main file for project: " + options.project_name << endl;
        exit(1);
    }

    cout << "Project: \"" << options.project_name << "\" successfully created!" << endl;
}

int _bootstrap(int argc, const char* args[])
{
    if(argc < 2) {
        help();
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        args_:
        string arg(args[i]);
        if(opt("-a")){
            enable_show_all_errors(true);
        }
        else if(opt("-c")){
            enable_compile_only(true);
        }
        else if(opt("-o")){
            if(i+1 >= argc)
                error("output file required after option `-o`");
            else
                set_output_file(string(args[++i]));
        }
        else if(opt("-L")){
            if(i+1 >= argc)
                error("library directory required after option `-L`");
            else
                add_library_path(string(args[++i]));
        }
        else if(opt("-V")){
            printVersion();
            exit(0);
        }
        else if(opt("-O") || opt("-O1")){
            set_optimization_level(basic_optimization);
        }
        else if(opt("-O2")){
            set_optimization_level(high_performance_optimization);
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
            set_optimization_level(high_performance_optimization);
            enable_app_debugging(false);
        }
        else if(opt("-magic")){
            enable_magic_mode(true);
        }
        else if(opt("-d")){
            enable_debug_mode(true);
        }
        else if(opt("-dbg") | opt("-debug")) {
            enable_app_debugging(true);
        }
        else if(opt("-showversion")){
            printVersion();
            cout << endl;
        }
        else if(opt("-target")){
            if(i+1 >= argc)
                error("file version required after option `-target`");
            else {
                set_target_platform(string(args[++i]));
            }
        }
        else if(opt("-w")){
            enable_warnings(false);
        }
        else if(opt("-waccess") || opt("-wambig") || opt("-wdecl")
                 || opt("-wmain") || opt("-wcast") || opt("-winit")
                 || opt("-wnull")){
            enable_warning_type(arg, false);
        }
        else if(opt("-p")){
            set_compilation_mode(project_mode);
        }
        else if(opt("-run")){
            enable_run_source_code(true);
        }
        else if(opt("-clean")){
            enable_clean_project(true);
        }
        else if(opt("-g") || opt("-green")){
            enable_green_mode(true);
        }
        else if(opt("--new-project") || opt("--create-project")){
            if(i+1 >= argc)
                error("project directory required after option `" + arg + "`");
            else
                set_new_project_path(string(args[++i]));
        }
        else if(opt("-v")){
            if(i+1 >= argc)
                error("file version required after option `-v`");
            else
                set_source_version(string(args[++i]));
        }
        else if(opt("-ignore")){
            if(i+1 >= argc)
                error("file path required after option `-ignore`");
            else
                add_ignored_file(string(args[++i]));
        }
        else if(opt("-ignoredir")){
            if(i+1 >= argc)
                error("directory path required after option `-ignoredir`");
            else
                add_ignored_directory(string(args[++i]));
        }
        else if(opt("-P")){
            if(i+1 >= argc)
                error("project path required after option `-P`");
            else
                set_project_directory(string(args[++i]));
        }
        else if(opt("-werror")){
            enable_warnings_as_errors();
        }
        else if(opt("-errlmt")) {
            if(i+1 >= argc)
                error("error limit required after option `-errlmt`");
            else {
                set_error_limit(string(args[++i]));
            }
        }
        else if(opt("-nativedir") || opt("-nd")){
            if(i+1 >= argc)
                error("output directory required after option `" + arg + "`");
            else
                set_native_code_directory(string(args[++i]));
        }
        else if(opt("-obf")){
            enable_source_obfuscation(true);
        }
        else if(string(args[i]).at(0) == '-'){
            error("invalid option `" + string(args[i]) + "`, try bootstrap --h");
        }
        else {
            // add the source files
            do {
                if(string(args[i]).at(0) == '-')
                    goto args_;


                options.source_files.addif(string(args[i++]));
            }while(i<argc);
            break;
        }
    }

    if(options.new_project_dir != "") {
        create_project();
    }

    if(options.source_files.size() == 0 && options.compile_mode == file_mode){
        if(options.new_project_dir != "") return 0;

        help();
        return 1;
    }

    initalizeErrors();
    if(options.compile_mode == project_mode) {
        if(options.project_dir == "") {
            string currDir;
            File::currentDirectory(currDir);
            options.project_dir = currDir;
        }

        options.project_name = File::name(options.project_dir);
        validate_and_add_source_files(options.project_dir);
        process_settings();
    } else {
        process_library_files(PROG_VERS);
    }

    for(uInt i = 0; i < options.source_files.size(); i++) {
        string sourceFile = options.source_files.get(i);

        if(!File::exists(sourceFile.c_str()))
            error("file `" + sourceFile + "` doesnt exist!");\

        if(!File::endswith(".sharp", sourceFile))
            error("file `" +sourceFile + "` is not a sharp file!");

        options.source_files.get(i).clear();
        File::resolvePath(sourceFile, options.source_files.get(i));
    }

    remove_ignored_files();
    return compile();
}

bool isFileLarger(sharp_file *f1, sharp_file *f2) {
    return File::length(f1->name.c_str()) < File::length(f2->name.c_str());
}

void run_pre_processing_tasks() {

    task t;
    for(Int i = 0; i < sharpFiles.size(); i++) {
        t.type = task_tokenize_;
        t.file = sharpFiles.get(i);
        submit_task(t);

        t.type = task_parse_;
        submit_task(t);

//        if(i == 0) break; // remove
    }

    wait_for_tasks();

    if(all_files_parsed()) {
        for (Int i = 0; i < sharpFiles.size(); i++) {
            t.file = sharpFiles.get(i);
            t.type = task_preprocess_;
            submit_task(t);
        }

        wait_for_tasks();
    }
}

void run_post_processing_tasks() {

    task t;
    for(Int i = 0; i < sharpFiles.size(); i++) {
        t.type = task_process_imports;
        t.file = sharpFiles.get(i);
        submit_task(t);
    }

    wait_for_tasks();

    for(Int i = 0; i < sharpFiles.size(); i++) {
        t.type = task_post_process_;
        t.file = sharpFiles.get(i);
        submit_task(t);
    }

    wait_for_tasks();
}

void run_compile_global_mutations_tasks() {

    task t;
    for(Int i = 0; i < sharpFiles.size(); i++) {
        t.type = task_compile_mutations_;
        t.file = sharpFiles.get(i);
        submit_task(t);
    }

    wait_for_tasks();
}

void run_compile_global_members_tasks() {

    task t;
    for(Int i = 0; i < sharpFiles.size(); i++) {
        t.type = task_compile_global_members_;
        t.file = sharpFiles.get(i);
        submit_task(t);
    }

    wait_for_tasks();
}

void run_compile_classes_tasks() {

    task t;
    for(Int i = 0; i < sharpFiles.size(); i++) {
        t.type = task_compile_classes_;
        t.file = sharpFiles.get(i);
        submit_task(t);
    }

    wait_for_tasks();
}

void run_compilation_tasks() {

    task t;
    for(Int i = 0; i < sharpFiles.size(); i++) {
        t.type = task_compile_components_;
        t.file = sharpFiles.get(i);
        submit_task(t);
    }

    wait_for_tasks();

    for(Int i = 0; i < sharpFiles.size(); i++) {
        t.type = task_process_delegates_;
        t.file = sharpFiles.get(i);
        submit_task(t);
    }

    wait_for_tasks();
}

int compile()
{
    for(Int i = 0; i < options.source_files.size(); i++) {
        sharpFiles.add(
                new sharp_file(options.source_files.get(i)));
    }


    sharpFiles.linearSort(isFileLarger);
    start_task_delegator();
    run_pre_processing_tasks();

    if(all_files_parsed()) {
        run_post_processing_tasks();
        run_compilation_tasks();
        run_compile_global_mutations_tasks();
        run_compile_global_members_tasks();
        run_compile_classes_tasks();
        setup_core_functions();
        finalize_compilation();
    }

    cout << "done" << endl;
    return 0;
}