//
// Function values in the compiler. The description is in function_value.h.
//

#include "function_value.h"
#include "types.h"
#include "../dependency/dependancy.h"
#include "../../compiler_info.h"
#include "../../taskdelegator/task_delegator.h"

sharp_class* function_value_class() {
    string kernel = "platform.kernel";
    sharp_module *module = get_module(kernel);
    if(module == NULL) return NULL;

    return resolve_class(module, "function_value", false, false);
}

static sharp_field* function_value_field(const char *name) {
    sharp_class *sc = function_value_class();
    if(sc == NULL) return NULL;

    return locate_field(name, sc);
}

sharp_field* function_value_address_field()  { return function_value_field("address"); }
sharp_field* function_value_captured_field() { return function_value_field("captured"); }

void depend_on_function_value() {
    sharp_class *sc = function_value_class();
    if(sc == NULL) return;

    // nothing is "being compiled" once the files are done (the finalizer makes
    // the function value of `main`): it names its user itself, see below
    if(currThread == NULL || currThread->currTask == NULL || currThread->currTask->file == NULL)
        return;

    create_dependency(sc);
    create_dependency(function_value_address_field());
    create_dependency(function_value_captured_field());
}

void depend_on_function_value(sharp_function *user) {
    sharp_class *sc = function_value_class();
    if(sc == NULL || user == NULL) return;

    create_dependency(user, sc);
    create_dependency(user, function_value_address_field());
    create_dependency(user, function_value_captured_field());
}
