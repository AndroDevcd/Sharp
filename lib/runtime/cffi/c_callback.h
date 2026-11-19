//
// C CALLING SHARP: a Sharp function handed to C as a C function pointer (c_callback.cpp has the how).
//
#pragma once

#include "../../../stdimports.h"
#include <string>

struct sharp_object;
struct sharp_function;

/**
 * The address C is handed for the function value `function`, which C will call as `signature`
 * says ("(ptr,ptr)i32"). null gives NULL. `family` and `number` say which thunk it is, for
 * c_callback_done. Throws when the signature cannot be a callback or every thunk is taken.
 */
void *c_callback_for(sharp_object *function, const std::string &signature, sharp_function *made_for, int &family, int &number);

/** The C call the callback was handed to is over: its thunk is free again, unless the program keeps the function. */
void c_callback_done(int family, int number);

/** From Sharp, c_keep(function): C will call this function after the call it is handed to has returned. */
void c_keep(sharp_object *function);
/** From Sharp, c_drop(function): C no longer has it. */
void c_drop(sharp_object *function);
bool c_is_kept(sharp_object *function);
