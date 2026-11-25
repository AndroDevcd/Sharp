//
// TEXT  -  fonts, measuring text, drawing text.
//
// A font file (.ttf) describes each letter as curves. Turning curves into
// pixels for every letter of every frame would be far too slow, so we do what
// every UI toolkit does:
//
//      When a font is loaded at a size, every letter is painted ONCE into one
//      big picture (the "atlas"). Drawing text is then just copying little
//      rectangles out of that picture - which a graphics card does very fast.
//
// The curve-to-pixel work is done by stb_truetype.h (one public-domain header,
// kept beside this file), so the only thing the GUI needs installed is SDL2.
//
// A font here is (file, size): the same file at 14 and at 32 is two fonts, each
// with its own atlas. Asking for the same one again hands back the one already
// made, so `gui.font(16)` in a draw loop costs nothing.
//
// The letters covered are code points 32..255 (English and Western European).
// Anything else is drawn as `?` for now.
//
#include "gui_internal.h"
#include <vector>
#include <map>
#include <cstdio>
#include <cmath>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

static const int FIRST_LETTER = 32;
static const int LETTER_COUNT = 224;        // 32..255

struct gui_font {
    std::vector<unsigned char> file;        // the .ttf, kept: stb reads it whenever it is asked something
    stbtt_fontinfo info;
    float scale = 0;                        // from the font's own units to pixels at this size
    float ascent = 0;                       // pixels from the top of a line down to where letters sit
    float line_height = 0;                  // pixels from one line of text to the next
    float made_for = 1;                     // the drawing scale the letters were made at (real pixels per unit)
    SDL_Texture* atlas = nullptr;           // the picture of all the letters
    int atlas_size = 0;                     // it is square: this many pixels each way
    stbtt_packedchar letters[LETTER_COUNT]; // where each letter is in the atlas, and how far to step after it
};

static std::vector<gui_font*> g_fonts;                  // font id = place in this list + 1
static std::map<std::string, int64_t> g_font_ids;       // "path|size" -> id, so a font is only made once

void gui_forget_fonts() {
    for(gui_font* font : g_fonts) {
        if(font->atlas != nullptr) SDL_DestroyTexture(font->atlas);
        delete font;
    }
    g_fonts.clear();
    g_font_ids.clear();
}

static gui_font* font_of(sni_env* env, int64_t id) {
    if(id >= 1 && id <= (int64_t) g_fonts.size()) return g_fonts[(size_t) id - 1];
    env->raise("std#illegal_argument_exception", "that font does not belong to the open window (fonts end when their window closes)");
    return nullptr;
}

static bool read_whole_file(const std::string& path, std::vector<unsigned char>& bytes) {
    FILE* file = fopen(path.c_str(), "rb");
    if(file == nullptr) return false;
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    bool ok = size > 0;
    if(ok) {
        bytes.resize((size_t) size);
        ok = fread(bytes.data(), 1, (size_t) size, file) == (size_t) size;
    }
    fclose(file);
    return ok;
}

//
// The font used when a program does not name one: the first of these that is
// on the machine. Every system ships at least one of them.
//
static std::string default_font_path() {
    static const char* candidates[] = {
#ifdef _WIN32
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf",
#elif defined(__APPLE__)
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/piboto/Piboto-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
#endif
    };
    for(const char* path : candidates) {
        FILE* file = fopen(path, "rb");
        if(file != nullptr) {
            fclose(file);
            return path;
        }
    }
    return "";
}

//
// The BOLD companion of the standard font (Sharp asks for it with the path
// "*bold"). When a machine has none of these, the regular font is used: bold
// text then simply looks like ordinary text, which beats an error.
//
static std::string default_bold_font_path() {
    static const char* candidates[] = {
#ifdef _WIN32
        "C:\\Windows\\Fonts\\segoeuib.ttf",
        "C:\\Windows\\Fonts\\arialbd.ttf",
        "C:\\Windows\\Fonts\\tahomabd.ttf",
#elif defined(__APPLE__)
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "/Library/Fonts/Arial Bold.ttf",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/piboto/Piboto-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf",
#endif
    };
    for(const char* path : candidates) {
        FILE* file = fopen(path, "rb");
        if(file != nullptr) {
            fclose(file);
            return path;
        }
    }
    return default_font_path();
}

//
// Paint all the letters into one square picture and hand it to the graphics
// card. The picture starts small and is doubled until the letters fit.
//
static bool build_atlas(gui_font* font, float size) {
    // small text is painted at twice the detail sideways, which keeps it sharp
    // when a letter lands between two pixels
    int detail = size <= 36 ? 2 : 1;

    for(int side = 256; side <= 4096; side *= 2) {
        std::vector<unsigned char> coverage((size_t) side * side);      // one byte per pixel: how much ink

        stbtt_pack_context pack;
        if(!stbtt_PackBegin(&pack, coverage.data(), side, side, 0, 1, nullptr)) return false;
        stbtt_PackSetOversampling(&pack, detail, detail);
        int fitted = stbtt_PackFontRange(&pack, font->file.data(), 0, STBTT_POINT_SIZE(size),
                                         FIRST_LETTER, LETTER_COUNT, font->letters);
        stbtt_PackEnd(&pack);
        if(!fitted) continue;                                           // too small: try the next size up

        // ink coverage -> white pixels of that much opacity (the colour is mixed in when drawing)
        std::vector<unsigned char> pixels((size_t) side * side * 4);
        for(size_t i = 0; i < coverage.size(); i++) {
            pixels[i * 4 + 0] = 255;
            pixels[i * 4 + 1] = 255;
            pixels[i * 4 + 2] = 255;
            pixels[i * 4 + 3] = coverage[i];
        }

        font->atlas = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, side, side);
        if(font->atlas == nullptr) return false;
        SDL_UpdateTexture(font->atlas, nullptr, pixels.data(), side * 4);
        SDL_SetTextureBlendMode(font->atlas, SDL_BLENDMODE_BLEND);
        font->atlas_size = side;
        return true;
    }
    return false;
}

//
// font_load(path, size): make a font ready at a size (in pixels, like CSS px).
// path "" means the system's standard font. Returns the font's id.
//
int64_t sharpgui_font_load(sni_env* env, sni_ref path_ref, double size) {
    if(!gui_ready(env)) return 0;
    if(size < 4 || size > 400) {
        env->raise("std#illegal_argument_exception", "a font size is between 4 and 400 pixels");
        return 0;
    }

    std::string path = env->text(path_ref);
    if(path == "*bold") {
        path = default_bold_font_path();        // "" when the machine has no standard font at all: reported just below
    }
    if(path.empty()) {
        path = default_font_path();
        if(path.empty()) {
            env->raise("std#io_exception", "no standard font was found on this machine: name a .ttf file, gui.font(\"my.ttf\", size)");
            return 0;
        }
    }

    char size_text[32];
    snprintf(size_text, sizeof(size_text), "|%.2f@%.2f", size, g_scale);
    std::string key = path + size_text;
    auto known = g_font_ids.find(key);
    if(known != g_font_ids.end()) return known->second;

    gui_font* font = new gui_font();
    if(!read_whole_file(path, font->file)) {
        delete font;
        env->raise("std#io_exception", "the font file could not be read: " + path);
        return 0;
    }
    if(!stbtt_InitFont(&font->info, font->file.data(), stbtt_GetFontOffsetForIndex(font->file.data(), 0))) {
        delete font;
        env->raise("std#io_exception", "that file is not a font this library can read (.ttf / .otf): " + path);
        return 0;
    }

    // The size is in units; the letters are made at the size in REAL pixels,
    // so on a dense screen they are drawn sharp instead of being a small
    // picture stretched. What is measured in pixels here is handed back to
    // the program divided by the scale again (line height, widths).
    font->made_for = g_scale;
    float pixels = (float) size * g_scale;
    font->scale = stbtt_ScaleForMappingEmToPixels(&font->info, pixels);
    int ascent = 0, descent = 0, gap = 0;
    stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &gap);
    font->ascent = ascent * font->scale;
    font->line_height = (ascent - descent + gap) * font->scale;

    if(!build_atlas(font, pixels)) {
        delete font;
        env->raise("std#illegal_state_exception", "the letters of the font could not be prepared: " + path);
        return 0;
    }

    g_fonts.push_back(font);
    int64_t id = (int64_t) g_fonts.size();
    g_font_ids[key] = id;
    return id;
}

// font_line_height(font): pixels from one line of text to the next.
double sharpgui_font_line_height(sni_env* env, int64_t id) {
    gui_font* font = font_of(env, id);
    return font == nullptr ? 0 : font->line_height / font->made_for;
}

//
// Take the next character out of UTF-8 text (Sharp strings are UTF-8) and move
// `at` past it. Characters we have no letter for come back as '?'.
//
static int next_letter(const std::string& text, size_t& at) {
    unsigned char first = (unsigned char) text[at];
    int code = first, more = 0;
    if(first >= 0xF0)      { code = first & 0x07; more = 3; }
    else if(first >= 0xE0) { code = first & 0x0F; more = 2; }
    else if(first >= 0xC0) { code = first & 0x1F; more = 1; }
    at++;
    while(more > 0 && at < text.size() && ((unsigned char) text[at] & 0xC0) == 0x80) {
        code = (code << 6) | ((unsigned char) text[at] & 0x3F);
        at++;
        more--;
    }
    if(code < FIRST_LETTER || code >= FIRST_LETTER + LETTER_COUNT) return '?';
    return code;
}

//
// Walk a line of text letter by letter. With `paint` it copies each letter out
// of the atlas onto the window; either way it returns how wide the text is.
// (One function for both, so measuring and drawing can never disagree.)
//
static float walk_text(gui_font* font, const std::string& text, float x, float y, bool paint) {
    // (x, y) come in units; the letters are laid out in real pixels (that is
    // what the font was made in) and each one is drawn at its place divided by
    // the scale - the renderer multiplies it back, landing on exact pixels
    float real = font->made_for;
    float start_x = x * real;
    float pen_x = start_x;
    float baseline = std::floor(y * real + font->ascent + 0.5f);       // letters sit on a whole pixel row: sharper
    float pen_y = baseline;
    int previous = 0;

    size_t at = 0;
    while(at < text.size()) {
        int letter = next_letter(text, at);

        // some pairs of letters sit closer together ("AV", "To"): the font says by how much
        if(previous != 0)
            pen_x += stbtt_GetCodepointKernAdvance(&font->info, previous, letter) * font->scale;
        previous = letter;

        stbtt_aligned_quad quad;
        stbtt_GetPackedQuad(font->letters, font->atlas_size, font->atlas_size, letter - FIRST_LETTER,
                            &pen_x, &pen_y, &quad, 0);
        if(paint) {
            SDL_Rect from = {
                (int) (quad.s0 * font->atlas_size), (int) (quad.t0 * font->atlas_size),
                (int) ((quad.s1 - quad.s0) * font->atlas_size), (int) ((quad.t1 - quad.t0) * font->atlas_size)
            };
            SDL_FRect to = { quad.x0 / real, quad.y0 / real, (quad.x1 - quad.x0) / real, (quad.y1 - quad.y0) / real };
            SDL_RenderCopyF(g_renderer, font->atlas, &from, &to);
        }
    }
    return (pen_x - start_x) / real;
}

// text_width(font, text): how many pixels wide this text is in this font.
double sharpgui_text_width(sni_env* env, int64_t id, sni_ref text) {
    gui_font* font = font_of(env, id);
    if(font == nullptr) return 0;
    return walk_text(font, env->text(text), 0, 0, false);
}

//
// draw_text(font, text, x, y, colour): one line of text; (x, y) is its TOP-LEFT
// corner (not the baseline), the same as every other shape.
//
void sharpgui_draw_text(sni_env* env, int64_t id, sni_ref text, double x, double y, int64_t color) {
    if(!gui_ready(env)) return;
    gui_font* font = font_of(env, id);
    if(font == nullptr) return;

    SDL_Color c = gui_color(color);
    SDL_SetTextureColorMod(font->atlas, c.r, c.g, c.b);
    SDL_SetTextureAlphaMod(font->atlas, c.a);
    walk_text(font, env->text(text), (float) x, (float) y, true);
}
