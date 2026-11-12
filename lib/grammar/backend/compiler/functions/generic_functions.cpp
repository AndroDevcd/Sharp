//
// Generic functions - see generic_functions.h for the idea.
//

#include "generic_functions.h"
#include "function_compiler.h"
#include "lazy_functions.h"
#include "../../context/context.h"
#include "../../types/types.h"
#include "../../postprocessor/function_processor.h"
#include "../../../taskdelegator/task_delegator.h"
#include "../../../compiler_info.h"
#include "../../../frontend/parser/Ast.h"
#include <map>
#include <set>
#include <vector>

// what note_generic_call() remembers under a typed name (guarded by globalLock)
struct generic_call {
    std::string baseName;
    std::vector<sharp_type> types;
};
static std::map<std::string, generic_call> noted_calls;

// The type bindings in force, innermost last. Only the innermost list is
// consulted: an instance made in the middle of another one is isolated from it.
static thread_local std::vector<std::vector<generic_type_identifier>*> binding_stack;

// Instances that are being made right now. Making one runs the ordinary
// function processing, which itself looks the typed name up (to reject a
// duplicate declaration) - and that lookup must not start making it again.
static std::set<std::pair<sharp_class*, std::string>> in_progress;

static thread_local Ast *instantiating = NULL;     // the declaration an instance is being made from

bool is_generic_function_decl(Ast *ast) {
    return ast != NULL && ast->hasSubAst(ast_generic_identifier_list);
}

bool processing_generic_function_instance(Ast *ast) {
    return instantiating != NULL && instantiating == ast;
}

void register_generic_function(sharp_class *owner, std::string &name, function_type type, Ast *ast) {
    GUARD(globalLock)

    Ast *list = ast->getSubAst(ast_generic_identifier_list);
    for(size_t i = 0; i < owner->genericFunctions.size(); i++) {
        generic_function_blueprint &existing = owner->genericFunctions[i];
        if(existing.ast == ast)
            return;     // the same declaration again (a class is processed more than once)

        if(existing.name == name && (Int)existing.typeNames.size() == list->getSubAstCount()) {
            create_new_error(GENERIC, ast, "generic function `" + name
                    + "` is already declared with the same number of type parameters in `" + owner->fullName + "`.");
            return;
        }
    }

    generic_function_blueprint blueprint;
    blueprint.name = name;
    blueprint.ast = ast;
    blueprint.type = type;
    blueprint.file = current_file;
    blueprint.module = currModule;

    for(Int i = 0; i < list->getSubAstCount(); i++) {
        blueprint.typeNames.push_back(list->getSubAst(i)->getToken(0).getValue());
    }

    owner->genericFunctions.push_back(blueprint);
}

void note_generic_call(const std::string &typedName, const std::string &baseName, List<sharp_type> &types) {
    GUARD(globalLock)
    if(noted_calls.find(typedName) != noted_calls.end())
        return;

    generic_call call;
    call.baseName = baseName;
    for(Int i = 0; i < types.size(); i++) {
        call.types.push_back(sharp_type(types.get(i)));
    }
    noted_calls[typedName] = call;
}

bool generic_call_info(const std::string &typedName, std::string &baseName, List<sharp_type> &types) {
    GUARD(globalLock)
    auto found = noted_calls.find(typedName);
    if(found == noted_calls.end())
        return false;

    baseName = found->second.baseName;
    for(size_t i = 0; i < found->second.types.size(); i++) {
        types.__new().copy(found->second.types[i]);
    }
    return true;
}

static sharp_function *find_function_named(sharp_class *owner, const std::string &name) {
    for(Int i = 0; i < owner->functions.size(); i++) {
        if(owner->functions.get(i)->name == name)
            return owner->functions.get(i);
    }
    return NULL;
}

/**
 * Make the function `typedName` from `blueprint` in `owner`, with the
 * blueprint's type parameters bound to `types`.
 */
static void instantiate(sharp_class *owner, generic_function_blueprint &blueprint,
                        const std::string &typedName, std::vector<sharp_type> &types) {
    std::vector<generic_type_identifier> bindings;
    for(size_t i = 0; i < blueprint.typeNames.size(); i++) {
        sharp_type noBase;
        bindings.push_back(generic_type_identifier(blueprint.typeNames[i], types[i], noBase));
    }

    // Process the declaration as if the compiler stood where it was written:
    // its file, its module, inside its class - and with no locals of the
    // function whose call brought us here.
    worker_thread *createdWorker = NULL;
    if(currThread == NULL) {
        createdWorker = new worker_thread();
        currThread = createdWorker;
    }

    task *taskBefore = currThread->currTask;
    sharp_module *moduleBefore = currModule;
    Ast *instantiatingBefore = instantiating;

    task here;
    here.file = blueprint.file;
    here.type = taskBefore != NULL ? taskBefore->type : task_compile_classes_;
    currThread->currTask = &here;
    currModule = blueprint.module;

    {
        isolated_context_scope isolation(&blueprint.file->ctx);
        create_context(owner, true);

        binding_stack.push_back(&bindings);
        instantiating = blueprint.ast;

        std::string name = typedName;
        process_function(owner, name, blueprint.type, blueprint.ast);

        instantiating = instantiatingBefore;

        sharp_function *instance = find_function_named(owner, typedName);
        if(instance != NULL) {
            instance->genericBindings = bindings;

            if(compilation_ready(blueprint.file->stage)) {
                // bodies are being compiled already: this one too
                compile_function(instance, blueprint.ast);
            } else {
                // too early (the files are still being post-processed): leave
                // the body with the lazily compiled ones, it is compiled when
                // the program turns out to use the function
                force_defer_function_body(instance, blueprint.ast, blueprint.file,
                                          blueprint.module, owner, true);
            }
        }

        binding_stack.pop_back();
        delete_context();
    }

    currModule = moduleBefore;
    currThread->currTask = taskBefore;
    if(createdWorker != NULL) {
        currThread = NULL;
        delete createdWorker;
    }
}

void instantiate_generic_function_for_call(const std::string &typedName, sharp_class *owner) {
    GUARD(globalLock)
    if(owner == NULL) return;

    auto found = noted_calls.find(typedName);
    if(found == noted_calls.end())
        return;

    generic_call &call = found->second;

    // the class itself, then its base classes: a generic method is inherited
    for(sharp_class *sc = owner; sc != NULL; sc = sc->baseClass) {
        if(find_function_named(sc, typedName) != NULL)
            return;     // made before

        for(size_t i = 0; i < sc->genericFunctions.size(); i++) {
            // (by index and re-read: instantiating can add blueprints to other classes, not to this list)
            if(sc->genericFunctions[i].name == call.baseName
               && sc->genericFunctions[i].typeNames.size() == call.types.size()) {
                std::pair<sharp_class*, std::string> key(sc, typedName);
                if(in_progress.count(key) != 0)
                    return;

                in_progress.insert(key);
                generic_function_blueprint blueprint = sc->genericFunctions[i];
                std::vector<sharp_type> types = call.types;    // (a copy: `call` lives in a map that may grow meanwhile)
                instantiate(sc, blueprint, typedName, types);
                in_progress.erase(key);
                return;
            }
        }

        if(sc->baseClass == sc) break;
    }
}

bool resolve_generic_function_type_param(const std::string &name, sharp_type &result) {
    if(binding_stack.empty())
        return false;

    std::vector<generic_type_identifier> &bindings = *binding_stack.back();
    for(size_t i = 0; i < bindings.size(); i++) {
        if(bindings[i].name == name) {
            result.copy(bindings[i].type);
            return true;
        }
    }
    return false;
}

void inherit_generic_bindings(sharp_function *lambda) {
    if(lambda != NULL && !binding_stack.empty() && lambda->genericBindings.empty())
        lambda->genericBindings = *binding_stack.back();
}

generic_binding_scope::generic_binding_scope(sharp_function *function)
        : pushed(false) {
    if(function != NULL) {
        binding_stack.push_back(&function->genericBindings);
        pushed = true;
    }
}

static std::vector<generic_type_identifier> no_bindings;

generic_binding_isolation::generic_binding_isolation() {
    binding_stack.push_back(&no_bindings);
}

generic_binding_isolation::~generic_binding_isolation() {
    binding_stack.pop_back();
}

generic_binding_scope::~generic_binding_scope() {
    if(pushed)
        binding_stack.pop_back();
}
