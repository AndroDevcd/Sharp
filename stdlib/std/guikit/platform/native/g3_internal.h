//
// What the files of the 3D side share with each other.
//
//      g3_gl.h / g3_gl.cpp     OpenGL's functions, found at run time
//      g3_core.cpp             the context, shaders, textures, models, scenes: drawing a scene
//      g3_load.cpp             model files: OBJ and glTF
//
// THE WORDS
//
//      a MODEL     what something looks like: one or more PARTS, each a list of corners and
//                  triangles on the graphics card with one material (colour, texture, shine ...)
//      a SCENE     a list of NODES - each a model at a place, turned and scaled, relative to its
//                  parent node - with a camera, a sun, lamps, a sky and fog
//
// Everything is made by the Sharp side (std/guikit/three) through the g3_* functions.
//
#ifndef SHARPGUI_G3_INTERNAL_H
#define SHARPGUI_G3_INTERNAL_H

#include "gui_internal.h"
#include "g3_gl.h"
#include <vector>
#include <string>
#include <cstdint>

// one corner as the graphics card gets it (36 bytes)
struct g3_vertex {
    float x, y, z;          // where
    float nx, ny, nz;       // which way the surface faces there
    float u, v;             // the point of the texture
    uint8_t r, g, b, a;     // its own colour (white: none)
};

// what a part is made of
enum { G3_TWO_SIDED = 1, G3_CUTOUT = 2, G3_BLEND = 4, G3_UNLIT = 8 };

struct g3_material {
    float color[4] = { 1, 1, 1, 1 };
    int64_t texture = 0;    // 0: none
    float shiny = 0;        // 0 .. 1
    float glow = 0;         // 0 .. : how much of its own colour it gives off
    int flags = 0;          // G3_TWO_SIDED ...
};

// Make sure the 3D side is running and its OpenGL context is the current one.
// False (with a Sharp exception raised) when this machine cannot do it.
// Every g3_begin that returned true needs its g3_end, which puts the 2D side's context back.
bool g3_begin(sni_env* env);
void g3_end();

int64_t g3_new_model();
// adds a part and gives its number, or -1 (with an exception raised) when the card refused it
int64_t g3_add_part(sni_env* env, int64_t model, const std::vector<g3_vertex>& corners,
                    const std::vector<uint32_t>& triangles, const g3_material& material);

// a texture from a picture file, or from the bytes of one (key: a name to remember it by). 0: failed
int64_t g3_texture_file(sni_env* env, const std::string& path);
int64_t g3_texture_bytes(sni_env* env, const std::string& key, const unsigned char* bytes, int length);
// a colour picture with its see-through parts taken from a second picture (white: solid). 0: failed, nothing raised
int64_t g3_texture_cut(sni_env* env, const std::string& color_path, const std::string& solid_path);
// a texture already made under this name (0: none), and one from pixels made here (four bytes a pixel)
int64_t g3_texture_known(const std::string& key);
int64_t g3_texture_pixels(const std::string& key, const unsigned char* pixels, int width, int height);
// has the texture pixels that are see-through? (leaves, fences: such a part is cut out and seen from both sides)
bool g3_texture_see_through(int64_t texture);

// g3_load.cpp: read a model file into a new model. 0 (with an exception raised) when it cannot be read
int64_t g3_load_model_file(sni_env* env, const std::string& path);

#endif //SHARPGUI_G3_INTERNAL_H
