//
// Lazy compilation of library function bodies - see lazy_functions.h for the idea.
//

#include "lazy_functions.h"
#include "function_compiler.h"
#include "../../context/context.h"
#include "../../types/types.h"
#include "../../dependency/dependancy.h"
#include "../../../taskdelegator/task_delegator.h"
#include "../../../compiler_info.h"
#include "../../../compiler_timings.h"
#include "../../../settings/settings.h"
#include "../../../frontend/parser/Ast.h"
#include <unordered_set>
#include <vector>

static long deferredCount = 0;
static long deferredCompiledCount = 0;

// The one function whose body compile_function() must compile although it
// would qualify for deferring: the one compile_deferred_function_body() is
// working on right now.
static thread_local sharp_function *forcedFunction = NULL;

long deferred_function_count() { return deferredCount; }
long deferred_function_compiled_count() { return deferredCompiledCount; }

bool is_library_file(sharp_file *file) {
    if(file == NULL) return false;

    if(file->isLibrary == -1) {     // not worked out yet
        file->isLibrary = 0;
        for(Int i = 0; i < options.libraries.size(); i++) {
            const string &folder = options.libraries.get(i);
            if(!folder.empty() && file->name.compare(0, folder.size(), folder) == 0) {
                file->isLibrary = 1;
                break;
            }
        }
    }

    return file->isLibrary == 1;
}

/** Is there a lambda anywhere inside this piece of the syntax tree? */
static bool contains_lambda(Ast *ast) {
    if(ast == NULL) return false;
    if(ast->getType() == ast_lambda_function) return true;

    for(Int i = 0; i < ast->getSubAstCount(); i++) {
        if(contains_lambda(ast->getSubAst(i)))
            return true;
    }
    return false;
}

bool defer_function_body(sharp_function *function, Ast *ast) {
    if(!options.lazy_library || function == forcedFunction || function->bodyPending)
        return false;

    // Only plain functions, operators and constructors. Initializers, static
    // initializers and delegates are part of how a class is set up.
    if(function->type != normal_function && function->type != operator_function
       && function->type != constructor_function)
        return false;

    // Only code that comes from a library file. (For a generic class the body
    // may be compiled while another file is the current one - what counts is
    // where the function was written.)
    if(!is_library_file(function->implLocation.file))
        return false;

    // Only from a class level context, which is all that is written down and
    // rebuilt later (getters and setters are compiled inside a field context).
    if(current_context.type != class_context && current_context.type != global_context)
        return false;

    // `def f() := <expression>` has no declared return type: compiling the body
    // is what gives the function its type, and callers need that.
    if(function->returnType.type == type_untyped)
        return false;

    // The platform class holds the functions the runtime starts with.
    if(function->owner != NULL && function->owner->fullName == platform_class_name)
        return false;
    if(function->name == "main")
        return false;

    // A lambda inside a function becomes a function and a closure of the
    // enclosing CLASS, which are finished right after the class's functions
    // (compile_class_lambdas). Such bodies
    // stay where they are.
    if(contains_lambda(ast))
        return false;

    function->bodyPending = true;
    function->pendingAst = ast;
    function->pendingFile = current_file;                   // the file whose context it is compiled in
    function->pendingModule = currModule;
    function->pendingClass = current_context.classCxt;
    function->pendingStatic = current_context.isStatic;
    deferredCount++;
    return true;
}

void force_defer_function_body(sharp_function *function, Ast *ast, sharp_file *file,
                               sharp_module *module, sharp_class *classContext, bool isStatic) {
    if(function == NULL || function->bodyPending)
        return;

    function->bodyPending = true;
    function->pendingAst = ast;
    function->pendingFile = file;
    function->pendingModule = module;
    function->pendingClass = classContext;
    function->pendingStatic = isStatic;
    deferredCount++;
}

bool compile_deferred_function_body(sharp_function *function) {
    if(function == NULL || !function->bodyPending)
        return false;

    TIME_STAGE("compile reached library functions (lazy)");
    function->bodyPending = false;
    deferredCompiledCount++;

    // Put the compiler where it was when the body was put aside: same file,
    // same module, inside the same class.
    worker_thread *createdWorker = NULL;
    if(currThread == NULL) {    // the calling thread was never a worker (threaded compile)
        createdWorker = new worker_thread();
        currThread = createdWorker;
    }

    task *taskBefore = currThread->currTask;
    sharp_module *moduleBefore = currModule;
    sharp_function *forcedBefore = forcedFunction;

    task here;
    here.file = function->pendingFile;
    here.type = task_compile_classes_;
    currThread->currTask = &here;
    currModule = function->pendingModule;
    forcedFunction = function;

    create_context(function->pendingClass, function->pendingStatic);
    compile_function(function, function->pendingAst);
    delete_context();

    forcedFunction = forcedBefore;
    currModule = moduleBefore;
    currThread->currTask = taskBefore;
    if(createdWorker != NULL) {
        currThread = NULL;
        delete createdWorker;
    }

    return true;
}

// ---------------------------------------------------------------------------
// The walk. It mirrors mark() in finalizer/optimization/dependency_runner.cpp
// step for step - the two have to agree on what "reachable" means - but it
// keeps its own "seen" set and leaves the `used` flags alone, because it may
// have to go round more than once (see below).
// ---------------------------------------------------------------------------

static std::unordered_set<void*> seen;
static long compiledThisRound = 0;

static void reach(sharp_field *field);
static void reach(sharp_function *function);
static void reach(sharp_class *sc);
static void reach(sharp_file *file);

static void reach_all(List<dependency> &dependencies) {
    // by index, and re-reading the size: compiling a body appends to these lists
    for(Int i = 0; i < dependencies.size(); i++) {
        dependency d = dependencies.get(i);

        switch(d.type) {
            case dependency_file:     reach(d.fileDependency); break;
            case dependency_class:    reach(d.classDependency); break;
            case dependency_function: reach(d.functionDependency); break;
            case dependency_field:    reach(d.fieldDependency); break;
            default: break;
        }
    }
}

static void reach(sharp_field *field) {
    if(field == NULL || !seen.insert(field).second) return;

    reach_all(field->dependencies);
    reach(field->implLocation.file);
}

static void reach(sharp_function *function) {
    if(function == NULL || !seen.insert(function).second) return;

    // the point of it all: this function is used, so now it needs its body
    if(compile_deferred_function_body(function))
        compiledThisRound++;

    for(Int i = 0; i < function->locals.size(); i++) {
        reach(function->locals.get(i));
    }

    if(function->type == delegate_function) {
        for(Int i = 0; i < function->impls.size(); i++) {
            reach(function->impls.get(i));
            reach(function->impls.get(i)->implLocation.file);
            reach(function->impls.get(i)->owner);
        }
    }

    reach_all(function->dependencies);
    reach(function->implLocation.file);
}

static void reach(sharp_class *sc) {
    if(sc == NULL || !seen.insert(sc).second) return;

    if(sc->type == class_enum) {
        for(Int i = 0; i < sc->fields.size(); i++) {
            reach(sc->fields.get(i));
        }

        for(Int i = 0; i < sc->functions.size(); i++) {
            reach(sc->functions.get(i));
        }
    }

    reach_all(sc->dependencies);
    reach(sc->implLocation.file);
}

static void reach(sharp_file *file) {
    if(file == NULL || !seen.insert(file).second) return;

    reach_all(file->dependencies);
}

void compile_reachable_function_bodies() {
    if(deferredCount == 0)
        return;     // (also runs with -eager: instances of generic functions may have been put aside)
    if(genesis_method == NULL || user_main_method == NULL)
        return;     // no entry point: the finalizer reports that

    // Compiling a body adds dependencies - to the function itself (the walk
    // sees those: it compiles before it looks), but sometimes also to a class
    // or file the walk has already passed. So the walk is repeated until a
    // whole round compiles nothing new. A round over the finished graph takes
    // a few milliseconds.
    do {
        seen.clear();
        compiledThisRound = 0;

        reach(genesis_method);
        reach(user_main_method);
        reach(static_init_method);
    } while(compiledThisRound > 0 && !panic);

    seen.clear();
}
