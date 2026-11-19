//
// OPENGL, the part of it the 3D side uses  -  found at run time.
//
// The 3D scenes (g3_core.cpp) are drawn with OpenGL: on a PC "desktop" OpenGL,
// on a Raspberry Pi and other small machines OpenGL ES 2. The two share the
// functions below, and the code is written to that common part.
//
// WHY THE FUNCTIONS ARE LOOKED UP BY NAME instead of linked: on Windows only
// the OpenGL of 1996 can be linked against; everything newer has to be asked
// for from the graphics driver once a context exists. SDL does the asking
// (SDL_GL_GetProcAddress) the same way on every system, so one list serves
// them all, and the library needs no OpenGL headers or libraries to build.
//
// g3_load_gl() fills the pointers after a context is current. A function the
// system does not have stays null - only the ones marked "optional" may be.
//
#ifndef SHARPGUI_G3_GL_H
#define SHARPGUI_G3_GL_H

#include <SDL2/SDL.h>
#include <cstddef>

#if defined(_WIN32)
#define G3_CALL __stdcall
#else
#define G3_CALL
#endif

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLbitfield;
typedef unsigned char GLboolean;
typedef unsigned char GLubyte;
typedef float GLfloat;
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_LINES 0x0001
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_LEQUAL 0x0203
#define GL_LESS 0x0201
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE 1
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_SCISSOR_TEST 0x0C11
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_MULTISAMPLE 0x809D
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_TEXTURE_2D 0x0DE1
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT 0x1403
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_DEPTH_COMPONENT 0x1902
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03
#define GL_TEXTURE0 0x84C0
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_FRAMEBUFFER 0x8D40
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_RGBA8 0x8058
#define GL_MAX_SAMPLES 0x8D57
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_NONE 0

// every function the 3D side calls: G3_GL(result, name, arguments)
#define G3_GL_FUNCTIONS \
    G3_GL(void, glViewport, (GLint x, GLint y, GLsizei w, GLsizei h)) \
    G3_GL(void, glScissor, (GLint x, GLint y, GLsizei w, GLsizei h)) \
    G3_GL(void, glEnable, (GLenum what)) \
    G3_GL(void, glDisable, (GLenum what)) \
    G3_GL(void, glClear, (GLbitfield mask)) \
    G3_GL(void, glClearColor, (GLfloat r, GLfloat g, GLfloat b, GLfloat a)) \
    G3_GL(void, glDepthFunc, (GLenum how)) \
    G3_GL(void, glDepthMask, (GLboolean on)) \
    G3_GL(void, glColorMask, (GLboolean r, GLboolean g, GLboolean b, GLboolean a)) \
    G3_GL(void, glCullFace, (GLenum side)) \
    G3_GL(void, glBlendFunc, (GLenum from, GLenum to)) \
    G3_GL(void, glPolygonOffset, (GLfloat factor, GLfloat units)) \
    G3_GL(void, glPixelStorei, (GLenum what, GLint value)) \
    G3_GL(void, glReadPixels, (GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, void* to)) \
    G3_GL(const GLubyte*, glGetString, (GLenum what)) \
    G3_GL(void, glGetIntegerv, (GLenum what, GLint* to)) \
    G3_GL(GLenum, glGetError, (void)) \
    G3_GL(void, glFinish, (void)) \
    G3_GL(void, glDrawArrays, (GLenum kind, GLint first, GLsizei count)) \
    G3_GL(void, glDrawElements, (GLenum kind, GLsizei count, GLenum type, const void* offset)) \
    G3_GL(void, glGenTextures, (GLsizei count, GLuint* to)) \
    G3_GL(void, glDeleteTextures, (GLsizei count, const GLuint* which)) \
    G3_GL(void, glBindTexture, (GLenum target, GLuint texture)) \
    G3_GL(void, glTexImage2D, (GLenum target, GLint level, GLint inside, GLsizei w, GLsizei h, GLint border, GLenum format, GLenum type, const void* pixels)) \
    G3_GL(void, glTexParameteri, (GLenum target, GLenum what, GLint value)) \
    G3_GL(void, glActiveTexture, (GLenum unit)) \
    G3_GL(void, glGenerateMipmap, (GLenum target)) \
    G3_GL(GLuint, glCreateShader, (GLenum kind)) \
    G3_GL(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar* const* text, const GLint* lengths)) \
    G3_GL(void, glCompileShader, (GLuint shader)) \
    G3_GL(void, glGetShaderiv, (GLuint shader, GLenum what, GLint* to)) \
    G3_GL(void, glGetShaderInfoLog, (GLuint shader, GLsizei room, GLsizei* length, GLchar* to)) \
    G3_GL(void, glDeleteShader, (GLuint shader)) \
    G3_GL(GLuint, glCreateProgram, (void)) \
    G3_GL(void, glAttachShader, (GLuint program, GLuint shader)) \
    G3_GL(void, glBindAttribLocation, (GLuint program, GLuint index, const GLchar* name)) \
    G3_GL(void, glLinkProgram, (GLuint program)) \
    G3_GL(void, glGetProgramiv, (GLuint program, GLenum what, GLint* to)) \
    G3_GL(void, glGetProgramInfoLog, (GLuint program, GLsizei room, GLsizei* length, GLchar* to)) \
    G3_GL(void, glUseProgram, (GLuint program)) \
    G3_GL(void, glDeleteProgram, (GLuint program)) \
    G3_GL(GLint, glGetUniformLocation, (GLuint program, const GLchar* name)) \
    G3_GL(void, glUniform1i, (GLint at, GLint v)) \
    G3_GL(void, glUniform1f, (GLint at, GLfloat v)) \
    G3_GL(void, glUniform2f, (GLint at, GLfloat a, GLfloat b)) \
    G3_GL(void, glUniform3f, (GLint at, GLfloat a, GLfloat b, GLfloat c)) \
    G3_GL(void, glUniform4f, (GLint at, GLfloat a, GLfloat b, GLfloat c, GLfloat d)) \
    G3_GL(void, glUniform3fv, (GLint at, GLsizei count, const GLfloat* v)) \
    G3_GL(void, glUniform4fv, (GLint at, GLsizei count, const GLfloat* v)) \
    G3_GL(void, glUniformMatrix4fv, (GLint at, GLsizei count, GLboolean transpose, const GLfloat* v)) \
    G3_GL(void, glGenBuffers, (GLsizei count, GLuint* to)) \
    G3_GL(void, glDeleteBuffers, (GLsizei count, const GLuint* which)) \
    G3_GL(void, glBindBuffer, (GLenum target, GLuint buffer)) \
    G3_GL(void, glBufferData, (GLenum target, GLsizeiptr size, const void* data, GLenum use)) \
    G3_GL(void, glEnableVertexAttribArray, (GLuint index)) \
    G3_GL(void, glDisableVertexAttribArray, (GLuint index)) \
    G3_GL(void, glVertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* offset)) \
    G3_GL(void, glVertexAttrib4f, (GLuint index, GLfloat a, GLfloat b, GLfloat c, GLfloat d)) \
    G3_GL(void, glGenFramebuffers, (GLsizei count, GLuint* to)) \
    G3_GL(void, glDeleteFramebuffers, (GLsizei count, const GLuint* which)) \
    G3_GL(void, glBindFramebuffer, (GLenum target, GLuint buffer)) \
    G3_GL(void, glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)) \
    G3_GL(void, glFramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum rbtarget, GLuint renderbuffer)) \
    G3_GL(GLenum, glCheckFramebufferStatus, (GLenum target)) \
    G3_GL(void, glGenRenderbuffers, (GLsizei count, GLuint* to)) \
    G3_GL(void, glDeleteRenderbuffers, (GLsizei count, const GLuint* which)) \
    G3_GL(void, glBindRenderbuffer, (GLenum target, GLuint buffer)) \
    G3_GL(void, glRenderbufferStorage, (GLenum target, GLenum format, GLsizei w, GLsizei h))

// optional: anti-aliased off-screen drawing (desktop OpenGL 3 and later). Null where missing.
#define G3_GL_OPTIONAL \
    G3_GL(void, glRenderbufferStorageMultisample, (GLenum target, GLsizei samples, GLenum format, GLsizei w, GLsizei h)) \
    G3_GL(void, glBlitFramebuffer, (GLint sx0, GLint sy0, GLint sx1, GLint sy1, GLint dx0, GLint dy0, GLint dx1, GLint dy1, GLbitfield mask, GLenum filter))

#define G3_GL(result, name, arguments) extern result (G3_CALL *name) arguments;
G3_GL_FUNCTIONS
G3_GL_OPTIONAL
#undef G3_GL

// Look every function up (a context must be current). Returns the name of the first
// required one that is missing, or null when all are there.
const char* g3_load_gl();

#endif //SHARPGUI_G3_GL_H
