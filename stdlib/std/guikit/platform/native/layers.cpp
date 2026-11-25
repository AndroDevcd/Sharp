//
// layers.cpp - painting a part of the screen "to one side" first, so that the
// whole part can then be put on the screen turned, enlarged, blurred, faded
// or cut to a shape.
//
// WHY: the drawing functions (rectangles, text, images) only know straight,
// sharp, upright. To rotate a card with everything on it, the card is first
// painted - upright, as always - into a picture of its own (a LAYER), and that
// one picture is then drawn rotated. Blur and cutting to a shape work the same
// way: paint normally into a layer, then treat the layer.
//
//      layer_begin()                       everything painted from now on goes into a new layer
//      ... the usual draw_ calls ...
//      layer_mask(x, y, w, h, radius)      (optional) keep only what is inside this rounded box
//      layer_end(x, y, w, h, angle, scale, opacity, blur)
//                                          put that area of the layer onto what is underneath
//
// Layers nest (a rotated card inside a blurred panel): each layer_begin goes
// one deeper, each layer_end comes back up and draws onto the layer below -
// or, from the first one, onto the window.
//
// A layer is a texture the size of the window that the graphics card can
// paint INTO (SDL: a "render target"). They are made once per depth and kept.
// (Android: View.setLayerType / RenderNode. Compose: Modifier.graphicsLayer.
// CSS: every `transform` or `filter` makes one.)
//
// COLOURS IN A LAYER ARE "PREMULTIPLIED". When something half see-through is
// painted into an EMPTY layer, the graphics card stores colour x opacity, and
// the opacity. Drawing the layer the ordinary way would multiply by the
// opacity a second time and darken every soft edge. So a layer is drawn with
// its own blending rule: "the colour is already multiplied - just add it".
//

#include "gui_internal.h"
#include <vector>
#include <cmath>

static std::vector<SDL_Texture*> g_layers;      // one per depth, made on first use
static int g_layer_w = 0, g_layer_h = 0;        // the size they were made for (the window's, in real pixels)
static int g_depth = 0;                         // how many layers are open right now
static std::vector<SDL_Texture*> g_steps;       // blurring: step k is the window at 1 / 2^(k+1) of its size
static std::vector<SDL_Texture*> g_spare;       // ... and a second picture of each size, to blur across into

void gui_forget_layers() {
    for(SDL_Texture* layer : g_layers) SDL_DestroyTexture(layer);
    g_layers.clear();
    for(SDL_Texture* step : g_steps) SDL_DestroyTexture(step);
    g_steps.clear();
    for(SDL_Texture* step : g_spare) SDL_DestroyTexture(step);
    g_spare.clear();
    g_layer_w = g_layer_h = 0;
    g_depth = 0;
}

// "the colour is already multiplied by the opacity": result = layer + what is there x (1 - layer's opacity)
static SDL_BlendMode premultiplied() {
    return SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
                                      SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
}

// Paint into `target` (nullptr: the window) and put the drawing scale back:
// SDL forgets the scale whenever the target changes.
static void paint_into(SDL_Texture* target) {
    SDL_SetRenderTarget(g_renderer, target);
    SDL_RenderSetScale(g_renderer, g_scale, g_scale);
}

static SDL_Texture* new_layer(int w, int h) {
    SDL_Texture* made = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if(made != nullptr) {
        if(SDL_SetTextureBlendMode(made, premultiplied()) != 0)
            SDL_SetTextureBlendMode(made, SDL_BLENDMODE_BLEND);     // a renderer without custom rules: a little darker at soft edges
        SDL_SetTextureScaleMode(made, SDL_ScaleModeLinear);        // smooth when turned or enlarged
    }
    return made;
}

// the layer at a depth, made (or made again after the window changed size) when needed
static SDL_Texture* layer_at(int depth) {
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(g_renderer, &w, &h);
    if(depth == 0 && g_depth == 0 && (w != g_layer_w || h != g_layer_h)) {
        // (only when no layer is open: the output size of a texture target is the texture's)
        for(SDL_Texture* layer : g_layers) SDL_DestroyTexture(layer);
        g_layers.clear();
        for(SDL_Texture* step : g_steps) SDL_DestroyTexture(step);
        g_steps.clear();
        for(SDL_Texture* step : g_spare) SDL_DestroyTexture(step);
        g_spare.clear();
        g_layer_w = w;
        g_layer_h = h;
    }
    while((int) g_layers.size() <= depth) {
        SDL_Texture* made = new_layer(g_layer_w, g_layer_h);
        if(made == nullptr) return nullptr;
        g_layers.push_back(made);
    }
    return g_layers[depth];
}

static void clear_target() {
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 0);
    SDL_RenderSetClipRect(g_renderer, nullptr);
    SDL_RenderClear(g_renderer);
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
}

//
// layer_begin(): from now on, paint into a new, empty layer. Returns 1, or 0
// when the graphics card would not make one (then painting simply carries on
// onto what was there, and layer_end must not be called).
//
int64_t sharpgui_layer_begin(sni_env* env) {
    if(!gui_ready(env)) return 0;
    if(g_depth == 0) SDL_SetRenderTarget(g_renderer, nullptr);     // so the window's own size is what is measured
    SDL_Texture* layer = layer_at(g_depth);
    if(layer == nullptr) return 0;

    g_depth++;
    paint_into(layer);
    clear_target();
    return 1;
}

//
// layer_mask(x, y, w, h, radius): of what has been painted into the open layer
// so far, keep only what is inside this box with round corners; the rest
// becomes empty. (radius = half the height: a pill; half of a square: a circle.)
// Returns 0 when this graphics card cannot do it - then nothing is cut.
//
int64_t sharpgui_layer_mask(sni_env* env, double x, double y, double w, double h, double radius) {
    if(!gui_ready(env) || g_depth == 0) return 0;
    SDL_Texture* layer = g_layers[g_depth - 1];
    SDL_Texture* stencil = layer_at(g_depth);                       // the next free layer serves as the stencil
    if(stencil == nullptr) return 0;

    // "multiply what is there by my opacity": opaque keeps, empty erases
    SDL_BlendMode keep_inside = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
                                                           SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
    if(SDL_SetTextureBlendMode(stencil, keep_inside) != 0) {
        return 0;
    }

    // the stencil: empty everywhere, solid white inside the shape
    paint_into(stencil);
    clear_target();
    sharpgui_draw_rect(env, x, y, w, h, radius, 0xFFFFFFFF);

    // press it onto the layer (pixel for pixel: no scale)
    SDL_SetRenderTarget(g_renderer, layer);
    SDL_RenderSetScale(g_renderer, 1, 1);
    SDL_RenderSetClipRect(g_renderer, nullptr);
    SDL_RenderCopy(g_renderer, stencil, nullptr, nullptr);
    SDL_RenderSetScale(g_renderer, g_scale, g_scale);

    // the stencil texture is an ordinary layer again
    if(SDL_SetTextureBlendMode(stencil, premultiplied()) != 0)
        SDL_SetTextureBlendMode(stencil, SDL_BLENDMODE_BLEND);
    return 1;
}

//
// layer_end(x, y, w, h, angle, scale, opacity, blur): close the open layer and
// draw its area (x, y, w, h) onto what is underneath -
//
//      angle       turned this many degrees clockwise around the area's middle
//      scale       enlarged (2 = twice the size) or shrunk (0.5) around the middle
//      opacity     0 .. 255
//      blur        0 = sharp; more = softer (about this many pixels)
//
// BLUR is a real Gaussian blur - every pixel becomes a weighted average of
// its neighbours, the near ones counting most, falling off in a bell curve -
// made affordable by two old tricks:
//
//   1. DO IT SMALL. The area is first halved, and halved again, until the
//      blur wanted is only a pixel or two wide at that size (a pyramid:
//      full -> 1/2 -> 1/4 ...). Never more than a factor of two per step:
//      halving with smooth scaling averages every 2 x 2 pixels properly,
//      where one big shrink would skip most of them and show as squares.
//
//   2. ACROSS, THEN DOWN. A bell curve in two directions is the same as one
//      across followed by one down. So instead of averaging a whole disc of
//      neighbours per pixel, the small picture is drawn nine times shifted
//      sideways, each time with the weight of that distance, ADDING up - then
//      the result nine times shifted vertically. Eighteen draws of a tiny
//      picture, whatever the blur.
//
// Then back up the pyramid, doubling each time (which smooths once more), and
// onto the screen. Games blur this way for bloom and depth of field.
//
// The nine weights: a bell curve sampled every half sigma out to two sigmas
// each side, scaled so that they add up to 1 (in 255ths, because that is how
// a draw is given its strength).
static const int   TAPS = 9;
static const float TAP_AT[TAPS]     = { -2.0f, -1.5f, -1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 1.5f, 2.0f };    // in sigmas
static const Uint8 TAP_WEIGHT[TAPS] = { 7, 17, 32, 46, 51, 46, 32, 17, 7 };                            // sums to 255

// Draw `area` of `from` into the same area of `into` nine times, shifted
// along one direction (dx, dy are one sigma, in pixels of this size), adding.
static void bell_pass(SDL_Texture* from, SDL_Texture* into, const SDL_Rect& area, float dx, float dy) {
    SDL_SetRenderTarget(g_renderer, into);
    SDL_RenderSetScale(g_renderer, 1, 1);
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 0);
    SDL_RenderSetClipRect(g_renderer, nullptr);
    SDL_RenderClear(g_renderer);
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

    for(int i = 0; i < TAPS; i++) {
        SDL_FRect to = { area.x + dx * TAP_AT[i], area.y + dy * TAP_AT[i], (float) area.w, (float) area.h };
        SDL_SetTextureColorMod(from, TAP_WEIGHT[i], TAP_WEIGHT[i], TAP_WEIGHT[i]);
        SDL_SetTextureAlphaMod(from, TAP_WEIGHT[i]);
        SDL_RenderCopyF(g_renderer, from, &area, &to);
    }
    SDL_SetTextureColorMod(from, 255, 255, 255);
    SDL_SetTextureAlphaMod(from, 255);
}

void sharpgui_layer_end(sni_env* env, double x, double y, double w, double h,
                        double angle, double scale, int64_t opacity, double blur) {
    if(!gui_ready(env) || g_depth == 0) return;
    g_depth--;
    SDL_Texture* layer = g_layers[g_depth];
    SDL_Texture* below = g_depth > 0 ? g_layers[g_depth - 1] : nullptr;

    // the area in real pixels, kept inside the layer
    SDL_Rect from = { (int) std::floor(x * g_scale), (int) std::floor(y * g_scale),
                      (int) std::ceil(w * g_scale), (int) std::ceil(h * g_scale) };
    if(from.x < 0) { from.w += from.x; from.x = 0; }
    if(from.y < 0) { from.h += from.y; from.y = 0; }
    if(from.x + from.w > g_layer_w) from.w = g_layer_w - from.x;
    if(from.y + from.h > g_layer_h) from.h = g_layer_h - from.y;
    if(from.w <= 0 || from.h <= 0) {
        paint_into(below);
        return;
    }

    SDL_Texture* picture = layer;
    SDL_Rect picture_from = from;

    if(blur > 0.5) {
        // `blur` is the sigma of the bell curve, in units; here in real pixels
        float sigma = (float) (blur * g_scale);

        // how many halvings, so that the sigma left over is between one and two pixels of the smallest picture
        int steps = 1;
        while(steps < 6 && (float) (1 << (steps + 1)) <= sigma) steps++;
        float sigma_small = sigma / (float) (1 << steps);

        // the pictures of the pyramid (and a spare of each size): step k holds the window at 1 / 2^(k+1)
        while((int) g_steps.size() < steps) {
            int k = (int) g_steps.size() + 1;
            SDL_Texture* made = new_layer((g_layer_w >> k) + 16, (g_layer_h >> k) + 16);
            SDL_Texture* spare = new_layer((g_layer_w >> k) + 16, (g_layer_h >> k) + 16);
            if(made == nullptr || spare == nullptr) break;
            g_steps.push_back(made);
            g_spare.push_back(spare);
        }
        if((int) g_steps.size() < steps) steps = (int) g_steps.size();

        if(steps > 0) {
            SDL_SetTextureColorMod(layer, 255, 255, 255);
            SDL_SetTextureAlphaMod(layer, 255);

            // DOWN: each step is the one before at exactly half the size
            std::vector<SDL_Rect> areas;
            SDL_Texture* source = layer;
            SDL_Rect source_area = from;
            for(int k = 0; k < steps; k++) {
                SDL_Rect area = { 8, 8, (source_area.w + 1) / 2, (source_area.h + 1) / 2 };  // (in from the edge: room for the blur to spread into)
                SDL_SetRenderTarget(g_renderer, g_steps[k]);
                SDL_RenderSetScale(g_renderer, 1, 1);
                clear_target();
                SDL_RenderCopy(g_renderer, source, &source_area, &area);
                areas.push_back(area);
                source = g_steps[k];
                source_area = area;
            }

            // THE BELL CURVE, at the smallest size: across into the spare, then down back into the step.
            // The weights must ADD UP, so for these draws the pictures are blended by plain addition.
            SDL_Texture* smallest = g_steps[steps - 1];
            SDL_Texture* spare = g_spare[steps - 1];
            SDL_BlendMode adding = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
                                                              SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
            if(SDL_SetTextureBlendMode(smallest, adding) == 0 && SDL_SetTextureBlendMode(spare, adding) == 0) {
                bell_pass(smallest, spare, areas[steps - 1], sigma_small, 0);
                bell_pass(spare, smallest, areas[steps - 1], 0, sigma_small);
            }
            if(SDL_SetTextureBlendMode(smallest, premultiplied()) != 0) SDL_SetTextureBlendMode(smallest, SDL_BLENDMODE_BLEND);
            if(SDL_SetTextureBlendMode(spare, premultiplied()) != 0) SDL_SetTextureBlendMode(spare, SDL_BLENDMODE_BLEND);

            // UP: back through the same pictures, each drawn at exactly double into the one above
            for(int k = steps - 1; k > 0; k--) {
                SDL_SetRenderTarget(g_renderer, g_steps[k - 1]);
                SDL_RenderSetScale(g_renderer, 1, 1);
                clear_target();
                SDL_RenderCopy(g_renderer, g_steps[k], &areas[k], &areas[k - 1]);
            }
            picture = g_steps[0];
            picture_from = areas[0];
        }
    }

    paint_into(below);

    // where it lands: the same area (in the units everything is drawn in), grown or shrunk around its middle
    float fx = from.x / g_scale, fy = from.y / g_scale, fw = from.w / g_scale, fh = from.h / g_scale;
    float grown_w = fw * (float) scale, grown_h = fh * (float) scale;
    SDL_FRect to = { fx + (fw - grown_w) / 2, fy + (fh - grown_h) / 2, grown_w, grown_h };

    Uint8 solid = (Uint8) (opacity < 0 ? 0 : (opacity > 255 ? 255 : opacity));
    SDL_SetTextureColorMod(picture, solid, solid, solid);           // premultiplied: fading scales the colour too
    SDL_SetTextureAlphaMod(picture, solid);
    SDL_RenderCopyExF(g_renderer, picture, &picture_from, &to, angle, nullptr, SDL_FLIP_NONE);
    SDL_SetTextureColorMod(picture, 255, 255, 255);
    SDL_SetTextureAlphaMod(picture, 255);
}
