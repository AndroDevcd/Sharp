//
// The OpenGL function pointers (g3_gl.h), and looking them up.
//
#include "g3_gl.h"

#define G3_GL(result, name, arguments) result (G3_CALL *name) arguments = nullptr;
G3_GL_FUNCTIONS
G3_GL_OPTIONAL
#undef G3_GL

const char* g3_load_gl() {
    const char* missing = nullptr;
#define G3_GL(result, name, arguments) \
    name = (result (G3_CALL *) arguments) SDL_GL_GetProcAddress(#name); \
    if(name == nullptr && missing == nullptr) missing = #name;
    G3_GL_FUNCTIONS
#undef G3_GL
#define G3_GL(result, name, arguments) \
    name = (result (G3_CALL *) arguments) SDL_GL_GetProcAddress(#name);
    G3_GL_OPTIONAL
#undef G3_GL
    return missing;
}
