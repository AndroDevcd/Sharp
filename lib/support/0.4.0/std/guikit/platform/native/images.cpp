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

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

struct gui_image {
    SDL_Texture* texture = nullptr;
    int width = 0, height = 0;
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
