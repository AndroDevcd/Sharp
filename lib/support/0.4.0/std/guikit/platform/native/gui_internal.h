//
// What the C++ files of the "sharpgui" library share with each other.
//
// The library is split by topic, one file each (the same rule as the rest of
// Sharp):
//
//      sharpgui.cpp    the window, the frame, the events
//      shapes.cpp      rectangles, lines, circles
//      text.cpp        fonts and drawing text
//      images.cpp      loading and drawing pictures
//
// They all paint through the ONE window and renderer kept in sharpgui.cpp.
//
#ifndef SHARPGUI_INTERNAL_H
#define SHARPGUI_INTERNAL_H

#include "sharpgui_sni.h"
#include <SDL2/SDL.h>
#include <string>

// The window and what paints it. Null while no window is open.
extern SDL_Window*   g_window;
extern SDL_Renderer* g_renderer;

// How many real pixels one unit of the program's measurements is. Everything
// the program says - sizes, positions, font sizes - is in UNITS that look the
// same size on every screen; on a dense screen (a laptop at 150 %) one unit is
// 1.5 pixels. The renderer is told this scale, so the drawing functions need
// not multiply anything themselves - except text, which is made at the real
// pixel size so that it stays sharp (text.cpp).
// (Android: dp and sp. CSS: the "px" that is not a device pixel.)
extern float g_scale;

//
// A colour arrives from Sharp as ONE whole number: red, green, blue and alpha,
// one byte each, red highest (0xRRGGBBAA). One argument instead of four keeps
// every drawing call short. See color.sharp for the packing side.
//
static inline SDL_Color gui_color(int64_t packed) {
    SDL_Color c;
    c.r = (Uint8) ((packed >> 24) & 255);
    c.g = (Uint8) ((packed >> 16) & 255);
    c.b = (Uint8) ((packed >> 8) & 255);
    c.a = (Uint8) (packed & 255);
    return c;
}

// Fonts and images are textures of the renderer: when the window closes they
// are thrown away with it (text.cpp, images.cpp).
void gui_forget_fonts();
void gui_forget_images();
void gui_forget_layers();

void sharpgui_draw_rect(sni_env* env, double x, double y, double w, double h, double radius, int64_t color);

// True when a window is open; otherwise raises a Sharp exception that says
// what to do, and the caller just returns.
bool gui_ready(sni_env* env);

#endif //SHARPGUI_INTERNAL_H
