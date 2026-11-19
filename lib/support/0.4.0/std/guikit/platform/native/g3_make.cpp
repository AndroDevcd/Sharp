//
// TEXTURES MADE HERE  -  so that land and plants look like something with no
// picture files at all.
//
//      g3_texture_made("bark", 1)      the id of a texture, drawn by the code below
//
//      bark        the furrowed trunk of a tree: repeats in both directions
//      leaves      a spray of broad leaves on see-through: for the leaf cards of a tree
//      needles     a twig of a conifer with its needles, on see-through
//      grass       rough green ground: repeats
//      rock        grey stone with darker cracks: repeats
//      sand        pale, fine-grained ground: repeats
//
// The number is a seed: another number is another texture of the same kind.
// The same kind and seed is made once and handed out again after that.
//
// They are built from two things: smooth NOISE (a random value at every point
// of a grid, blended in between, several grids of different fineness added up)
// for anything rough - and, for leaves and needles, shapes painted one over the
// other with soft edges.
//
#include "g3_internal.h"
#include <cmath>
#include <algorithm>

namespace {

const float PI = 3.14159265358979f;

// ---- numbers that look random and are the same every time ------------------------------------

uint32_t mixed(uint32_t n) {
    n ^= n >> 16; n *= 0x7feb352dU;
    n ^= n >> 15; n *= 0x846ca68bU;
    n ^= n >> 16;
    return n;
}

struct dice {                           // a stream of numbers 0 .. 1 from a seed
    uint32_t at;
    explicit dice(uint32_t seed) : at(seed * 2654435761U + 12345U) {}
    float next() { at = mixed(at + 0x9e3779b9U); return (at & 0xffffff) / 16777216.0f; }
    float between(float low, float high) { return low + (high - low) * next(); }
};

// a value 0 .. 1 at the grid point (x, y), the grid wrapping round every `period` points
float at_grid(int x, int y, int period, uint32_t seed) {
    x = ((x % period) + period) % period;
    y = ((y % period) + period) % period;
    return (mixed((uint32_t) x * 73856093U ^ (uint32_t) y * 19349663U ^ seed * 83492791U) & 0xffff) / 65535.0f;
}

// smooth noise 0 .. 1 at (x, y), repeating every `period`
float noise(float x, float y, int period, uint32_t seed) {
    int ix = (int) std::floor(x), iy = (int) std::floor(y);
    float fx = x - ix, fy = y - iy;
    float ux = fx * fx * (3 - 2 * fx), uy = fy * fy * (3 - 2 * fy);
    float a = at_grid(ix, iy, period, seed), b = at_grid(ix + 1, iy, period, seed);
    float c = at_grid(ix, iy + 1, period, seed), d = at_grid(ix + 1, iy + 1, period, seed);
    return a + (b - a) * ux + (c - a) * uy + (a - b - c + d) * ux * uy;
}

// several noises added up, each twice as fine and half as strong: 0 .. 1. (u, v) 0 .. 1 across the
// texture; `across` and `down` are how many grid cells the coarsest noise has each way.
float rough(float u, float v, int across, int down, int layers, uint32_t seed) {
    float total = 0, weight = 1, sum = 0;
    for(int i = 0; i < layers; i++) {
        total += weight * noise(u * across, v * down, std::max(across, down) * 64, seed + (uint32_t) i * 101U);
        sum += weight;
        weight *= 0.5f;
        across *= 2;
        down *= 2;
    }
    return total / sum;
}

// the same, repeating exactly at the texture's edges
float rough_repeating(float u, float v, int across, int down, int layers, uint32_t seed) {
    float total = 0, weight = 1, sum = 0;
    for(int i = 0; i < layers; i++) {
        // wrap each way separately: sample a grid whose period is the cell count that way
        int ix = (int) std::floor(u * across), iy = (int) std::floor(v * down);
        float fx = u * across - ix, fy = v * down - iy;
        float ux = fx * fx * (3 - 2 * fx), uy = fy * fy * (3 - 2 * fy);
        auto grid = [&](int x, int y) {
            x = ((x % across) + across) % across;
            y = ((y % down) + down) % down;
            return (mixed((uint32_t) x * 73856093U ^ (uint32_t) y * 19349663U ^ (seed + (uint32_t) i * 101U) * 83492791U) & 0xffff) / 65535.0f;
        };
        float a = grid(ix, iy), b = grid(ix + 1, iy), c = grid(ix, iy + 1), d = grid(ix + 1, iy + 1);
        total += weight * (a + (b - a) * ux + (c - a) * uy + (a - b - c + d) * ux * uy);
        sum += weight;
        weight *= 0.5f;
        across *= 2;
        down *= 2;
    }
    return total / sum;
}

// ---- a picture being painted ------------------------------------------------------------------

struct picture {
    int w, h;
    std::vector<float> rgba;            // 0 .. 1; alpha 0: nothing there yet

    picture(int width, int height) : w(width), h(height), rgba((size_t) width * height * 4, 0.0f) {}

    // paint one pixel over what is there, `solid` 0 .. 1 of it
    void over(int x, int y, float r, float g, float b, float solid) {
        if(x < 0 || y < 0 || x >= w || y >= h || solid <= 0) return;
        float* p = &rgba[((size_t) y * w + x) * 4];
        float under = p[3] * (1 - solid);
        float all = solid + under;
        if(all <= 0) return;
        p[0] = (r * solid + p[0] * under) / all;
        p[1] = (g * solid + p[1] * under) / all;
        p[2] = (b * solid + p[2] * under) / all;
        p[3] = all;
    }

    // a line from (x1, y1) to (x2, y2), `wide` pixels wide, tapering to `tip` of that at its end
    void stroke(float x1, float y1, float x2, float y2, float wide, float tip, float r, float g, float b) {
        float dx = x2 - x1, dy = y2 - y1, length = std::sqrt(dx * dx + dy * dy);
        if(length < 0.001f) return;
        int left = (int) std::floor(std::min(x1, x2) - wide - 1), right = (int) std::ceil(std::max(x1, x2) + wide + 1);
        int top = (int) std::floor(std::min(y1, y2) - wide - 1), bottom = (int) std::ceil(std::max(y1, y2) + wide + 1);
        for(int y = top; y <= bottom; y++) {
            for(int x = left; x <= right; x++) {
                float along = ((x + 0.5f - x1) * dx + (y + 0.5f - y1) * dy) / (length * length);
                float t = std::min(1.0f, std::max(0.0f, along));
                float px = x1 + dx * t, py = y1 + dy * t;
                float away = std::sqrt((x + 0.5f - px) * (x + 0.5f - px) + (y + 0.5f - py) * (y + 0.5f - py));
                float half = wide * (1 - (1 - tip) * t) / 2;
                over(x, y, r, g, b, std::min(1.0f, std::max(0.0f, half - away + 0.5f)));
            }
        }
    }

    // as bytes, for the graphics card. Pixels with nothing in them take the colour of what is
    // near them (still invisible): without that the smoothed edges of leaves show a dark rim.
    std::vector<unsigned char> bytes() {
        std::vector<float> spread = rgba;
        for(int pass = 0; pass < 8; pass++) {
            std::vector<float> from = spread;
            for(int y = 0; y < h; y++) {
                for(int x = 0; x < w; x++) {
                    float* p = &spread[((size_t) y * w + x) * 4];
                    if(rgba[((size_t) y * w + x) * 4 + 3] > 0 || p[3] > 0) continue;
                    float r = 0, g = 0, b = 0;
                    int found = 0;
                    const int nx[4] = { 1, -1, 0, 0 }, ny[4] = { 0, 0, 1, -1 };
                    for(int k = 0; k < 4; k++) {
                        int ox = x + nx[k], oy = y + ny[k];
                        if(ox < 0 || oy < 0 || ox >= w || oy >= h) continue;
                        const float* q = &from[((size_t) oy * w + ox) * 4];
                        if(q[3] > 0) { r += q[0]; g += q[1]; b += q[2]; found++; }
                    }
                    if(found > 0) { p[0] = r / found; p[1] = g / found; p[2] = b / found; p[3] = 0.0001f; }
                }
            }
        }
        std::vector<unsigned char> out((size_t) w * h * 4);
        for(size_t i = 0; i < (size_t) w * h; i++) {
            for(int k = 0; k < 3; k++) out[i * 4 + k] = (unsigned char) (std::min(1.0f, std::max(0.0f, spread[i * 4 + k])) * 255 + 0.5f);
            out[i * 4 + 3] = (unsigned char) (std::min(1.0f, std::max(0.0f, rgba[i * 4 + 3])) * 255 + 0.5f);
        }
        return out;
    }
};

float mix(float a, float b, float k) { return a + (b - a) * k; }

// ---- the kinds --------------------------------------------------------------------------------

// every pixel solid, its colour from a function of where it is (u, v: 0 .. 1)
template<typename shade> void fill(picture& p, shade color) {
    for(int y = 0; y < p.h; y++) {
        for(int x = 0; x < p.w; x++) {
            float rgb[3];
            color((x + 0.5f) / p.w, (y + 0.5f) / p.h, rgb);
            float* at = &p.rgba[((size_t) y * p.w + x) * 4];
            at[0] = rgb[0]; at[1] = rgb[1]; at[2] = rgb[2]; at[3] = 1;
        }
    }
}

void bark(picture& p, uint32_t seed) {
    fill(p, [&](float u, float v, float rgb[3]) {
        // long furrows running up the trunk: noise that is fine across and coarse along, with its
        // middle values pulled apart into ridges and grooves; finer grain over it
        float wander = rough_repeating(u, v, 4, 6, 3, seed + 7) * 0.9f;
        float furrow = rough_repeating(u + wander * 0.18f, v, 14, 3, 4, seed);
        float groove = std::fabs(furrow * 2 - 1);                       // 0 in the middle of a groove
        groove = std::pow(std::min(1.0f, groove * 1.9f), 0.7f);
        float grain = rough_repeating(u, v, 48, 24, 3, seed + 31);
        float light = 0.22f + 0.6f * groove + 0.22f * (grain - 0.5f);
        float warm = rough_repeating(u, v, 5, 5, 2, seed + 53);          // patches a little greyer or browner
        rgb[0] = light * mix(0.50f, 0.62f, warm);
        rgb[1] = light * mix(0.40f, 0.47f, warm);
        rgb[2] = light * mix(0.30f, 0.36f, warm);
    });
}

void ground_grass(picture& p, uint32_t seed) {
    fill(p, [&](float u, float v, float rgb[3]) {
        float patches = rough_repeating(u, v, 5, 5, 4, seed);
        float blades = rough_repeating(u, v, 64, 64, 2, seed + 11);
        float dry = std::max(0.0f, rough_repeating(u, v, 9, 9, 3, seed + 23) - 0.55f) * 2.2f;      // a few dry spots
        float light = 0.62f + 0.38f * blades;
        rgb[0] = light * mix(mix(0.20f, 0.33f, patches), 0.52f, dry);
        rgb[1] = light * mix(mix(0.38f, 0.52f, patches), 0.50f, dry);
        rgb[2] = light * mix(mix(0.13f, 0.20f, patches), 0.28f, dry);
    });
}

void ground_rock(picture& p, uint32_t seed) {
    fill(p, [&](float u, float v, float rgb[3]) {
        float body = rough_repeating(u, v, 6, 6, 5, seed);
        float crack = std::fabs(rough_repeating(u, v, 9, 9, 4, seed + 17) * 2 - 1);          // 0 along a crack
        float dark = 1 - std::max(0.0f, 1 - crack * 9) * 0.65f;
        float grain = rough_repeating(u, v, 96, 96, 2, seed + 29);
        float light = (0.34f + 0.42f * body + 0.14f * (grain - 0.5f)) * dark;
        rgb[0] = light * 1.02f;
        rgb[1] = light;
        rgb[2] = light * 0.96f;
    });
}

void ground_sand(picture& p, uint32_t seed) {
    fill(p, [&](float u, float v, float rgb[3]) {
        float waves = rough_repeating(u, v, 4, 10, 3, seed);
        float grain = rough_repeating(u, v, 128, 128, 2, seed + 13);
        float light = 0.74f + 0.14f * waves + 0.16f * (grain - 0.5f);
        rgb[0] = light * 0.96f;
        rgb[1] = light * 0.86f;
        rgb[2] = light * 0.66f;
    });
}

// one leaf: from its stem end (x, y), `length` pixels long towards `angle`, `fat` its width over its length
void leaf(picture& p, float x, float y, float angle, float length, float fat, const float green[3], uint32_t seed) {
    float ax = std::cos(angle), ay = std::sin(angle);
    int reach = (int) std::ceil(length) + 2;
    int cx = (int) (x + ax * length / 2), cy = (int) (y + ay * length / 2);
    float curl = ((seed % 100) / 100.0f - 0.5f) * 0.5f;                 // the leaf bends a little to one side
    for(int py = cy - reach; py <= cy + reach; py++) {
        for(int px = cx - reach; px <= cx + reach; px++) {
            float dx = px + 0.5f - x, dy = py + 0.5f - y;
            float along = (dx * ax + dy * ay) / length;                 // 0 at the stem .. 1 at the tip
            if(along < -0.02f || along > 1.02f) continue;
            float across = (dy * ax - dx * ay) / length - curl * along * (1 - along);
            // the outline: widest a third of the way along, a point at the tip
            float t = std::min(1.0f, std::max(0.0f, along));
            float half = fat * 0.5f * std::pow(std::sin(PI * std::pow(t, 0.75f)), 0.9f);
            float inside = (half - std::fabs(across)) * length;        // pixels inside the edge
            if(inside < -1) continue;
            float solid = std::min(1.0f, std::max(0.0f, inside + 0.5f));
            // lighter along the middle rib and the side veins, darker towards the stem
            float rib = std::max(0.0f, 1 - std::fabs(across) * length / 1.6f);
            float vein = std::max(0.0f, 1 - std::fabs(std::sin((t * 9 - std::fabs(across) / std::max(0.001f, half) * 1.6f) * PI)) * 4);
            float light = 0.72f + 0.34f * t + 0.22f * rib + 0.10f * vein * (1 - rib);
            float edge = 1 - 0.25f * std::min(1.0f, std::fabs(across) / std::max(0.001f, half));
            p.over(px, py, green[0] * light * edge, green[1] * light * edge, green[2] * light * edge, solid);
        }
    }
}

void leaves(picture& p, uint32_t seed) {
    dice roll(seed + 3);
    float mid_x = p.w / 2.0f, mid_y = p.h / 2.0f, size = p.w / 512.0f;

    // twigs fanning out from the middle, and leaves along and at the end of each: the ones painted
    // first lie underneath, and are darker - the inside of the spray
    struct twig { float x, y, angle, length; };
    std::vector<twig> twigs;
    int count = 19;
    for(int i = 0; i < count; i++) {
        twig t;
        t.angle = 2 * PI * i / count + roll.between(-0.2f, 0.2f);
        t.length = roll.between(95, 215) * size;
        t.x = mid_x + std::cos(t.angle) * 8 * size;
        t.y = mid_y + std::sin(t.angle) * 8 * size;
        twigs.push_back(t);
    }
    for(const twig& t : twigs)
        p.stroke(t.x, t.y, t.x + std::cos(t.angle) * t.length, t.y + std::sin(t.angle) * t.length, 3.5f * size, 0.3f, 0.30f, 0.22f, 0.13f);

    for(int layer = 0; layer < 3; layer++) {
        float depth = 0.74f + 0.15f * layer;                            // the lowest layer is the darkest
        for(const twig& t : twigs) {
            int along_twig = 9;
            for(int k = 0; k < along_twig; k++) {
                float where = (k + roll.between(0.2f, 0.9f)) / along_twig;
                if(where < 0.25f && layer == 2) continue;
                float bx = t.x + std::cos(t.angle) * t.length * where, by = t.y + std::sin(t.angle) * t.length * where;
                float side = (k + layer) % 2 == 0 ? 1.0f : -1.0f;
                float angle = t.angle + side * roll.between(0.45f, 1.05f);
                if(k == along_twig - 1) angle = t.angle + roll.between(-0.3f, 0.3f);      // the last one at the tip
                float length = roll.between(30, 48) * size * (0.8f + 0.2f * where);
                float hue = roll.next();
                float green[3] = { depth * mix(0.27f, 0.47f, hue), depth * mix(0.56f, 0.70f, hue), depth * mix(0.16f, 0.24f, hue) };
                leaf(p, bx, by, angle, length, roll.between(0.40f, 0.54f), green, mixed(seed + (uint32_t) (k * 31 + layer * 7)) + (uint32_t) (hue * 1000));
            }
        }
    }
}

void needles(picture& p, uint32_t seed) {
    dice roll(seed + 5);
    float size = p.w / 512.0f;
    // one twig up the middle of the picture (its base at the bottom edge) with shorter ones off it,
    // every one bristling with needles
    struct twig { float x1, y1, x2, y2; };
    std::vector<twig> twigs;
    twigs.push_back({ p.w / 2.0f, p.h - 4.0f, p.w / 2.0f + roll.between(-12, 12) * size, 22 * size });
    int pairs = 7;
    for(int i = 0; i < pairs; i++) {
        float where = 0.12f + 0.74f * i / (pairs - 1);
        float bx = mix(twigs[0].x1, twigs[0].x2, where), by = mix(twigs[0].y1, twigs[0].y2, where);
        float reach = (1 - where * 0.72f) * 205 * size;
        for(int side = -1; side <= 1; side += 2) {
            float angle = -PI / 2 + side * roll.between(0.85f, 1.2f);
            twigs.push_back({ bx, by, bx + std::cos(angle) * reach, by + std::sin(angle) * reach * 0.9f });
        }
    }
    for(int layer = 0; layer < 2; layer++) {
        float depth = 0.66f + 0.3f * layer;
        for(const twig& t : twigs) {
            float dx = t.x2 - t.x1, dy = t.y2 - t.y1, length = std::sqrt(dx * dx + dy * dy);
            float along = std::atan2(dy, dx);
            int count = (int) (length / (3.2f * size));
            for(int i = 0; i < count; i++) {
                float where = (i + roll.next()) / count;
                float bx = t.x1 + dx * where, by = t.y1 + dy * where;
                float side = i % 2 == 0 ? 1.0f : -1.0f;
                float angle = along + side * roll.between(0.5f, 1.0f);
                float reach = roll.between(20, 32) * size * (1 - 0.35f * where);
                float hue = roll.next();
                p.stroke(bx, by, bx + std::cos(angle) * reach, by + std::sin(angle) * reach, 2.6f * size, 0.35f,
                         depth * mix(0.14f, 0.26f, hue), depth * mix(0.40f, 0.54f, hue), depth * mix(0.20f, 0.28f, hue));
            }
        }
        if(layer == 0)
            for(const twig& t : twigs) p.stroke(t.x1, t.y1, t.x2, t.y2, 5 * size, 0.3f, 0.30f, 0.21f, 0.13f);
    }
}

} // namespace

// g3_texture_made(kind, seed): one of the textures drawn by this file (the top of it lists them). Its id.
int64_t sharpgui_g3_texture_made(sni_env* env, sni_ref kind_ref, int64_t seed) {
    std::string kind = env->text(kind_ref);
    std::string key = "made:" + kind + ":" + std::to_string(seed);
    if(!g3_begin(env)) return 0;
    int64_t id = g3_texture_known(key);
    if(id == 0) {
        bool see_through = kind == "leaves" || kind == "needles";
        picture p(see_through ? 512 : (kind == "bark" ? 256 : 512), 512);
        if(kind == "bark") bark(p, (uint32_t) seed);
        else if(kind == "leaves") leaves(p, (uint32_t) seed);
        else if(kind == "needles") needles(p, (uint32_t) seed);
        else if(kind == "grass") ground_grass(p, (uint32_t) seed);
        else if(kind == "rock") ground_rock(p, (uint32_t) seed);
        else if(kind == "sand") ground_sand(p, (uint32_t) seed);
        else {
            env->raise("std#illegal_argument_exception", "there is no made texture called `" + kind
                       + "`: the kinds are bark, leaves, needles, grass, rock and sand");
            g3_end();
            return 0;
        }
        std::vector<unsigned char> bytes = p.bytes();
        id = g3_texture_pixels(key, bytes.data(), p.w, p.h);
    }
    g3_end();
    return id;
}
