//
// THE WINDOW'S ICON  -  surface_icon(path): the picture of the window in its
// title bar, in the taskbar and in the list of open windows.
//
// Three kinds of file are read, told apart by what is IN them (not by their name):
//
//      .ico            Windows' own icon file: several sizes in one. The largest
//                      is taken, and the system scales it down where it is small.
//      .svg            a drawing: painted at 256 pixels a side, sharp at any size
//      a picture       PNG, JPG, BMP, GIF, TGA, PSD (what image_load reads)
//
// The pictures are unpacked by stb_image.h and the drawings by nanosvg.h and
// nanosvgrast.h (public-domain and zlib-licence headers kept beside this file),
// so the only thing the GUI needs installed is still SDL2.
//
#include "gui_internal.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg.h"
#include "nanosvgrast.h"

static const int DRAWING_SIDE = 256;        // a drawing has no size of its own: this many pixels on its longer side

// a picture unpacked: 4 bytes a pixel (red, green, blue, alpha), rows from the top
struct icon_picture {
    int width = 0, height = 0;
    std::vector<unsigned char> pixels;
};

typedef std::vector<unsigned char> bytes;

static bool read_file(const std::string& path, bytes& into) {
    FILE* file = fopen(path.c_str(), "rb");
    if(file == nullptr) return false;
    unsigned char chunk[65536];
    size_t got;
    while((got = fread(chunk, 1, sizeof(chunk), file)) > 0)
        into.insert(into.end(), chunk, chunk + got);
    fclose(file);
    return !into.empty();
}

// little-endian numbers in a file; 0 when they would lie outside it
static uint32_t u16_at(const bytes& d, size_t at) { return at + 2 <= d.size() ? (uint32_t) (d[at] | (d[at + 1] << 8)) : 0; }
static uint32_t u32_at(const bytes& d, size_t at) { return at + 4 <= d.size() ? (uint32_t) d[at] | ((uint32_t) d[at + 1] << 8) | ((uint32_t) d[at + 2] << 16) | ((uint32_t) d[at + 3] << 24) : 0; }

// any picture stb_image knows, from the bytes of its file
static bool picture_from_image(const unsigned char* data, size_t length, icon_picture& into) {
    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = stbi_load_from_memory(data, (int) length, &width, &height, &channels, 4);
    if(pixels == nullptr) return false;
    into.width = width;
    into.height = height;
    into.pixels.assign(pixels, pixels + (size_t) width * height * 4);
    stbi_image_free(pixels);
    return true;
}

//
// One image of an .ico file that is not a PNG: a bitmap without its file
// header, stored bottom row first, twice as high as the picture because a
// second bitmap follows the colours - one bit a pixel, 1 where the picture is
// see-through. 32 bits a pixel carry their own alpha (the mask is only used
// when that alpha is 0 everywhere, as old files have it); 24 bits are blue,
// green, red; 8, 4 and 1 bits are places in a table of colours.
//
static bool picture_from_bitmap(const bytes& d, size_t start, size_t length, icon_picture& into) {
    if(start + 40 > d.size() || start + length > d.size()) return false;
    uint32_t header = u32_at(d, start);
    int width = (int) u32_at(d, start + 4);
    int height = (int) u32_at(d, start + 8) / 2;
    int bits = (int) u16_at(d, start + 14);
    if(u32_at(d, start + 16) != 0) return false;                // packed bitmaps: not in icons that matter
    if(width <= 0 || height <= 0 || width > 1024 || height > 1024) return false;
    if(bits != 32 && bits != 24 && bits != 8 && bits != 4 && bits != 1) return false;

    size_t colors = 0;
    if(bits <= 8) {
        colors = u32_at(d, start + 32);
        if(colors == 0) colors = (size_t) 1 << bits;
    }
    size_t table = start + header;
    size_t row = (((size_t) width * bits + 31) / 32) * 4;        // rows are padded to four bytes
    size_t mask_row = (((size_t) width + 31) / 32) * 4;
    size_t picture = table + colors * 4;
    size_t mask = picture + row * height;
    if(picture + row * height > start + length) return false;
    bool has_mask = mask + mask_row * height <= start + length;

    into.width = width;
    into.height = height;
    into.pixels.assign((size_t) width * height * 4, 0);
    bool any_alpha = false;

    for(int y = 0; y < height; y++) {
        const unsigned char* from = &d[picture + row * (size_t) (height - 1 - y)];      // bottom row first
        const unsigned char* hidden = has_mask ? &d[mask + mask_row * (size_t) (height - 1 - y)] : nullptr;
        unsigned char* to = &into.pixels[(size_t) y * width * 4];
        for(int x = 0; x < width; x++, to += 4) {
            bool see_through = hidden != nullptr && ((hidden[x / 8] >> (7 - x % 8)) & 1) != 0;
            if(bits == 32) {
                to[0] = from[x * 4 + 2]; to[1] = from[x * 4 + 1]; to[2] = from[x * 4]; to[3] = from[x * 4 + 3];
                if(to[3] != 0) any_alpha = true;
            } else if(bits == 24) {
                to[0] = from[x * 3 + 2]; to[1] = from[x * 3 + 1]; to[2] = from[x * 3];
                to[3] = see_through ? 0 : 255;
            } else {
                size_t place = bits == 8 ? from[x]
                             : bits == 4 ? (from[x / 2] >> (x % 2 == 0 ? 4 : 0)) & 15
                             : (from[x / 8] >> (7 - x % 8)) & 1;
                if(place < colors && table + place * 4 + 3 < d.size()) {
                    const unsigned char* color = &d[table + place * 4];
                    to[0] = color[2]; to[1] = color[1]; to[2] = color[0];
                }
                to[3] = see_through ? 0 : 255;
            }
        }
    }

    // 32 bits with no alpha anywhere: an old file, whose mask says what shows
    if(bits == 32 && !any_alpha) {
        for(int y = 0; y < height; y++) {
            const unsigned char* hidden = has_mask ? &d[mask + mask_row * (size_t) (height - 1 - y)] : nullptr;
            for(int x = 0; x < width; x++) {
                bool see_through = hidden != nullptr && ((hidden[x / 8] >> (7 - x % 8)) & 1) != 0;
                into.pixels[((size_t) y * width + x) * 4 + 3] = see_through ? 0 : 255;
            }
        }
    }
    return true;
}

//
// An .ico file: 6 bytes (0, 0, 1, 0 and how many images), then 16 bytes for
// each image - its width and height (0 means 256), its bits a pixel, how long
// it is and where it starts. The largest image is taken; of two the same size,
// the one with more colours.
//
static bool is_icon_file(const bytes& d) {
    return d.size() >= 22 && d[0] == 0 && d[1] == 0 && d[2] == 1 && d[3] == 0 && u16_at(d, 4) > 0;
}

static bool picture_from_icon_file(const bytes& d, icon_picture& into) {
    size_t count = u16_at(d, 4);
    size_t best_start = 0, best_length = 0;
    int best_side = 0, best_bits = 0;
    for(size_t i = 0; i < count && 6 + (i + 1) * 16 <= d.size(); i++) {
        size_t entry = 6 + i * 16;
        int side = d[entry] == 0 ? 256 : d[entry];
        int bits = (int) u16_at(d, entry + 6);
        size_t length = u32_at(d, entry + 8), start = u32_at(d, entry + 12);
        if(length == 0 || start + length > d.size()) continue;
        if(side > best_side || (side == best_side && bits > best_bits)) {
            best_side = side; best_bits = bits; best_start = start; best_length = length;
        }
    }
    if(best_length == 0) return false;

    // an image inside is either a whole PNG file or a bitmap
    static const unsigned char png_sign[4] = { 0x89, 'P', 'N', 'G' };
    if(best_length >= 4 && memcmp(&d[best_start], png_sign, 4) == 0)
        return picture_from_image(&d[best_start], best_length, into);
    return picture_from_bitmap(d, best_start, best_length, into);
}

// a drawing: `<svg` stands near the top of the file
static bool is_drawing(const bytes& d) {
    std::string top((const char*) d.data(), d.size() < 4096 ? d.size() : 4096);
    return top.find("<svg") != std::string::npos;
}

static bool picture_from_drawing(const bytes& d, icon_picture& into) {
    std::string text((const char*) d.data(), d.size());            // (the reader changes the text it is given)
    NSVGimage* drawing = nsvgParse(&text[0], "px", 96);
    if(drawing == nullptr) return false;
    bool painted = false;
    if(drawing->width > 0 && drawing->height > 0) {
        float longer = drawing->width > drawing->height ? drawing->width : drawing->height;
        float scale = DRAWING_SIDE / longer;
        int width = (int) ceilf(drawing->width * scale), height = (int) ceilf(drawing->height * scale);
        NSVGrasterizer* painter = nsvgCreateRasterizer();
        if(painter != nullptr) {
            into.width = width;
            into.height = height;
            into.pixels.assign((size_t) width * height * 4, 0);
            nsvgRasterize(painter, drawing, 0, 0, scale, into.pixels.data(), width, height, width * 4);
            nsvgDeleteRasterizer(painter);
            painted = true;
        }
    }
    nsvgDelete(drawing);
    return painted;
}

//
// surface_icon(path): set the window's icon from an .ico file, an .svg drawing
// or a picture. Returns 1 when it is set, 0 while no window is open or when
// the file could not be read (an app does not stop for want of its icon).
//
int64_t sharpgui_surface_icon(sni_env* env, sni_ref path_ref) {
    if(g_window == nullptr) return 0;
    std::string path = env->text(path_ref);

    bytes file;
    if(!read_file(path, file)) return 0;

    icon_picture icon;
    bool read = is_icon_file(file) ? picture_from_icon_file(file, icon)
              : is_drawing(file)   ? picture_from_drawing(file, icon)
              : picture_from_image(file.data(), file.size(), icon);
    if(!read) return 0;

    SDL_Surface* picture = SDL_CreateRGBSurfaceWithFormatFrom(icon.pixels.data(), icon.width, icon.height, 32,
                                                              icon.width * 4, SDL_PIXELFORMAT_RGBA32);
    if(picture == nullptr) return 0;
    SDL_SetWindowIcon(g_window, picture);       // (SDL copies it)
    SDL_FreeSurface(picture);
    return 1;
}
