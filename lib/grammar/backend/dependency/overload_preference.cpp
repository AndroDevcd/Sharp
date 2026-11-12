//
// See overload_preference.h.
//

#include "overload_preference.h"
#include "../types/sharp_function.h"
#include "../types/sharp_field.h"
#include "../types/sharp_type.h"

namespace {

/** _int8 ... _uint64 or decimal, and not an array of them */
bool is_plain_number(sharp_type &type) {
    return is_numeric_type(type) && !type.isArray;
}

/**
 * Does `candidate` take every plain-number argument as a plain number?
 * (Arguments that are objects, arrays, lambdas ... do not count either way.)
 */
bool takes_plain_numbers_as_plain_numbers(sharp_function *candidate, List<sharp_field*> &arguments) {
    if(candidate->parameters.size() != arguments.size()) return false;

    for(Int i = 0; i < arguments.size(); i++) {
        if(is_plain_number(arguments.get(i)->type)
           && !is_plain_number(candidate->parameters.get(i)->type))
            return false;
    }
    return true;
}

} // namespace

sharp_function* prefer_plain_number_overload(
        List<sharp_function*> &candidates,
        List<sharp_field*> &arguments) {
    // the rule is about plain-number arguments: without one it has nothing to say
    bool hasPlainNumber = false;
    for(Int i = 0; i < arguments.size(); i++) {
        if(is_plain_number(arguments.get(i)->type)) { hasPlainNumber = true; break; }
    }
    if(!hasPlainNumber) return NULL;

    sharp_function *preferred = NULL;
    for(Int i = 0; i < candidates.size(); i++) {
        if(!takes_plain_numbers_as_plain_numbers(candidates.get(i), arguments)) continue;
        if(preferred != NULL) return NULL;      // two overloads are equally good: still ambiguous
        preferred = candidates.get(i);
    }
    return preferred;
}
