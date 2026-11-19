//
// IMAGES  -  loading pictures (PNG, JPG, BMP, GIF) and drawing them.
//
// A picture file is unpacked ONCE into a texture on the graphics card; drawing
// it afterwards - at any size, any number of times - is a fast copy. Loading
// the same file again hands back the picture already loaded.
//
// The unpacking is done by stb_image.h (one public-domain header, kept beside
// this file), so the only thing the GUI needs installed is SDL2.
//
#include "gui_internal.h"
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

struct gui_image {
    SDL_Texture* texture = nullptr;
    int width = 0, height = 0;
    bool smooth = true;         // scaled smoothly (image_smooth)
};

static std::vector<gui_image> g_images;                 // image id = place in this list + 1
static std::map<std::string, int64_t> g_image_ids;      // path -> id, so a file is only loaded once

void gui_forget_images() {
    for(gui_image& image : g_images)
        if(image.texture != nullptr) SDL_DestroyTexture(image.texture);
    g_images.clear();
    g_image_ids.clear();
}

static gui_image* image_of(sni_env* env, int64_t id) {
    if(id >= 1 && id <= (int64_t) g_images.size()) return &g_images[(size_t) id - 1];
    env->raise("std#illegal_argument_exception", "that image does not belong to the open window (images end when their window closes)");
    return nullptr;
}

// image_load(path): read a picture file and make it ready to draw. Returns its id.
int64_t sharpgui_image_load(sni_env* env, sni_ref path_ref) {
    if(!gui_ready(env)) return 0;
    std::string path = env->text(path_ref);

    auto known = g_image_ids.find(path);
    if(known != g_image_ids.end()) return known->second;

    // always unpack to 4 bytes a pixel: red, green, blue, alpha
    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if(pixels == nullptr) {
        env->raise("std#io_exception", "the image could not be loaded: " + path + " (" + stbi_failure_reason() + ")");
        return 0;
    }

    gui_image image;
    image.width = width;
    image.height = height;
    image.texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if(image.texture != nullptr) {
        SDL_UpdateTexture(image.texture, nullptr, pixels, width * 4);
        SDL_SetTextureBlendMode(image.texture, SDL_BLENDMODE_BLEND);
    }
    stbi_image_free(pixels);

    if(image.texture == nullptr) {
        env->raise("std#illegal_state_exception", std::string("the image could not be prepared for drawing: ") + SDL_GetError());
        return 0;
    }

    g_images.push_back(image);
    int64_t id = (int64_t) g_images.size();
    g_image_ids[path] = id;
    return id;
}

// image_width(id) / image_height(id): the picture's own size in pixels.
int32_t sharpgui_image_width(sni_env* env, int64_t id) {
    gui_image* image = image_of(env, id);
    return image == nullptr ? 0 : image->width;
}

int32_t sharpgui_image_height(sni_env* env, int64_t id) {
    gui_image* image = image_of(env, id);
    return image == nullptr ? 0 : image->height;
}

//
// draw_image(id, x, y, w, h, opacity): paint the picture stretched to the area
// (x, y, w, h). opacity 255 is solid, less lets what is behind show through.
//
void sharpgui_draw_image(sni_env* env, int64_t id, double x, double y, double w, double h, int32_t opacity) {
    if(!gui_ready(env)) return;
    gui_image* image = image_of(env, id);
    if(image == nullptr || w <= 0 || h <= 0) return;

    SDL_FRect to = { (float) x, (float) y, (float) w, (float) h };
    SDL_SetTextureAlphaMod(image->texture, (Uint8) (opacity < 0 ? 0 : opacity > 255 ? 255 : opacity));
    SDL_RenderCopyF(g_renderer, image->texture, nullptr, &to);
}


// =================================================================================
// SPRITES  -  parts of a picture
// =================================================================================
//
// A sprite sheet is one picture holding many small ones (the frames of a
// character's run, the tiles of a level). Drawing a sprite is drawing one BOX
// of the picture - the same fast copy as a whole picture.
//

static Uint8 byte_of(int32_t opacity) {
    return (Uint8) (opacity < 0 ? 0 : opacity > 255 ? 255 : opacity);
}

//
// image_smooth(id, on): how the picture is scaled. 1, how every picture starts:
// smoothly, right for photos and drawn art. 0: every pixel stays a sharp square,
// which is what pixel art needs (smoothed, it turns to mush when enlarged).
//
void sharpgui_image_smooth(sni_env* env, int64_t id, int64_t on) {
    if(!gui_ready(env)) return;
    gui_image* image = image_of(env, id);
    if(image == nullptr) return;
    image->smooth = on != 0;
#if SDL_VERSION_ATLEAST(2, 0, 12)
    SDL_SetTextureScaleMode(image->texture, image->smooth ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
#endif
}

//
// draw_image_part(id, sx, sy, sw, sh, x, y, w, h, opacity, flip, angle): the box
// (sx, sy, sw, sh) of the picture, in the picture's own pixels, stretched to the
// area (x, y, w, h). flip 1 mirrors it left to right, 2 top to bottom, 3 both;
// angle turns it that many degrees clockwise around the area's middle.
//
void sharpgui_draw_image_part(sni_env* env, int64_t id, double sx, double sy, double sw, double sh,
                              double x, double y, double w, double h, int32_t opacity, int32_t flip, double angle) {
    if(!gui_ready(env)) return;
    gui_image* image = image_of(env, id);
    if(image == nullptr || w <= 0 || h <= 0 || sw <= 0 || sh <= 0) return;

    SDL_Rect from = { (int) sx, (int) sy, (int) sw, (int) sh };
    SDL_FRect to = { (float) x, (float) y, (float) w, (float) h };
    SDL_SetTextureAlphaMod(image->texture, byte_of(opacity));
    if(flip == 0 && angle == 0) {
        SDL_RenderCopyF(g_renderer, image->texture, &from, &to);
    } else {
        int how = SDL_FLIP_NONE;
        if(flip & 1) how |= SDL_FLIP_HORIZONTAL;
        if(flip & 2) how |= SDL_FLIP_VERTICAL;
        SDL_RenderCopyExF(g_renderer, image->texture, &from, &to, angle, nullptr, (SDL_RendererFlip) how);
    }
}

// Parts of ONE picture being gathered; show() paints them all - with triangles
// in one go, where SDL has them.
struct part_run {
#if GUI_HAS_GEOMETRY
    std::vector<SDL_Vertex> verts;
    std::vector<int> indices;
#else
    struct part { SDL_Rect from; SDL_FRect to; bool mirrored; };
    std::vector<part> parts;
#endif

    // the box (sx, sy, sw, sh) of the picture on the area (x, y, w, h); mirrored: left and right swapped
    void add(const gui_image* image, float sx, float sy, float sw, float sh, bool mirrored,
             float x, float y, float w, float h, Uint8 alpha) {
        if(w <= 0 || h <= 0 || sw <= 0 || sh <= 0) return;
#if GUI_HAS_GEOMETRY
        // a hair inside the box, so a neighbour in the sheet never bleeds in at the edge
        // (half a pixel when the picture is smoothed: that is how far smoothing reaches)
        float inset = image->smooth ? 0.5f : 0.01f;
        float u0 = (sx + inset) / image->width, u1 = (sx + sw - inset) / image->width;
        float v0 = (sy + inset) / image->height, v1 = (sy + sh - inset) / image->height;
        if(mirrored) std::swap(u0, u1);

        int first = (int) verts.size();
        const float px[4] = { x, x + w, x + w, x }, py[4] = { y, y, y + h, y + h };
        const float tu[4] = { u0, u1, u1, u0 }, tv[4] = { v0, v0, v1, v1 };
        for(int i = 0; i < 4; i++) {
            SDL_Vertex v;
            v.position.x = px[i];
            v.position.y = py[i];
            v.color.r = 255;
            v.color.g = 255;
            v.color.b = 255;
            v.color.a = alpha;
            v.tex_coord.x = tu[i];
            v.tex_coord.y = tv[i];
            verts.push_back(v);
        }
        int two[6] = { first, first + 1, first + 2, first, first + 2, first + 3 };
        indices.insert(indices.end(), two, two + 6);
#else
        part p;
        p.from = { (int) sx, (int) sy, (int) sw, (int) sh };
        p.to = { x, y, w, h };
        p.mirrored = mirrored;
        parts.push_back(p);
#endif
    }

    void show(gui_image* image, Uint8 alpha) {
#if GUI_HAS_GEOMETRY
        if(!indices.empty()) {
            SDL_SetTextureAlphaMod(image->texture, 255);        // the alpha is in every corner's colour
            SDL_RenderGeometry(g_renderer, image->texture, verts.data(), (int) verts.size(), indices.data(), (int) indices.size());
        }
        verts.clear();
        indices.clear();
#else
        SDL_SetTextureAlphaMod(image->texture, alpha);
        for(const part& p : parts) {
            if(p.mirrored) SDL_RenderCopyExF(g_renderer, image->texture, &p.from, &p.to, 0, nullptr, SDL_FLIP_HORIZONTAL);
            else SDL_RenderCopyF(g_renderer, image->texture, &p.from, &p.to);
        }
        parts.clear();
#endif
    }
};

static part_run g_parts;        // one for every call (see g_run in shapes.cpp)

//
// draw_image_parts(id, from, to, count, shift_x, shift_y, opacity): `count`
// sprites of one picture in one call (sprite_batch.sharp fills the lists).
// Sprite i is the box from[4i ...] = sx sy sw sh of the picture - a NEGATIVE sw
// means that box mirrored left to right - put on the area to[4i ...] = x y w h,
// moved by the shift (the painter's origin).
//
void sharpgui_draw_image_parts(sni_env* env, int64_t id, sni_ref from_ref, sni_ref to_ref, int64_t count,
                               double shift_x, double shift_y, int32_t opacity) {
    if(!gui_ready(env) || count <= 0) return;
    gui_image* image = image_of(env, id);
    if(image == nullptr) return;
    sni_array from, to;
    if(!gui_numbers(env, from_ref, SNI_DECIMAL, count * 4, "the batch's list of source boxes", &from)) return;
    if(!gui_numbers(env, to_ref, SNI_DECIMAL, count * 4, "the batch's list of places", &to)) return;

    const double* s = (const double*) from.data;
    const double* d = (const double*) to.data;
    Uint8 alpha = byte_of(opacity);
    for(int64_t i = 0; i < count; i++, s += 4, d += 4) {
        bool mirrored = s[2] < 0;
        g_parts.add(image, (float) s[0], (float) s[1], (float) (mirrored ? -s[2] : s[2]), (float) s[3], mirrored,
                    (float) (d[0] + shift_x), (float) (d[1] + shift_y), (float) d[2], (float) d[3], alpha);
    }
    g_parts.show(image, alpha);
}

// a position moved to the nearest real pixel, so two tiles side by side share their edge exactly
static float on_a_pixel(double v) {
    return std::floor((float) v * g_scale + 0.5f) / g_scale;
}

//
// draw_tiles(...): a grid of tiles from a tile set, in one call (tile_map.sharp).
//
//      tiles                       the whole map, row after row: map_columns x map_rows numbers.
//                                  0 is "nothing here"; n is frame n - 1 of the tile set
//      first_column, first_row,    the part of the map to paint (cut down to the map here)
//      columns, rows
//      frame_w, frame_h,           the tile set: frames of this size, `margin` pixels in from the
//      margin, spacing,            picture's edge, `spacing` pixels apart, sheet_columns in a row
//      sheet_columns
//      x, y                        where the map's top-left corner (tile 0, 0) is on the screen
//      tile_w, tile_h              the size a tile is painted at
//
void sharpgui_draw_tiles(sni_env* env, int64_t id, sni_ref tiles_ref, int64_t map_columns, int64_t map_rows,
                         int64_t first_column, int64_t first_row, int64_t columns, int64_t rows,
                         double frame_w, double frame_h, double margin, double spacing, int64_t sheet_columns,
                         double x, double y, double tile_w, double tile_h, int32_t opacity) {
    if(!gui_ready(env)) return;
    gui_image* image = image_of(env, id);
    if(image == nullptr || map_columns <= 0 || map_rows <= 0 || sheet_columns <= 0 || tile_w <= 0 || tile_h <= 0) return;
    sni_array tiles;
    if(!gui_numbers(env, tiles_ref, SNI_INT32, map_columns * map_rows, "the map's list of tiles", &tiles)) return;

    int64_t last_column = std::min(first_column + columns, map_columns);
    int64_t last_row = std::min(first_row + rows, map_rows);
    if(first_column < 0) first_column = 0;
    if(first_row < 0) first_row = 0;

    const int32_t* tile = (const int32_t*) tiles.data;
    Uint8 alpha = byte_of(opacity);
    for(int64_t r = first_row; r < last_row; r++) {
        float top = on_a_pixel(y + r * tile_h), bottom = on_a_pixel(y + (r + 1) * tile_h);
        for(int64_t c = first_column; c < last_column; c++) {
            int32_t number = tile[r * map_columns + c];
            if(number <= 0) continue;
            int64_t frame = number - 1;
            float sx = (float) (margin + (frame % sheet_columns) * (frame_w + spacing));
            float sy = (float) (margin + (frame / sheet_columns) * (frame_h + spacing));
            float left = on_a_pixel(x + c * tile_w), right = on_a_pixel(x + (c + 1) * tile_w);
            g_parts.add(image, sx, sy, (float) frame_w, (float) frame_h, false, left, top, right - left, bottom - top, alpha);
        }
    }
    g_parts.show(image, alpha);
}
