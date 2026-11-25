//
// 3D SCENES  -  drawn by the graphics card, with OpenGL.
//
// The 2D side of this library paints through SDL's renderer, which only knows
// flat pictures. A 3D scene needs what the graphics card is really built for:
// a depth buffer (the card decides, for every pixel, what is in front),
// textures that stay sharp at any distance, and light worked out for every
// pixel. So scenes are drawn by a small OpenGL program of our own.
//
// HOW IT LIVES BESIDE THE 2D SIDE
//
// The 3D side has its OWN OpenGL context, in a window that is never shown. A
// scene is drawn into a picture off screen there, the picture is read back,
// and it is handed to the 2D side as an ordinary texture, which paints it
// where the scene widget is. So:
//
//      - it works whatever the 2D side paints with (Direct3D on Windows too);
//      - a scene is one more picture to the 2D side: widgets over it, clipping,
//        fading all work, and nothing the 3D side does to OpenGL's state can
//        disturb the 2D side - it is another context.
//
// The price is reading the picture back every frame: a few megabytes. A scene
// that is drawn frame after frame reads it without making the card wait
// (read_picture). g3_begin / g3_end switch contexts around everything the 3D
// side does.
//
// ONE PROGRAM FOR TWO OPENGLS: the shaders are written in the part of the
// shading language that desktop OpenGL 2.1 and OpenGL ES 2 (a Raspberry Pi)
// both understand; only their first line differs.
//
// THE SPACE: x to the right, y UP, z towards you (so "forward" is -z), one
// unit a metre. A heading is degrees round the up axis, 0 looking along -z and
// 90 along +x, like a compass seen from above.
//
#include "g3_internal.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <map>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

static const float G3_PI = 3.14159265358979f;
static float radians(double degrees) { return (float) (degrees * 3.14159265358979323846 / 180.0); }

// =================================================================================
// 1. Matrices: 4 x 4, column after column, as OpenGL takes them
// =================================================================================

struct mat4 { float m[16]; };

static mat4 identity() {
    mat4 r;
    memset(r.m, 0, sizeof(r.m));
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1;
    return r;
}

// a * b: what b does first, then a
static mat4 times(const mat4& a, const mat4& b) {
    mat4 r;
    for(int col = 0; col < 4; col++)
        for(int row = 0; row < 4; row++)
            r.m[col * 4 + row] = a.m[row] * b.m[col * 4] + a.m[4 + row] * b.m[col * 4 + 1]
                               + a.m[8 + row] * b.m[col * 4 + 2] + a.m[12 + row] * b.m[col * 4 + 3];
    return r;
}

static void point_through(const mat4& m, const float in[3], float out[3]) {
    for(int row = 0; row < 3; row++)
        out[row] = m.m[row] * in[0] + m.m[4 + row] * in[1] + m.m[8 + row] * in[2] + m.m[12 + row];
}

// where a model is: moved to (x, y, z), turned (degrees: heading, then nose up, then rolled), scaled
static mat4 placed(float x, float y, float z, float yaw, float pitch, float roll, float sx, float sy, float sz) {
    float cy = std::cos(-yaw), sy_ = std::sin(-yaw);       // a heading turns the other way round than a rotation about y
    float cp = std::cos(pitch), sp = std::sin(pitch);
    float cr = std::cos(-roll), sr = std::sin(-roll);
    // R = Ry(heading) * Rx(pitch) * Rz(roll)
    float r00 = cy * cr + sy_ * sp * sr, r01 = -cy * sr + sy_ * sp * cr, r02 = sy_ * cp;
    float r10 = cp * sr,                 r11 = cp * cr,                  r12 = -sp;
    float r20 = -sy_ * cr + cy * sp * sr, r21 = sy_ * sr + cy * sp * cr, r22 = cy * cp;
    mat4 r = identity();
    r.m[0] = r00 * sx; r.m[1] = r10 * sx; r.m[2] = r20 * sx;
    r.m[4] = r01 * sy; r.m[5] = r11 * sy; r.m[6] = r21 * sy;
    r.m[8] = r02 * sz; r.m[9] = r12 * sz; r.m[10] = r22 * sz;
    r.m[12] = x; r.m[13] = y; r.m[14] = z;
    return r;
}

static void cross(const float a[3], const float b[3], float out[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

static void unit(float v[3]) {
    float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if(length > 0) { v[0] /= length; v[1] /= length; v[2] /= length; }
}

// the world as an eye at `eye` sees it, looking along `forward`
static mat4 looking(const float eye[3], const float forward[3], const float up_hint[3]) {
    float f[3] = { forward[0], forward[1], forward[2] }, s[3], u[3];
    unit(f);
    cross(f, up_hint, s);
    unit(s);
    cross(s, f, u);
    mat4 r = identity();
    r.m[0] = s[0]; r.m[4] = s[1]; r.m[8] = s[2];
    r.m[1] = u[0]; r.m[5] = u[1]; r.m[9] = u[2];
    r.m[2] = -f[0]; r.m[6] = -f[1]; r.m[10] = -f[2];
    r.m[12] = -(s[0] * eye[0] + s[1] * eye[1] + s[2] * eye[2]);
    r.m[13] = -(u[0] * eye[0] + u[1] * eye[1] + u[2] * eye[2]);
    r.m[14] = f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2];
    return r;
}

static mat4 perspective(float fov_y, float aspect, float near_z, float far_z) {
    float f = 1 / std::tan(fov_y / 2);
    mat4 r;
    memset(r.m, 0, sizeof(r.m));
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far_z + near_z) / (near_z - far_z);
    r.m[11] = -1;
    r.m[14] = 2 * far_z * near_z / (near_z - far_z);
    return r;
}

static mat4 flat_view(float half, float near_z, float far_z) {     // no perspective: a box seen side on
    mat4 r = identity();
    r.m[0] = 1 / half;
    r.m[5] = 1 / half;
    r.m[10] = -2 / (far_z - near_z);
    r.m[14] = -(far_z + near_z) / (far_z - near_z);
    return r;
}

// the opposite of a matrix that only moves, turns and scales (no perspective)
static bool undone(const mat4& in, mat4& out) {
    const float* m = in.m;
    float a = m[0], b = m[4], c = m[8], d = m[1], e = m[5], f = m[9], g = m[2], h = m[6], i = m[10];
    float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if(std::fabs(det) < 1e-12f) return false;
    float inv = 1 / det;
    out = identity();
    out.m[0] = (e * i - f * h) * inv; out.m[4] = (c * h - b * i) * inv; out.m[8] = (b * f - c * e) * inv;
    out.m[1] = (f * g - d * i) * inv; out.m[5] = (a * i - c * g) * inv; out.m[9] = (c * d - a * f) * inv;
    out.m[2] = (d * h - e * g) * inv; out.m[6] = (b * g - a * h) * inv; out.m[10] = (a * e - b * d) * inv;
    out.m[12] = -(out.m[0] * m[12] + out.m[4] * m[13] + out.m[8] * m[14]);
    out.m[13] = -(out.m[1] * m[12] + out.m[5] * m[13] + out.m[9] * m[14]);
    out.m[14] = -(out.m[2] * m[12] + out.m[6] * m[13] + out.m[10] * m[14]);
    return true;
}

static void color_of(double packed, float out[4]) {
    int64_t c = (int64_t) packed;
    out[0] = ((c >> 24) & 255) / 255.0f;
    out[1] = ((c >> 16) & 255) / 255.0f;
    out[2] = ((c >> 8) & 255) / 255.0f;
    out[3] = (c & 255) / 255.0f;
}

// =================================================================================
// 2. The context
// =================================================================================

static SDL_Window* g3_window = nullptr;         // never shown: it only exists to own the context
static SDL_GLContext g3_context = nullptr;
static bool g3_es = false;                      // OpenGL ES (a small machine) or desktop OpenGL
static bool g3_failed = false;
static std::string g3_why;                      // why it failed, for the exception
static std::string g3_card;                     // what draws: "NVIDIA ... / OpenGL 4.6 ..."
static bool g3_wide_indices = true;             // can a part have more than 65,535 corners?
static bool g3_depth_pictures = true;           // can depth be drawn into a texture? (shadows need it)
static int g3_depth = 0;                        // nested g3_begin calls
static SDL_Window* g3_before_window = nullptr;
static SDL_GLContext g3_before_context = nullptr;

static bool has_extension(const char* name) {
    const char* all = (const char*) glGetString(GL_EXTENSIONS);
    return all != nullptr && strstr(all, name) != nullptr;
}

static void build_programs();

static bool start_3d() {
    if(g3_context != nullptr) return true;
    if(g3_failed) return false;

    // an OpenGL ES machine says so through what the 2D side is painted with
    SDL_RendererInfo painted_with;
    bool small = g_renderer != nullptr && SDL_GetRendererInfo(g_renderer, &painted_with) == 0
                 && painted_with.name != nullptr && strncmp(painted_with.name, "opengles", 8) == 0;

    for(int attempt = 0; attempt < 2 && g3_context == nullptr; attempt++) {
        bool es = (attempt == 0) == small;      // first what the machine looks like, then the other kind
        SDL_GL_ResetAttributes();
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        if(es) {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        }
        g3_window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 16, 16,
                                     SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if(g3_window == nullptr) { g3_why = SDL_GetError(); continue; }
        g3_context = SDL_GL_CreateContext(g3_window);
        if(g3_context == nullptr) {
            g3_why = SDL_GetError();
            SDL_DestroyWindow(g3_window);
            g3_window = nullptr;
            continue;
        }
        g3_es = es;
    }
    SDL_GL_ResetAttributes();
    if(g3_context == nullptr) {
        g3_failed = true;
        g3_why = "this machine gave no OpenGL context (" + g3_why + ")";
        return false;
    }

    SDL_GL_MakeCurrent(g3_window, g3_context);
    const char* missing = g3_load_gl();
    if(missing != nullptr) {
        g3_failed = true;
        g3_why = std::string("this machine's OpenGL is too old: it has no ") + missing;
        return false;
    }
    const char* version = (const char*) glGetString(GL_VERSION);
    const char* card = (const char*) glGetString(GL_RENDERER);
    g3_card = std::string(card != nullptr ? card : "?") + " / " + (version != nullptr ? version : "?");
    if(version != nullptr && strstr(version, "OpenGL ES") != nullptr) g3_es = true;
    if(g3_es) {
        g3_wide_indices = has_extension("OES_element_index_uint") || (version != nullptr && strstr(version, "ES 3") != nullptr);
        g3_depth_pictures = has_extension("OES_depth_texture") || (version != nullptr && strstr(version, "ES 3") != nullptr);
    }
    build_programs();
    return !g3_failed;
}

bool g3_begin(sni_env* env) {
    if(!gui_ready(env)) return false;
    if(g3_depth == 0) {
        g3_before_window = SDL_GL_GetCurrentWindow();
        g3_before_context = SDL_GL_GetCurrentContext();
        if(!start_3d()) {
            env->raise("std#illegal_state_exception", "3D is not available here: " + g3_why);
            return false;
        }
        SDL_GL_MakeCurrent(g3_window, g3_context);
    }
    g3_depth++;
    return true;
}

void g3_end() {
    if(g3_depth > 0 && --g3_depth == 0 && g3_before_context != nullptr)
        SDL_GL_MakeCurrent(g3_before_window, g3_before_context);    // the 2D side's own, when it paints with OpenGL
}

// =================================================================================
// 3. The shaders
// =================================================================================
//
// Two programs: one for everything that has a surface, one for the sky.

static const char* SURFACE_CORNER = R"(
attribute vec3 a_pos;
attribute vec3 a_normal;
attribute vec2 a_uv;
attribute vec4 a_color;
attribute mat4 a_place;     // corner -> the world: where the thing stands. One for the whole thing, or -
                            // many copies drawn at once - one for each copy (draw_models)
uniform mat4 u_view_lens;   // the world -> the picture
uniform mat4 u_sun_view;    // the world -> the sun's view (for shadows)
uniform float u_repeat;     // how many times the texture is laid across the model
uniform float u_sway;       // how far the thing bends in a current (0: it stands still)
uniform float u_clock;      // seconds since the window opened
varying vec3 v_normal;
varying vec3 v_world;
varying vec2 v_uv;
varying vec4 v_color;
varying vec4 v_shadow;
void main() {
    vec4 world = a_place * vec4(a_pos, 1.0);
    v_normal = (a_place * vec4(a_normal, 0.0)).xyz;
    v_uv = a_uv * u_repeat;
    v_color = a_color;
    if(u_sway > 0.0) {
        // A current: the foot stays where it is and the rest leans, more the higher it is, in slow
        // waves that pass through the water - so neighbours move together but not alike.
        float lift = max(a_pos.y, 0.0);
        float bend = u_sway * lift * (0.05 + lift * 0.004);
        float t = u_clock;
        vec2 push = vec2(sin(t * 0.61 + world.x * 0.17 + world.z * 0.11) + 0.45 * sin(t * 1.27 + world.y * 0.33 + world.z * 0.4),
                         cos(t * 0.53 + world.z * 0.15 - world.x * 0.09) + 0.45 * sin(t * 1.09 + world.y * 0.29 + world.x * 0.4));
        world.xz += push * bend;
    }
    v_world = world.xyz;
    v_shadow = u_sun_view * world;
    gl_Position = u_view_lens * world;
}
)";

static const char* SURFACE_PIXEL = R"(
uniform sampler2D u_tex;
uniform sampler2D u_shadow_map;
uniform float u_has_tex;
uniform vec4 u_tint;
uniform float u_cutout;         // 1: pixels more than half see-through are left out (leaves)
uniform float u_wrap;           // how far the sun's light reaches round to the side turned away (thin things: leaves)
uniform float u_unlit;          // 1: its own colour, whatever the light
uniform float u_shiny;
uniform float u_glow;
uniform vec3 u_sun_dir;         // towards the sun
uniform vec3 u_sun_color;
uniform vec3 u_sky_light;       // the light from above on a surface facing up ...
uniform vec3 u_ground_light;    // ... and from below on one facing down
uniform vec3 u_eye;
uniform vec3 u_fog_color;
uniform vec2 u_fog;             // from, to (to 0: none)
uniform vec4 u_lamp_at[4];      // xyz, and how far it reaches (0: no lamp)
uniform vec3 u_lamp_color[4];
uniform float u_shadows;        // 1: the shadow map is in use
uniform float u_shadow_step;    // one pixel of the shadow map
uniform float u_depth_only;     // 1: only the shape matters (drawing the shadow map)
uniform float u_time;           // seconds since the window opened
uniform sampler2D u_grain;      // the grain of stone: red and green its slopes, blue its height, alpha its blotches
uniform float u_rough;          // how much of it the thing has (0: smooth)
varying vec3 v_normal;
varying vec3 v_world;
varying vec2 v_uv;
varying vec4 v_color;
varying vec4 v_shadow;

// One look at the shadow map, smoothed between the four of its pixels round the place asked about:
// the answer changes evenly as the place moves, so a shadow's edge is a soft line and never the
// little squares of the map itself.
float one_look(vec2 at, float depth) {
    vec2 across = at / u_shadow_step - 0.5;
    vec2 part = fract(across);
    vec2 corner = (floor(across) + 0.5) * u_shadow_step;
    float a = step(depth, texture2D(u_shadow_map, corner).r);
    float b = step(depth, texture2D(u_shadow_map, corner + vec2(u_shadow_step, 0.0)).r);
    float c = step(depth, texture2D(u_shadow_map, corner + vec2(0.0, u_shadow_step)).r);
    float d = step(depth, texture2D(u_shadow_map, corner + vec2(u_shadow_step)).r);
    return mix(mix(a, b, part.x), mix(c, d, part.x), part.y);
}

float in_light() {
    vec3 at = v_shadow.xyz / v_shadow.w * 0.5 + 0.5;
    if(at.x < 0.0 || at.x > 1.0 || at.y < 0.0 || at.y > 1.0 || at.z > 1.0) return 1.0;
    float depth = at.z - 0.0012;
#ifdef GL_ES
    float lit = one_look(at.xy, depth);                 // (a small machine: one smoothed look)
#else
    // nine smoothed looks, a pixel of the map apart: a soft edge a few pixels wide
    float lit = 0.0;
    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            lit += one_look(at.xy + vec2(float(i), float(j)) * u_shadow_step, depth);
        }
    }
    lit /= 9.0;
#endif
    // towards the edge of what the map covers a shadow fades out, instead of ending at a line
    vec2 from_middle = abs(at.xy - 0.5) * 2.0;
    return mix(lit, 1.0, smoothstep(0.82, 1.0, max(from_middle.x, from_middle.y)));
}
)";

// WATER the eye is inside of: what the surface, the sky and the light shafts share.
//
//      water_color(ray)    the colour of open water looked into along `ray`: bright towards the surface,
//                          the deep colour below, and between them sideways
//      caustic(p, t)       the moving net of bright lines the surface's ripples throw on things below:
//                          three layers of waves, each bent by the one before
static const char* WATER_SHARED = R"(
uniform vec4 u_water;           // on (1), the height of the surface, the strength of the caustics, the haze per metre
uniform vec3 u_water_shallow;
uniform vec3 u_water_deep;
// water takes the red out of light first, then the green: what is far, or deep, turns blue
const vec3 WATER_TAKES = vec3(1.45, 1.0, 0.86);

// the light open water itself sends along `ray` to an eye at `from`: its own colour, lit from above -
// brighter looking up and near the surface, the deep colour looking down, a glow towards the sun
vec3 water_light(vec3 ray, vec3 from) {
    float under = max(u_water.y - from.y, 0.0);
    vec3 color = mix(u_water_shallow, u_water_deep, clamp(0.55 - ray.y * 0.8, 0.0, 1.0));
    color *= exp(-under * 0.006) * (0.85 + 0.6 * clamp(ray.y, 0.0, 1.0));
    float to_sun = max(dot(ray, u_sun_dir), 0.0);
    return color * (1.0 + 0.45 * to_sun * to_sun * to_sun);
}

// the net of bright lines the surface's ripples throw below them, as it looks to a diver: waves that
// bend each other, sharpened to lines. `p` in metres, `t` in seconds.
float caustic(vec2 p, float t) {
    vec2 start = p * 1.9;
    vec2 i = start;
    float sum = 0.0;
    for(int n = 1; n <= 4; n++) {
        float tt = t * (1.0 - 3.5 / float(n));
        i = start + vec2(cos(tt - i.x) + sin(tt + i.y), sin(tt - i.y) + cos(tt + i.x));
        float s = sin(i.x + tt), k = cos(i.y + tt);
        sum += abs(s * k) / (1.25 * sqrt(s * s + k * k) + 0.0001);
    }
    float c = 1.17 - pow(sum / 4.0, 1.4);
    return pow(abs(c), 8.0);
}

// bright things roll off softly instead of going flat white, as on film
vec3 filmed(vec3 c) {
    c *= 0.9;
    return clamp((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14), 0.0, 1.0);
}
)";

// What a program's own shader replaces (g3_shader): the last word on a pixel's colour, after the
// light and before the fog. The one every thing has until it is given another changes nothing.
static const char* SURFACE_LOOK = R"(
vec3 look(vec3 color, vec3 normal, vec3 place, vec2 at, float time) { return color; }
)";

static const char* SURFACE_MAIN = R"(
void main() {
    vec4 base = u_tint * v_color;
    if(u_has_tex > 0.5) base *= texture2D(u_tex, v_uv);
    // (far away a texture is shown from its smaller copies, in which thin leaves fade towards
    //  nothing: the further, the fainter a pixel may be and still count - or far trees go bare)
    float away = length(u_eye - v_world);
    if(u_cutout > 0.5 && base.a < mix(0.4, 0.08, clamp(away / 45.0, 0.0, 1.0))) discard;
    if(u_depth_only > 0.5) { gl_FragColor = vec4(1.0); return; }

    vec3 color = base.rgb;
    vec3 n = normalize(v_normal);
    if(!gl_FrontFacing) n = -n;

    if(u_rough > 0.0) {
        // THE GRAIN OF STONE: a picture of bumps laid on from three sides (so it never stretches,
        // whatever the shape), at two sizes. Its slopes tip the surface this way and that, so the
        // light catches every bump; its heights darken the hollows; its blotches vary the colour.
        vec3 share = pow(abs(n), vec3(4.0));
        share /= share.x + share.y + share.z;
        vec4 fine = texture2D(u_grain, v_world.zy * 0.31) * share.x + texture2D(u_grain, v_world.xz * 0.31) * share.y
                  + texture2D(u_grain, v_world.xy * 0.31) * share.z;
        vec4 wide = texture2D(u_grain, v_world.zy * 0.057 + 0.37) * share.x + texture2D(u_grain, v_world.xz * 0.057 + 0.37) * share.y
                  + texture2D(u_grain, v_world.xy * 0.057 + 0.37) * share.z;
        // (a third layer, turned and at another size, so that the eye does not find where the picture repeats)
        vec3 turned = vec3(v_world.x * 0.8 - v_world.z * 0.6, v_world.y, v_world.x * 0.6 + v_world.z * 0.8) * 0.137 + 0.61;
        vec4 other = texture2D(u_grain, turned.zy) * share.x + texture2D(u_grain, turned.xz) * share.y + texture2D(u_grain, turned.xy) * share.z;
        fine = mix(fine, other, 0.5);
        // (the slopes come squeezed into the picture's range: undone here)
        vec2 fine_in = (fine.rg - 0.5) * 2.0, wide_in = (wide.rg - 0.5) * 2.0;
        vec2 slope = fine_in / (1.0 - min(abs(fine_in), 0.95)) * 1.75 + wide_in / (1.0 - min(abs(wide_in), 0.95)) * 0.9;
        vec3 tipped = vec3(0.0, slope.x, slope.y) * share.x + vec3(slope.x, 0.0, slope.y) * share.y + vec3(slope.x, slope.y, 0.0) * share.z;
        n = normalize(n - tipped * u_rough * 1.5);
        float hollow = mix(1.0, 0.55 + 0.6 * fine.b + 0.3 * wide.b, min(u_rough, 1.0) * 0.6);
        float blotch = mix(1.0, 0.8 + 0.4 * wide.a, min(u_rough, 1.0));
        color *= hollow * blotch;
    }

    float lit = 1.0;            // 1: the sun reaches this pixel (0: something is in its way)
    float under = max(u_water.y - v_world.y, 0.0);      // how far below the water's surface (when there is water)
    if(u_unlit < 0.5) {
        float sun = max((dot(n, u_sun_dir) + u_wrap) / (1.0 + u_wrap), 0.0);
        // (asked of every pixel, turned to the sun or not: a rough face turned away from it has bumps
        //  that face it and bumps that do not, and asking only the first drew a line round each)
        if(u_shadows > 0.5) lit = in_light();
        if(u_wrap > 0.0) lit = 0.5 + 0.5 * lit;         // leaves in the shade of other leaves still glow a little
        vec3 sunlight = u_sun_color;
        vec3 light = mix(u_ground_light, u_sky_light, n.y * 0.5 + 0.5);
        if(u_water.x > 0.5) {
            // the sun comes through water: redder light is lost on the way down ...
            sunlight *= exp(-WATER_TAKES * under * 0.006);
            // ... the water all round glows with its own colour, most from above ...
            float up = n.y * 0.5 + 0.5;
            light = light * 0.5 + mix(u_water_shallow, vec3(1.0), 0.7) * (0.1 + 0.34 * up * up) * exp(-under * 0.007);
            // ... and the surface's ripples focus the sun into moving lines. (They are followed down the
            // sun's own slant, so that they lie on a wall as they do on the floor.)
            if(u_water.z > 0.0) {
                vec2 where = v_world.xz + u_sun_dir.xz * (under / max(u_sun_dir.y, 0.2));
                float net = min(caustic(where / 3.3, u_time * 0.45 + 23.0) * 3.2, 3.0);
                net *= 0.55 + 0.9 * (0.5 + 0.5 * sin(where.x * 0.19 + u_time * 0.21) * sin(where.y * 0.17 - u_time * 0.17));
                net = mix(net, 0.33, smoothstep(35.0, 100.0, away));     // far away the lines blur into an even light
                light += sunlight * net * u_water.z * lit * clamp(dot(n, u_sun_dir) * 0.85 + 0.3, 0.0, 1.0) * exp(-under * 0.01);
            }
        }
        light += sunlight * sun * lit;
        // a thin thing (a leaf, a fin) seen from its shaded side glows with the sun that comes through it
        if(u_wrap > 0.0) light += sunlight * max(dot(-n, u_sun_dir), 0.0) * 0.6 * lit;
        for(int i = 0; i < 4; i++) {
            float reach = u_lamp_at[i].w;
            if(reach > 0.0) {
                vec3 to = u_lamp_at[i].xyz - v_world;
                float far = length(to);
                float fall = clamp(1.0 - far / reach, 0.0, 1.0);
                light += u_lamp_color[i] * max(dot(n, to / max(far, 0.001)), 0.0) * fall * fall;
            }
        }
        color *= light;
        if(u_shiny > 0.0) {
            vec3 seen_from = normalize(u_eye - v_world);
            vec3 half_way = normalize(seen_from + u_sun_dir);
            color += sunlight * pow(max(dot(n, half_way), 0.0), 8.0 + u_shiny * 120.0) * u_shiny * lit;
        }
    }
    color += base.rgb * u_glow;
    color = look(color, n, v_world, v_uv, u_time);
    if(u_water.x > 0.5) {
        // water between the thing and the eye: it takes the thing's light away - the red first - and
        // puts its own in its place
        vec3 kept = exp(-WATER_TAKES * u_water.w * away);
        color = color * kept + water_light((v_world - u_eye) / max(away, 0.001), u_eye) * (1.0 - kept);
        color = filmed(color);
    } else if(u_fog.y > 0.0) {
        color = mix(color, u_fog_color, clamp((away - u_fog.x) / (u_fog.y - u_fog.x), 0.0, 1.0));
    }
    gl_FragColor = vec4(color, base.a);
}
)";

static const char* SKY_CORNER = R"(
attribute vec3 a_pos;       // a corner of the picture
attribute vec3 a_normal;    // the direction the eye looks through that corner
varying vec3 v_ray;
void main() {
    v_ray = a_normal;
    gl_Position = vec4(a_pos.xy, 0.9999, 1.0);
}
)";

static const char* SKY_PIXEL = R"(
uniform vec3 u_top;
uniform vec3 u_horizon;
uniform vec3 u_below;
uniform vec3 u_sun_dir;
uniform vec3 u_sun_color;
uniform vec3 u_eye;
uniform float u_time;
varying vec3 v_ray;
)";

// LIGHT SHAFTS under water: each a strip, soft at its edges and fading towards its lower end. a_uv is
// (across 0 .. 1, along 0 .. 1) and a_color.a how bright the strip is.
static const char* SHAFT_CORNER = R"(
attribute vec3 a_pos;
attribute vec2 a_uv;
attribute vec4 a_color;
uniform mat4 u_view_lens;
varying vec2 v_uv;
varying float v_bright;
void main() {
    v_uv = a_uv;
    v_bright = a_color.a;
    gl_Position = u_view_lens * vec4(a_pos, 1.0);
}
)";

static const char* SHAFT_PIXEL = R"(
uniform vec3 u_shaft_color;
varying vec2 v_uv;
varying float v_bright;
void main() {
    float across = sin(v_uv.x * 3.14159);
    float along = (1.0 - v_uv.y) * smoothstep(0.0, 0.08, v_uv.y);
    gl_FragColor = vec4(u_shaft_color, across * across * along * v_bright);
}
)";

static const char* SKY_MAIN = R"(
void main() {
    vec3 ray = normalize(v_ray);
    if(u_water.x > 0.5) {
        // under water there is no sky: open water all round, and above it the surface seen from below -
        // a rippling skin, bright where the sun comes straight through (the round window every diver
        // sees overhead), darker towards its rim where it only mirrors the deep
        vec3 color = water_light(ray, u_eye);
        if(ray.y > 0.02) {
            float far = (u_water.y - u_eye.y) / ray.y;
            vec2 at = u_eye.xz + ray.xz * far;
            float ripple = min(caustic(at / 9.0, u_time * 0.4 + 7.0), 1.0) + 0.5 * min(caustic(at / 3.7 + 5.0, u_time * 0.55), 1.0);
            float window = smoothstep(0.38, 0.9, ray.y);
            float to_sun = pow(max(dot(ray, u_sun_dir), 0.0), 12.0);
            vec3 skin = u_water_shallow * (0.85 + 0.25 * window)
                      + u_sun_color * (ripple * (0.08 + 0.42 * window) + window * 0.26 + to_sun * 1.3);
            vec3 kept = exp(-WATER_TAKES * u_water.w * far * 0.55);
            color = skin * kept + color * (1.0 - kept);
        }
        gl_FragColor = vec4(filmed(color), 1.0);
        return;
    }
    vec3 color = ray.y >= 0.0 ? mix(u_horizon, u_top, pow(ray.y, 0.55))
                              : mix(u_horizon, u_below, clamp(-ray.y * 6.0, 0.0, 1.0));
    float near_sun = max(dot(ray, u_sun_dir), 0.0);
    color += u_sun_color * (pow(near_sun, 8.0) * 0.12 + pow(near_sun, 96.0) * 0.35);    // the glow round it
    color = mix(color, u_sun_color + vec3(0.25), smoothstep(0.99965, 0.9998, near_sun));   // the disc
    gl_FragColor = vec4(color, 1.0);
}
)";

// MOTES: small things adrift - specks of light, bubbles, snow, dust. Each is a square turned to the
// eye; a_uv runs -1 .. 1 across it, a_color is its colour and how bright it is.
static const char* MOTE_CORNER = R"(
attribute vec3 a_pos;
attribute vec2 a_uv;
attribute vec4 a_color;
uniform mat4 u_view_lens;
varying vec2 v_uv;
varying vec4 v_color;
void main() {
    v_uv = a_uv;
    v_color = a_color;
    gl_Position = u_view_lens * vec4(a_pos, 1.0);
}
)";

static const char* MOTE_PIXEL = R"(
uniform float u_bubble;         // 1: a bubble (a bright rim and a glint); 0: a soft speck of light
varying vec2 v_uv;
varying vec4 v_color;
void main() {
    float far = length(v_uv);
    if(far > 1.0) discard;
    float shown = 0.0;
    if(u_bubble > 0.5) {
        float rim = smoothstep(0.62, 0.92, far) * (1.0 - smoothstep(0.92, 1.0, far));
        float glint = 1.0 - smoothstep(0.0, 0.26, length(v_uv - vec2(-0.38, 0.42)));
        shown = rim * 0.75 + glint * 0.9 + 0.07;
    } else {
        shown = (1.0 - far) * (1.0 - far);
    }
    gl_FragColor = vec4(v_color.rgb, shown * v_color.a);
}
)";

// BLOOM: the bright parts of the finished picture, blurred, are added back over it - so a bright
// thing has a soft light round it, as it has through a lens or an eye. One program does every step:
// it reads a picture at five places along `u_step` and adds them up, a soft blur; in the first step
// (`u_first`) only what is bright is let through.
static const char* GLOW_CORNER = R"(
attribute vec3 a_pos;
varying vec2 v_at;
void main() {
    v_at = a_pos.xy * 0.5 + 0.5;
    gl_Position = vec4(a_pos.xy, 0.0, 1.0);
}
)";

static const char* GLOW_PIXEL = R"(
uniform sampler2D u_from;
uniform vec2 u_step;
uniform float u_first;
uniform float u_strength;
varying vec2 v_at;
vec3 take(vec2 at) {
    vec3 c = texture2D(u_from, at).rgb;
    if(u_first > 0.5) c *= smoothstep(0.58, 1.0, max(c.r, max(c.g, c.b)));
    return c;
}
void main() {
    vec3 sum = take(v_at) * 0.227
             + (take(v_at + u_step * 1.384) + take(v_at - u_step * 1.384)) * 0.316
             + (take(v_at + u_step * 3.231) + take(v_at - u_step * 3.231)) * 0.07;
    gl_FragColor = vec4(sum * u_strength, 1.0);
}
)";

struct glow_program { GLuint id = 0; GLint from, step, first, strength; };
static glow_program g_glow;

struct mote_program { GLuint id = 0; GLint view_lens, bubble; };
static mote_program g_mote;

struct shaft_program { GLuint id = 0; GLint view_lens, color; };
static shaft_program g_shaft;

static const int PLACE_AT = 4;         // the attribute a_place: 4, 5, 6 and 7 (0 to 3 are a corner's own)
static bool g_many_at_once = false;     // can the card draw many copies in one call? (draw_models)

struct surface_program {
    GLuint id = 0;
    GLint tex, shadow_map, has_tex, tint, cutout, unlit, shiny, glow;
    GLint sun_dir, sun_color, sky_light, ground_light, eye, fog_color, fog, lamp_at, lamp_color;
    GLint shadows, shadow_step, depth_only, repeat, wrap, time;
    GLint view_lens, sun_view, sway, clock, water, water_shallow, water_deep, grain, rough;
};
struct sky_program { GLuint id = 0; GLint top, horizon, below, sun_dir, sun_color, eye, time, water, water_shallow, water_deep; };

static surface_program g_surface;                   // the one every thing is drawn with ...
static std::vector<surface_program> g_own;          // ... and the programs' own (g3_shader): id = place + 1
static sky_program g_sky;

// compile one shader from pieces of text; what went wrong (if anything) is added to `wrong`
static GLuint compiled(GLenum kind, const std::vector<const char*>& texts, const char* what, std::string& wrong) {
    const char* first = g3_es
        ? "#version 100\n#ifdef GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n#else\nprecision mediump float;\n#endif\n"
        : "#version 120\n";
    std::vector<const char*> parts;
    parts.push_back(first);
    parts.insert(parts.end(), texts.begin(), texts.end());
    GLuint shader = glCreateShader(kind);
    glShaderSource(shader, (GLsizei) parts.size(), parts.data(), nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if(!ok) {
        char said[1024] = "";
        glGetShaderInfoLog(shader, sizeof(said) - 1, nullptr, said);
        wrong += std::string("the graphics card refused the ") + what + " shader: " + said;
    }
    return shader;
}

// a whole program from the two shaders' texts; 0 and `wrong` filled when the card refuses it
static GLuint linked(const std::vector<const char*>& corner, const std::vector<const char*>& pixel, const char* what, std::string& wrong) {
    GLuint program = glCreateProgram();
    GLuint a = compiled(GL_VERTEX_SHADER, corner, what, wrong), b = compiled(GL_FRAGMENT_SHADER, pixel, what, wrong);
    glAttachShader(program, a);
    glAttachShader(program, b);
    glBindAttribLocation(program, 0, "a_pos");
    glBindAttribLocation(program, 1, "a_normal");
    glBindAttribLocation(program, 2, "a_uv");
    glBindAttribLocation(program, 3, "a_color");
    glBindAttribLocation(program, PLACE_AT, "a_place");         // (a matrix: it takes four places, 4 to 7)
    glLinkProgram(program);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if(!ok && wrong.empty()) {
        char said[1024] = "";
        glGetProgramInfoLog(program, sizeof(said) - 1, nullptr, said);
        wrong += std::string("the graphics card refused the ") + what + " program: " + said;
    }
    if(!wrong.empty()) {
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

// the surface program with this `look` function in it (SURFACE_LOOK: the one that changes nothing)
static bool surface_with(const char* look, surface_program& s, std::string& wrong) {
    s.id = linked({ SURFACE_CORNER }, { SURFACE_PIXEL, WATER_SHARED, look, SURFACE_MAIN }, "surface", wrong);
    if(s.id == 0) return false;
#define AT(name) s.name = glGetUniformLocation(s.id, "u_" #name)
    AT(tex); AT(shadow_map); AT(has_tex); AT(tint); AT(cutout); AT(unlit);
    AT(shiny); AT(glow); AT(sun_dir); AT(sun_color); AT(sky_light); AT(ground_light); AT(eye); AT(fog_color);
    AT(fog); AT(lamp_at); AT(lamp_color); AT(shadows); AT(shadow_step); AT(depth_only); AT(repeat); AT(wrap); AT(time);
    AT(view_lens); AT(sun_view); AT(sway); AT(clock); AT(water); AT(water_shallow); AT(water_deep); AT(grain); AT(rough);
#undef AT
    return true;
}

static void build_programs() {
    if(!surface_with(SURFACE_LOOK, g_surface, g3_why)) { g3_failed = true; return; }
    g_sky.id = linked({ SKY_CORNER }, { SKY_PIXEL, WATER_SHARED, SKY_MAIN }, "sky", g3_why);
    if(g_sky.id == 0) { g3_failed = true; return; }
    g_sky.top = glGetUniformLocation(g_sky.id, "u_top");
    g_sky.horizon = glGetUniformLocation(g_sky.id, "u_horizon");
    g_sky.below = glGetUniformLocation(g_sky.id, "u_below");
    g_sky.sun_dir = glGetUniformLocation(g_sky.id, "u_sun_dir");
    g_sky.sun_color = glGetUniformLocation(g_sky.id, "u_sun_color");
    g_sky.eye = glGetUniformLocation(g_sky.id, "u_eye");
    g_sky.time = glGetUniformLocation(g_sky.id, "u_time");
    g_sky.water = glGetUniformLocation(g_sky.id, "u_water");
    g_sky.water_shallow = glGetUniformLocation(g_sky.id, "u_water_shallow");
    g_sky.water_deep = glGetUniformLocation(g_sky.id, "u_water_deep");
    g_shaft.id = linked({ SHAFT_CORNER }, { SHAFT_PIXEL }, "light shaft", g3_why);
    if(g_shaft.id == 0) { g3_failed = true; return; }
    g_shaft.view_lens = glGetUniformLocation(g_shaft.id, "u_view_lens");
    g_shaft.color = glGetUniformLocation(g_shaft.id, "u_shaft_color");
    g_mote.id = linked({ MOTE_CORNER }, { MOTE_PIXEL }, "mote", g3_why);
    if(g_mote.id == 0) { g3_failed = true; return; }
    g_mote.view_lens = glGetUniformLocation(g_mote.id, "u_view_lens");
    g_mote.bubble = glGetUniformLocation(g_mote.id, "u_bubble");
    g_glow.id = linked({ GLOW_CORNER }, { GLOW_PIXEL }, "bloom", g3_why);
    if(g_glow.id == 0) { g3_failed = true; return; }
    g_glow.from = glGetUniformLocation(g_glow.id, "u_from");
    g_glow.step = glGetUniformLocation(g_glow.id, "u_step");
    g_glow.first = glGetUniformLocation(g_glow.id, "u_first");
    g_glow.strength = glGetUniformLocation(g_glow.id, "u_strength");
    g_many_at_once = !g3_es && glVertexAttribDivisor != nullptr && glDrawElementsInstanced != nullptr;
}

// =================================================================================
// 4. Textures
// =================================================================================

struct g3_texture { GLuint id = 0; int width = 0, height = 0; bool see_through = false; };

static std::vector<g3_texture> g_textures;              // texture id = place + 1
static std::map<std::string, int64_t> g_texture_ids;    // by file (or name), so each is loaded once

static g3_texture* texture_of(int64_t id) {
    return id >= 1 && id <= (int64_t) g_textures.size() ? &g_textures[(size_t) id - 1] : nullptr;
}

static int64_t texture_from_pixels(const std::string& key, const unsigned char* pixels, int width, int height) {
    g3_texture made;
    made.width = width;
    made.height = height;
    for(int i = 0; i < width * height; i++)
        if(pixels[i * 4 + 3] < 250) { made.see_through = true; break; }

    glGenTextures(1, &made.id);
    glBindTexture(GL_TEXTURE_2D, made.id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    // smaller copies of the picture for when it is far away (without them a fine texture
    // shimmers in the distance). OpenGL ES 2 only makes them for sizes that are powers of two.
    bool powers = (width & (width - 1)) == 0 && (height & (height - 1)) == 0;
    bool smaller = !g3_es || powers;
    if(smaller) glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smaller ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLint wrap = !g3_es || powers ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

    g_textures.push_back(made);
    int64_t id = (int64_t) g_textures.size();
    g_texture_ids[key] = id;
    return id;
}

int64_t g3_texture_file(sni_env* env, const std::string& path) {
    auto known = g_texture_ids.find(path);
    if(known != g_texture_ids.end()) return known->second;
    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if(pixels == nullptr) {
        env->raise("std#io_exception", "the texture could not be loaded: " + path + " (" + stbi_failure_reason() + ")");
        return 0;
    }
    int64_t id = texture_from_pixels(path, pixels, width, height);
    stbi_image_free(pixels);
    return id;
}

// A colour picture and a separate picture of how solid each pixel is (white: solid, black:
// nothing there), as one texture - how many asset libraries ship leaves: the colours in a .jpg,
// which cannot hold see-through pixels, and the cut-out beside it. 0 when either cannot be read.
int64_t g3_texture_cut(sni_env* env, const std::string& color_path, const std::string& solid_path) {
    std::string key = color_path + "+" + solid_path;
    auto known = g_texture_ids.find(key);
    if(known != g_texture_ids.end()) return known->second;
    int width = 0, height = 0, channels = 0, solid_w = 0, solid_h = 0;
    unsigned char* solid = stbi_load(solid_path.c_str(), &solid_w, &solid_h, &channels, 1);
    if(solid == nullptr) return 0;
    unsigned char* pixels = stbi_load(color_path.c_str(), &width, &height, &channels, 4);
    if(pixels == nullptr) { stbi_image_free(solid); return 0; }
    for(int y = 0; y < height; y++)
        for(int x = 0; x < width; x++)
            pixels[(y * width + x) * 4 + 3] = solid[(y * solid_h / height) * solid_w + x * solid_w / width];
    int64_t id = texture_from_pixels(key, pixels, width, height);
    stbi_image_free(pixels);
    stbi_image_free(solid);
    return id;
}

int64_t g3_texture_bytes(sni_env* env, const std::string& key, const unsigned char* bytes, int length) {
    auto known = g_texture_ids.find(key);
    if(known != g_texture_ids.end()) return known->second;
    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = stbi_load_from_memory(bytes, length, &width, &height, &channels, 4);
    if(pixels == nullptr) {
        env->raise("std#io_exception", "a texture inside " + key + " could not be read (" + stbi_failure_reason() + ")");
        return 0;
    }
    int64_t id = texture_from_pixels(key, pixels, width, height);
    stbi_image_free(pixels);
    return id;
}

// g3_texture(path): a picture file as a texture. Returns its id; the same file gives the same id.
int64_t sharpgui_g3_texture(sni_env* env, sni_ref path_ref) {
    if(!g3_begin(env)) return 0;
    int64_t id = g3_texture_file(env, env->text(path_ref));
    g3_end();
    return id;
}

// g3_texture_pixels(name, pixels, width, height): a texture made by the program: width x height
// colours, 0xRRGGBBAA, row after row from the top. The same name replaces nothing: it gives the one made first.
int64_t sharpgui_g3_texture_pixels(sni_env* env, sni_ref name_ref, sni_ref pixels_ref, int64_t width, int64_t height) {
    if(!g3_begin(env)) return 0;
    std::string name = env->text(name_ref);
    int64_t id = 0;
    auto known = g_texture_ids.find(name);
    sni_array colors;
    if(known != g_texture_ids.end()) {
        id = known->second;
    } else if(width > 0 && height > 0 && gui_numbers(env, pixels_ref, SNI_INT64, width * height, "the texture's list of colours", &colors)) {
        const int64_t* color = (const int64_t*) colors.data;
        std::vector<unsigned char> pixels((size_t) (width * height * 4));
        for(int64_t i = 0; i < width * height; i++) {
            pixels[i * 4] = (unsigned char) ((color[i] >> 24) & 255);
            pixels[i * 4 + 1] = (unsigned char) ((color[i] >> 16) & 255);
            pixels[i * 4 + 2] = (unsigned char) ((color[i] >> 8) & 255);
            pixels[i * 4 + 3] = (unsigned char) (color[i] & 255);
        }
        id = texture_from_pixels(name, pixels.data(), (int) width, (int) height);
    }
    g3_end();
    return id;
}

// =================================================================================
// 5. Models
// =================================================================================

struct g3_part {
    GLuint corners = 0, triangles = 0;
    int count = 0;                  // how many corner numbers the triangles are
    GLenum number_kind = GL_UNSIGNED_SHORT;
    g3_material material;
};

struct g3_model {
    std::vector<g3_part> parts;
    float low[3] = { 0, 0, 0 }, high[3] = { 0, 0, 0 };  // the box round all of it
    bool any = false;
    float middle[3] = { 0, 0, 0 };
    float reach = 0;                                     // from the middle to its furthest corner
    int triangle_count = 0;
};

static std::vector<g3_model> g_models;                  // model id = place + 1
static std::map<std::string, int64_t> g_model_files;    // by file, so each is loaded once

static g3_model* model_of(int64_t id) {
    return id >= 1 && id <= (int64_t) g_models.size() ? &g_models[(size_t) id - 1] : nullptr;
}

int64_t g3_new_model() {
    g_models.push_back(g3_model());
    return (int64_t) g_models.size();
}

static void upload(sni_env* env, g3_part& part, const std::vector<g3_vertex>& corners, const std::vector<uint32_t>& triangles, bool& ok) {
    ok = true;
    if(part.corners == 0) glGenBuffers(1, &part.corners);
    if(part.triangles == 0) glGenBuffers(1, &part.triangles);
    glBindBuffer(GL_ARRAY_BUFFER, part.corners);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (corners.size() * sizeof(g3_vertex)), corners.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, part.triangles);
    if(corners.size() <= 65535) {
        std::vector<uint16_t> narrow(triangles.begin(), triangles.end());
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr) (narrow.size() * 2), narrow.data(), GL_STATIC_DRAW);
        part.number_kind = GL_UNSIGNED_SHORT;
    } else if(g3_wide_indices) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr) (triangles.size() * 4), triangles.data(), GL_STATIC_DRAW);
        part.number_kind = GL_UNSIGNED_INT;
    } else {
        env->raise("std#illegal_argument_exception", "this machine's graphics card takes at most 65,535 corners in one part of a model: split it");
        ok = false;
    }
    part.count = (int) triangles.size();
}

static void grow_box(g3_model& model, const std::vector<g3_vertex>& corners) {
    for(const g3_vertex& c : corners) {
        const float at[3] = { c.x, c.y, c.z };
        for(int k = 0; k < 3; k++) {
            if(!model.any || at[k] < model.low[k]) model.low[k] = at[k];
            if(!model.any || at[k] > model.high[k]) model.high[k] = at[k];
        }
        model.any = true;
    }
    float far = 0;
    for(int k = 0; k < 3; k++) {
        model.middle[k] = (model.low[k] + model.high[k]) / 2;
        float half = (model.high[k] - model.low[k]) / 2;
        far += half * half;
    }
    model.reach = std::sqrt(far);
}

int64_t g3_add_part(sni_env* env, int64_t model_id, const std::vector<g3_vertex>& corners,
                    const std::vector<uint32_t>& triangles, const g3_material& material) {
    g3_model* model = model_of(model_id);
    if(model == nullptr || corners.empty() || triangles.empty()) return -1;
    g3_part part;
    part.material = material;
    bool ok = false;
    upload(env, part, corners, triangles, ok);
    if(!ok) return -1;
    model->parts.push_back(part);
    model->triangle_count += (int) triangles.size() / 3;
    grow_box(*model, corners);
    return (int64_t) model->parts.size() - 1;
}

// the lists a mesh hands over, as corners and triangles (false, with an exception, when they do not fit together)
static bool corners_from(sni_env* env, sni_ref points_ref, sni_ref normals_ref, sni_ref uvs_ref, sni_ref colors_ref,
                         sni_ref faces_ref, int64_t corner_count, int64_t face_count,
                         std::vector<g3_vertex>& corners, std::vector<uint32_t>& triangles) {
    sni_array points, normals, uvs, colors, faces;
    if(corner_count <= 0 || face_count <= 0) return false;
    if(!gui_numbers(env, points_ref, SNI_DECIMAL, corner_count * 3, "the mesh's list of corners", &points)) return false;
    if(!gui_numbers(env, normals_ref, SNI_DECIMAL, corner_count * 3, "the mesh's list of facings", &normals)) return false;
    if(!gui_numbers(env, uvs_ref, SNI_DECIMAL, corner_count * 2, "the mesh's list of texture points", &uvs)) return false;
    if(!gui_numbers(env, colors_ref, SNI_INT64, corner_count, "the mesh's list of corner colours", &colors)) return false;
    if(!gui_numbers(env, faces_ref, SNI_INT32, face_count * 3, "the mesh's list of triangles", &faces)) return false;

    const double* p = (const double*) points.data;
    const double* n = (const double*) normals.data;
    const double* t = (const double*) uvs.data;
    const int64_t* c = (const int64_t*) colors.data;
    const int32_t* f = (const int32_t*) faces.data;
    corners.resize((size_t) corner_count);
    for(int64_t i = 0; i < corner_count; i++) {
        g3_vertex& v = corners[(size_t) i];
        v.x = (float) p[i * 3]; v.y = (float) p[i * 3 + 1]; v.z = (float) p[i * 3 + 2];
        v.nx = (float) n[i * 3]; v.ny = (float) n[i * 3 + 1]; v.nz = (float) n[i * 3 + 2];
        v.u = (float) t[i * 2]; v.v = (float) t[i * 2 + 1];
        v.r = (uint8_t) ((c[i] >> 24) & 255); v.g = (uint8_t) ((c[i] >> 16) & 255);
        v.b = (uint8_t) ((c[i] >> 8) & 255); v.a = (uint8_t) (c[i] & 255);
    }
    triangles.resize((size_t) face_count * 3);
    for(int64_t i = 0; i < face_count * 3; i++) {
        if(f[i] < 0 || f[i] >= corner_count) {
            env->raise("std#illegal_argument_exception", "a triangle of the mesh names corner " + std::to_string(f[i])
                       + ", and the mesh has " + std::to_string(corner_count));
            return false;
        }
        triangles[(size_t) i] = (uint32_t) f[i];
    }
    return true;
}

// g3_model(): a new, empty model. Parts are added with g3_model_part.
int64_t sharpgui_g3_model(sni_env* env) {
    if(!g3_begin(env)) return 0;
    int64_t id = g3_new_model();
    g3_end();
    return id;
}

//
// g3_model_part(model, part, points, normals, uvs, colours, faces, corner_count, face_count)
// Put a mesh on the graphics card as a part of the model. part -1 adds a new part; a part's own
// number replaces what it holds (a mesh that changes). Returns the part's number, -1 when refused.
//
int64_t sharpgui_g3_model_part(sni_env* env, int64_t model_id, int64_t part_number, sni_ref points_ref, sni_ref normals_ref,
                               sni_ref uvs_ref, sni_ref colors_ref, sni_ref faces_ref, int64_t corner_count, int64_t face_count) {
    if(!g3_begin(env)) return -1;
    int64_t result = -1;
    g3_model* model = model_of(model_id);
    std::vector<g3_vertex> corners;
    std::vector<uint32_t> triangles;
    if(model == nullptr) {
        env->raise("std#illegal_argument_exception", "that model does not exist");
    } else if(corners_from(env, points_ref, normals_ref, uvs_ref, colors_ref, faces_ref, corner_count, face_count, corners, triangles)) {
        if(part_number >= 0 && part_number < (int64_t) model->parts.size()) {
            g3_part& part = model->parts[(size_t) part_number];
            bool ok = false;
            model->triangle_count -= part.count / 3;
            upload(env, part, corners, triangles, ok);
            model->triangle_count += part.count / 3;
            model->any = false;                 // the box is measured again from this part alone
            grow_box(*model, corners);
            if(ok) result = part_number;
        } else {
            result = g3_add_part(env, model_id, corners, triangles, g3_material());
        }
    }
    g3_end();
    return result;
}

// g3_part_look(model, part, colour, texture, shiny, glow, flags): what a part is made of. part -1: every part.
void sharpgui_g3_part_look(sni_env* env, int64_t model_id, int64_t part_number, int64_t color, int64_t texture,
                           double shiny, double glow, int64_t flags) {
    g3_model* model = model_of(model_id);
    if(model == nullptr) return;
    for(size_t i = 0; i < model->parts.size(); i++) {
        if(part_number >= 0 && part_number != (int64_t) i) continue;
        g3_material& m = model->parts[i].material;
        color_of((double) color, m.color);
        m.texture = texture;
        m.shiny = (float) shiny;
        m.glow = (float) glow;
        m.flags = (int) flags;
    }
}

// g3_model_load(path): a model file (.obj, .gltf, .glb) with its textures. The same file gives the same model.
int64_t sharpgui_g3_model_load(sni_env* env, sni_ref path_ref) {
    std::string path = env->text(path_ref);
    auto known = g_model_files.find(path);
    if(known != g_model_files.end()) return known->second;
    if(!g3_begin(env)) return 0;
    int64_t id = g3_load_model_file(env, path);
    if(id != 0) g_model_files[path] = id;
    g3_end();
    return id;
}

// g3_model_box(model, into): the box round the model - low x y z, high x y z - into a list of six. Returns its triangles.
int64_t sharpgui_g3_model_box(sni_env* env, int64_t model_id, sni_ref into_ref) {
    g3_model* model = model_of(model_id);
    sni_array into;
    if(model == nullptr || !gui_numbers(env, into_ref, SNI_DECIMAL, 6, "the list for the model's box", &into)) return 0;
    double* box = (double*) into.data;
    for(int k = 0; k < 3; k++) { box[k] = model->low[k]; box[3 + k] = model->high[k]; }
    return model->triangle_count;
}

// =================================================================================
// 6. Many copies of one model (scatter)
// =================================================================================

struct g3_copies {
    std::vector<float> where;                           // x y z heading scale, for every copy
    float middle[3] = { 0, 0, 0 };                      // the middle of all the places ...
    float reach = 0;                                    // ... and how far from it the furthest one is
    float largest = 1;                                  // the largest scale among them
};

static std::vector<g3_copies> g_copies;                 // id = place + 1

// g3_copies(places, count): remember where `count` copies stand: x y z heading scale for each. Returns an id.
int64_t sharpgui_g3_copies(sni_env* env, sni_ref places_ref, int64_t count) {
    sni_array places;
    if(count <= 0 || !gui_numbers(env, places_ref, SNI_DECIMAL, count * 5, "the list of places", &places)) return 0;
    const double* p = (const double*) places.data;
    g3_copies made;
    made.where.assign(p, p + count * 5);
    // (the ball round all of them: a whole scattering that is out of sight is passed over at once)
    float low[3] = { (float) p[0], (float) p[1], (float) p[2] }, high[3] = { (float) p[0], (float) p[1], (float) p[2] };
    for(int64_t c = 0; c < count; c++) {
        for(int k = 0; k < 3; k++) {
            low[k] = std::min(low[k], (float) p[c * 5 + k]);
            high[k] = std::max(high[k], (float) p[c * 5 + k]);
        }
        made.largest = std::max(made.largest, (float) p[c * 5 + 4]);
    }
    float across = 0;
    for(int k = 0; k < 3; k++) {
        made.middle[k] = (low[k] + high[k]) / 2;
        across += (high[k] - low[k]) * (high[k] - low[k]);
    }
    made.reach = std::sqrt(across) / 2;
    g_copies.push_back(made);
    return (int64_t) g_copies.size();
}

// g3_drop(model, copies): a model's shapes are taken off the graphics card, and the places of copies
// are forgotten (0 for either: none). Land that follows the eye drops what was left far behind.
// The ids stay taken: a thing that still shows one draws nothing.
int64_t sharpgui_g3_drop(sni_env* env, int64_t model_id, int64_t copies_id) {
    g3_model* model = model_of(model_id);
    if(model != nullptr && !model->parts.empty()) {
        if(!g3_begin(env)) return 0;
        for(g3_part& part : model->parts) {
            if(part.corners != 0) glDeleteBuffers(1, &part.corners);
            if(part.triangles != 0) glDeleteBuffers(1, &part.triangles);
        }
        model->parts.clear();
        model->any = false;
        model->triangle_count = 0;
        g3_end();
    }
    if(copies_id >= 1 && copies_id <= (int64_t) g_copies.size()) {
        std::vector<float>().swap(g_copies[(size_t) copies_id - 1].where);
    }
    return 1;
}

// =================================================================================
// 7. Scenes
// =================================================================================
//
// A scene is kept as the Sharp side hands it over (scene.sharp packs it):
//
//      NODES   35 numbers each (NODE_... below)
//      LAMPS   8 numbers each: x y z, colour, reach, strength, the node it hangs on (-1: none), -
//      LOOK    48 numbers: the camera, the sun, the sky, the fog, the water (LOOK_... below)
//
enum { NODE_PARENT, NODE_MODEL, NODE_X, NODE_Y, NODE_Z, NODE_YAW, NODE_PITCH, NODE_ROLL, NODE_SX, NODE_SY, NODE_SZ,
       NODE_SPIN_YAW, NODE_SPIN_PITCH, NODE_SPIN_ROLL, NODE_TINT, NODE_TEXTURE, NODE_SHINY, NODE_GLOW, NODE_FLAGS,
       NODE_COPIES, NODE_FAR_MODEL, NODE_FAR_FROM, NODE_BOB, NODE_BOB_RATE, NODE_REPEAT, NODE_SHADER, NODE_SWAY, NODE_ROUGH,
       // moving by itself: swinging round its up axis (wag) or its length (flap) by so many degrees, so many
       // times in 6.28 seconds (beat); going round a circle of this radius in so many seconds; its copies
       // swimming round it at this speed (flock); and, for a thing that is MOTES, how many there are
       NODE_WAG, NODE_FLAP, NODE_BEAT, NODE_CIRCLE, NODE_CIRCLE_TIME, NODE_FLOCK, NODE_MOTES, NODE_SIZE };
enum { NODE_HIDDEN = 1, NODE_FACES_EYE = 2, NODE_NO_SHADOW = 4, NODE_UNLIT = 8, NODE_NOT_CLICKED = 16, NODE_BUBBLES = 32 };
enum { LAMP_SIZE = 8 };
enum { LOOK_EYE_X, LOOK_EYE_Y, LOOK_EYE_Z, LOOK_HEADING, LOOK_TILT, LOOK_FOV, LOOK_NEAR, LOOK_FAR,
       LOOK_SUN_X, LOOK_SUN_Y, LOOK_SUN_Z, LOOK_SUN_COLOR, LOOK_SUN_STRENGTH, LOOK_SKY_LIGHT, LOOK_GROUND_LIGHT,
       LOOK_SKY_TOP, LOOK_SKY_HORIZON, LOOK_SKY_ON, LOOK_FOG_COLOR, LOOK_FOG_FROM, LOOK_FOG_TO,
       LOOK_SHADOWS, LOOK_SHADOW_REACH, LOOK_BACKGROUND, LOOK_AT_ON, LOOK_AT_X, LOOK_AT_Y, LOOK_AT_Z,
       LOOK_SKY_BELOW,
       // water the eye is inside of (0 in LOOK_WATER_ON: none): its colour near the surface and in the deep, how
       // high its surface is, how strong the moving light on things is (0 .. 1), how fast things fade into it
       // (per metre), and how strong the shafts of light from the surface are (0 .. 1)
       LOOK_WATER_ON, LOOK_WATER_SHALLOW, LOOK_WATER_DEEP, LOOK_WATER_SURFACE, LOOK_WATER_CAUSTICS, LOOK_WATER_HAZE,
       LOOK_WATER_SHAFTS,
       // how much light the bright parts of the picture spill round themselves (0: none)
       LOOK_BLOOM, LOOK_SIZE = 48 };

struct g3_scene {
    std::vector<double> nodes;      // (doubles: a colour is a number of 32 bits, more than a float holds exactly)
    int node_count = 0;
    std::vector<double> lamps;
    int lamp_count = 0;
    double look[LOOK_SIZE] = { 0 };
    // worked out when it is drawn, kept for clicks:
    std::vector<mat4> world;        // where each node is
    mat4 view, lens;
    float eye[3] = { 0, 0, 0 };
    int drawn_triangles = 0, drawn_models = 0;
    float seen_w = 1, seen_h = 1;
};

static std::vector<g3_scene> g_scenes;                  // id = place + 1

static g3_scene* scene_of(int64_t id) {
    return id >= 1 && id <= (int64_t) g_scenes.size() ? &g_scenes[(size_t) id - 1] : nullptr;
}

// g3_scene(): a new, empty scene. Returns its id.
int64_t sharpgui_g3_scene(sni_env* env) {
    g_scenes.push_back(g3_scene());
    return (int64_t) g_scenes.size();
}

// g3_scene_nodes(scene, nodes, count, lamps, lamp_count): what is in the scene, as packed lists (copied)
void sharpgui_g3_scene_nodes(sni_env* env, int64_t scene_id, sni_ref nodes_ref, int64_t count, sni_ref lamps_ref, int64_t lamp_count) {
    g3_scene* scene = scene_of(scene_id);
    if(scene == nullptr) return;
    sni_array nodes, lamps;
    scene->node_count = 0;
    scene->lamp_count = 0;
    if(count > 0) {
        if(!gui_numbers(env, nodes_ref, SNI_DECIMAL, count * NODE_SIZE, "the scene's list of things", &nodes)) return;
        const double* n = (const double*) nodes.data;
        scene->nodes.assign(n, n + count * NODE_SIZE);
        scene->node_count = (int) count;
    }
    if(lamp_count > 0) {
        if(!gui_numbers(env, lamps_ref, SNI_DECIMAL, lamp_count * LAMP_SIZE, "the scene's list of lamps", &lamps)) return;
        const double* l = (const double*) lamps.data;
        scene->lamps.assign(l, l + lamp_count * LAMP_SIZE);
        scene->lamp_count = (int) lamp_count;
    }
}

// g3_scene_look(scene, look): the camera, the sun, the sky and the fog: 32 numbers (copied)
void sharpgui_g3_scene_look(sni_env* env, int64_t scene_id, sni_ref look_ref) {
    g3_scene* scene = scene_of(scene_id);
    sni_array look;
    if(scene == nullptr || !gui_numbers(env, look_ref, SNI_DECIMAL, LOOK_SIZE, "the scene's camera and light", &look)) return;
    const double* l = (const double*) look.data;
    for(int i = 0; i < LOOK_SIZE; i++) scene->look[i] = l[i];
}

// ---- the picture a scene is drawn into ------------------------------------------------------

struct g3_target {
    int width = 0, height = 0;
    GLuint frame = 0, picture = 0, depth = 0;           // what ends up being read back
    GLuint smooth_frame = 0, smooth_color = 0, smooth_depth = 0;     // drawn into first where edges can be smoothed
    GLuint glow_frame[2] = { 0, 0 }, glow_picture[2] = { 0, 0 };     // bloom: two small pictures to blur between
    int glow_w = 0, glow_h = 0;
    SDL_Texture* shown = nullptr;                       // the 2D side's copy
    std::vector<unsigned char> pixels;
};

static g3_target g_target;

static void forget_target() {
    g3_target& t = g_target;
    if(t.frame) glDeleteFramebuffers(1, &t.frame);
    if(t.picture) glDeleteTextures(1, &t.picture);
    if(t.depth) glDeleteRenderbuffers(1, &t.depth);
    if(t.smooth_frame) glDeleteFramebuffers(1, &t.smooth_frame);
    if(t.smooth_color) glDeleteRenderbuffers(1, &t.smooth_color);
    if(t.smooth_depth) glDeleteRenderbuffers(1, &t.smooth_depth);
    t.frame = t.picture = t.depth = t.smooth_frame = t.smooth_color = t.smooth_depth = 0;
    for(int i = 0; i < 2; i++) {
        if(t.glow_frame[i]) glDeleteFramebuffers(1, &t.glow_frame[i]);
        if(t.glow_picture[i]) glDeleteTextures(1, &t.glow_picture[i]);
        t.glow_frame[i] = t.glow_picture[i] = 0;
    }
}

// BLOOM over the finished picture (in t.frame): what is bright in it, made small and blurred across
// and down twice over, is added back. The small pictures are a quarter of the size each way: the
// blur is wide and costs little.
static void add_bloom(g3_target& t, float strength) {
    int w = std::max(8, t.width / 4), h = std::max(8, t.height / 4);
    if(t.glow_frame[0] == 0 || t.glow_w != w || t.glow_h != h) {
        for(int i = 0; i < 2; i++) {
            if(t.glow_frame[i]) glDeleteFramebuffers(1, &t.glow_frame[i]);
            if(t.glow_picture[i]) glDeleteTextures(1, &t.glow_picture[i]);
            glGenTextures(1, &t.glow_picture[i]);
            glBindTexture(GL_TEXTURE_2D, t.glow_picture[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glGenFramebuffers(1, &t.glow_frame[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, t.glow_frame[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.glow_picture[i], 0);
        }
        t.glow_w = w;
        t.glow_h = h;
    }

    // the whole picture as two triangles
    static GLuint buffer = 0;
    g3_vertex corner[4];
    const float cx[4] = { -1, 1, -1, 1 }, cy[4] = { -1, -1, 1, 1 };
    for(int i = 0; i < 4; i++) {
        memset(&corner[i], 0, sizeof(g3_vertex));
        corner[i].x = cx[i];
        corner[i].y = cy[i];
    }
    if(buffer == 0) glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corner), corner, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(g3_vertex), (const void*) offsetof(g3_vertex, x));
    for(int i = 1; i < 4; i++) glDisableVertexAttribArray((GLuint) i);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glUseProgram(g_glow.id);
    glUniform1i(g_glow.from, 0);
    glUniform1f(g_glow.strength, 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glViewport(0, 0, w, h);

    // four steps: across (only the bright parts), down, across again wider, down again wider
    const float wide[4] = { 1.0f, 1.0f, 2.4f, 2.4f };
    for(int pass = 0; pass < 4; pass++) {
        int into = pass % 2;
        glBindFramebuffer(GL_FRAMEBUFFER, t.glow_frame[into]);
        glBindTexture(GL_TEXTURE_2D, pass == 0 ? t.picture : t.glow_picture[1 - into]);
        glUniform1f(g_glow.first, pass == 0 ? 1.0f : 0.0f);
        if(pass % 2 == 0) glUniform2f(g_glow.step, wide[pass] / w, 0);
        else glUniform2f(g_glow.step, 0, wide[pass] / h);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // ... and added to the picture
    glBindFramebuffer(GL_FRAMEBUFFER, t.frame);
    glViewport(0, 0, t.width, t.height);
    glBindTexture(GL_TEXTURE_2D, t.glow_picture[1]);
    glUniform1f(g_glow.first, 0.0f);
    glUniform2f(g_glow.step, 0, 0);
    glUniform1f(g_glow.strength, strength);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    for(int i = 1; i < 4; i++) glEnableVertexAttribArray((GLuint) i);
}

static bool make_target(int width, int height) {
    g3_target& t = g_target;
    if(t.width == width && t.height == height && t.frame != 0) return true;
    forget_target();
    t.width = width;
    t.height = height;

    glGenTextures(1, &t.picture);
    glBindTexture(GL_TEXTURE_2D, t.picture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenRenderbuffers(1, &t.depth);
    glBindRenderbuffer(GL_RENDERBUFFER, t.depth);
    glRenderbufferStorage(GL_RENDERBUFFER, g3_es ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24, width, height);
    glGenFramebuffers(1, &t.frame);
    glBindFramebuffer(GL_FRAMEBUFFER, t.frame);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.picture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, t.depth);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return false;

    // smooth edges: draw with four samples a pixel, then blend them down (desktop OpenGL 3 and later)
    if(!g3_es && glRenderbufferStorageMultisample != nullptr && glBlitFramebuffer != nullptr) {
        GLint most = 0;
        glGetIntegerv(GL_MAX_SAMPLES, &most);
        int samples = most >= 4 ? 4 : 0;
        if(samples > 0) {
            glGenRenderbuffers(1, &t.smooth_color);
            glBindRenderbuffer(GL_RENDERBUFFER, t.smooth_color);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height);
            glGenRenderbuffers(1, &t.smooth_depth);
            glBindRenderbuffer(GL_RENDERBUFFER, t.smooth_depth);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height);
            glGenFramebuffers(1, &t.smooth_frame);
            glBindFramebuffer(GL_FRAMEBUFFER, t.smooth_frame);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, t.smooth_color);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, t.smooth_depth);
            if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                glDeleteFramebuffers(1, &t.smooth_frame);
                t.smooth_frame = 0;
            }
        }
    }
    t.pixels.resize((size_t) width * height * 4);
    return true;
}

// ---- the sun's view of the world, for shadows -----------------------------------------------

static int SHADOW_SIZE = 2048;             // pixels along each side of the shadow map (4096 on a PC: make_shadow_map)
static GLuint g_shadow_frame = 0, g_shadow_map = 0;

static bool make_shadow_map() {
    if(g_shadow_frame != 0) return true;
    if(!g3_depth_pictures) return false;
    // (a PC's card has room for a map twice as fine each way: shadows four times as sharp)
    GLint largest = 2048;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &largest);
    SHADOW_SIZE = !g3_es && largest >= 4096 ? 4096 : 2048;
    glGenTextures(1, &g_shadow_map);
    glBindTexture(GL_TEXTURE_2D, g_shadow_map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_SIZE, SHADOW_SIZE, 0, GL_DEPTH_COMPONENT,
                 g3_es ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &g_shadow_frame);
    glBindFramebuffer(GL_FRAMEBUFFER, g_shadow_frame);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_shadow_map, 0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteFramebuffers(1, &g_shadow_frame);
        g_shadow_frame = 0;
        g3_depth_pictures = false;
        return false;
    }
    return true;
}

// ---- drawing --------------------------------------------------------------------------------

// one model at one place: what the two passes (shadow map, picture) both draw
struct drawn_model {
    const g3_model* model;
    mat4 world;
    const double* node;     // its settings (tint, texture ...)
    float far_away;         // from the eye, for putting see-through things in order
};

static std::vector<drawn_model> g_drawn;

//
// WHERE A THING STANDS reaches the card as the attribute a_place (four numbers four times: a matrix).
//
// One thing: the attribute is set to one value for every corner (place_one) and the thing is drawn.
//
// MANY COPIES of one model - a scattering, a flock: a forest is thousands - are drawn in ONE call
// (place_many): the places of all of them go to the card as a list, the card is told to take the
// next place for each copy, and it draws them all. The processor's work for a thousand trees is
// then the same as for one. This needs a card of 2010 or later ("instancing"); where it is missing
// (g_many_at_once false) each copy is drawn by itself, as one thing, and the picture is the same.
//
static GLuint g_places = 0;                    // the buffer the list of places is sent in
static std::vector<float> g_place_list;

static void place_one(const mat4& world) {
    for(int k = 0; k < 4; k++) glVertexAttrib4f((GLuint) (PLACE_AT + k), world.m[k * 4], world.m[k * 4 + 1], world.m[k * 4 + 2], world.m[k * 4 + 3]);
}

static void place_many(const drawn_model* first, size_t count) {
    g_place_list.resize(count * 16);
    for(size_t c = 0; c < count; c++) memcpy(&g_place_list[c * 16], first[c].world.m, 16 * sizeof(float));
    if(g_places == 0) glGenBuffers(1, &g_places);
    glBindBuffer(GL_ARRAY_BUFFER, g_places);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (count * 16 * sizeof(float)), g_place_list.data(), GL_DYNAMIC_DRAW);
    for(int k = 0; k < 4; k++) {
        glEnableVertexAttribArray((GLuint) (PLACE_AT + k));
        glVertexAttribPointer((GLuint) (PLACE_AT + k), 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (const void*) (size_t) (k * 4 * sizeof(float)));
        glVertexAttribDivisor((GLuint) (PLACE_AT + k), 1);         // (the next place for each copy, not for each corner)
    }
}

// after place_many: a_place is one value for a whole thing again
static void place_no_more() {
    for(int k = 0; k < 4; k++) {
        glVertexAttribDivisor((GLuint) (PLACE_AT + k), 0);
        glDisableVertexAttribArray((GLuint) (PLACE_AT + k));
    }
}

static void use_corners(const g3_part& part) {
    glBindBuffer(GL_ARRAY_BUFFER, part.corners);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, part.triangles);
    const GLsizei step = (GLsizei) sizeof(g3_vertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, step, (const void*) offsetof(g3_vertex, x));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, step, (const void*) offsetof(g3_vertex, nx));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, step, (const void*) offsetof(g3_vertex, u));
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, step, (const void*) offsetof(g3_vertex, r));
}

// what is the same for every thing of one frame: the light, the eye, the fog
struct frame_light {
    float sun[3], sun_color[3], sky_light[3], ground_light[3], eye[3], fog_color[3], fog_from, fog_to;
    float lamp_at[16], lamp_color[12];
    bool shadows;
    float time;
    float water[4], water_shallow[3], water_deep[3];        // u_water (on, surface, caustics, haze) and its colours
};
static frame_light g_light;

// make `s` the program in use, with the frame's light in it
static void use_surface(const surface_program& s, bool shadow_pass, const mat4& view_lens, const mat4& sun_view) {
    const frame_light& l = g_light;
    glUseProgram(s.id);
    glUniformMatrix4fv(s.view_lens, 1, GL_FALSE, view_lens.m);
    glUniformMatrix4fv(s.sun_view, 1, GL_FALSE, sun_view.m);
    glUniform1i(s.tex, 0);
    glUniform1f(s.depth_only, shadow_pass ? 1.0f : 0.0f);
    glUniform1f(s.time, l.time);
    glUniform1f(s.clock, l.time);
    if(shadow_pass) {
        glUniform4f(s.tint, 1, 1, 1, 1);                // (only a texture's see-through parts matter here)
        glUniform3f(s.eye, l.eye[0], l.eye[1], l.eye[2]);
        return;
    }
    glUniform1i(s.shadow_map, 1);
    glUniform1i(s.grain, 2);
    glUniform3f(s.sun_dir, l.sun[0], l.sun[1], l.sun[2]);
    glUniform3f(s.sun_color, l.sun_color[0], l.sun_color[1], l.sun_color[2]);
    glUniform3f(s.sky_light, l.sky_light[0], l.sky_light[1], l.sky_light[2]);
    glUniform3f(s.ground_light, l.ground_light[0], l.ground_light[1], l.ground_light[2]);
    glUniform3f(s.eye, l.eye[0], l.eye[1], l.eye[2]);
    glUniform3f(s.fog_color, l.fog_color[0], l.fog_color[1], l.fog_color[2]);
    glUniform2f(s.fog, l.fog_from, l.fog_to);
    glUniform1f(s.shadows, l.shadows ? 1.0f : 0.0f);
    glUniform1f(s.shadow_step, 1.0f / SHADOW_SIZE);
    glUniform4fv(s.lamp_at, 4, l.lamp_at);
    glUniform3fv(s.lamp_color, 4, l.lamp_color);
    glUniform4fv(s.water, 1, l.water);
    glUniform3fv(s.water_shallow, 1, l.water_shallow);
    glUniform3fv(s.water_deep, 1, l.water_deep);
}

// draw the parts of every model in g_drawn. see_through: only the blended parts (false: only the solid ones)
static int draw_models(g3_scene& scene, const mat4& view_lens, const mat4& sun_view, bool shadow_pass, bool see_through) {
    const surface_program* in_use = &g_surface;
    use_surface(*in_use, shadow_pass, view_lens, sun_view);
    int triangles = 0;
    const size_t all = g_drawn.size();
    size_t next = 0;
    while(next < all) {
        const drawn_model& item = g_drawn[next];
        const double* node = item.node;
        // the ones straight after it that are the same model with the same settings: copies of one
        // scattering. They are drawn together.
        size_t copies = 1;
        while(next + copies < all && g_drawn[next + copies].model == item.model && g_drawn[next + copies].node == node) copies++;
        const drawn_model* first = &g_drawn[next];
        next += copies;

        int node_flags = (int) node[NODE_FLAGS];
        if(shadow_pass && (node_flags & NODE_NO_SHADOW)) continue;
        // the thing's own shader, when it has one (the shadow map only needs shapes: the usual one does)
        int64_t own = shadow_pass ? 0 : (int64_t) node[NODE_SHADER];
        const surface_program* wanted = own >= 1 && own <= (int64_t) g_own.size() ? &g_own[(size_t) own - 1] : &g_surface;
        if(wanted != in_use) {
            in_use = wanted;
            use_surface(*in_use, shadow_pass, view_lens, sun_view);
        }
        const surface_program& s = *in_use;
        bool together = g_many_at_once && copies > 1;
        bool settings_made = false;
        for(const g3_part& part : item.model->parts) {
            const g3_material& m = part.material;
            float tint[4];
            color_of(node[NODE_TINT], tint);
            for(int k = 0; k < 4; k++) tint[k] *= m.color[k];
            bool blended = (m.flags & G3_BLEND) != 0 || tint[3] < 0.995f;
            if(blended != see_through) continue;
            if(shadow_pass && blended) continue;

            if(!settings_made) {
                glUniform1f(s.repeat, node[NODE_REPEAT] > 0 ? node[NODE_REPEAT] : 1.0f);
                glUniform1f(s.sway, (float) node[NODE_SWAY]);
                if(!shadow_pass) glUniform1f(s.rough, (float) node[NODE_ROUGH]);
                if(together) place_many(first, copies);
                settings_made = true;
            }
            int64_t texture = node[NODE_TEXTURE] > 0 ? (int64_t) node[NODE_TEXTURE] : m.texture;
            const g3_texture* picture = texture_of(texture);
            glUniform1f(s.has_tex, picture != nullptr ? 1.0f : 0.0f);
            if(picture != nullptr) glBindTexture(GL_TEXTURE_2D, picture->id);
            glUniform1f(s.cutout, (m.flags & G3_CUTOUT) ? 1.0f : 0.0f);
            glUniform1f(s.wrap, (m.flags & G3_CUTOUT) ? 0.7f : ((m.flags & G3_TWO_SIDED) ? 0.5f : 0.0f));
            if(!shadow_pass) {
                glUniform4f(s.tint, tint[0], tint[1], tint[2], tint[3]);
                glUniform1f(s.unlit, ((m.flags & G3_UNLIT) || (node_flags & NODE_UNLIT)) ? 1.0f : 0.0f);
                glUniform1f(s.shiny, node[NODE_SHINY] >= 0 ? node[NODE_SHINY] : m.shiny);
                glUniform1f(s.glow, node[NODE_GLOW] >= 0 ? node[NODE_GLOW] : m.glow);
            }
            if(m.flags & G3_TWO_SIDED) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE);
            use_corners(part);
            if(together) {
                glDrawElementsInstanced(GL_TRIANGLES, part.count, part.number_kind, nullptr, (GLsizei) copies);
            } else {
                for(size_t c = 0; c < copies; c++) {
                    place_one(first[c].world);
                    glDrawElements(GL_TRIANGLES, part.count, part.number_kind, nullptr);
                }
            }
            triangles += part.count / 3 * (int) copies;
        }
        if(together && settings_made) place_no_more();
    }
    return triangles;
}

// is a ball round `at` (in the world), `reach` wide, anywhere in the picture? (flat: the sun's view, no perspective)
static bool in_view(const mat4& view_lens, const float at[3], float reach, bool flat) {
    const float* m = view_lens.m;
    float w = m[3] * at[0] + m[7] * at[1] + m[11] * at[2] + m[15];
    float x = m[0] * at[0] + m[4] * at[1] + m[8] * at[2] + m[12];
    float y = m[1] * at[0] + m[5] * at[1] + m[9] * at[2] + m[13];
    // how much a unit is in the picture, sideways and up, is at most the length of the matrix's first rows
    float wide = std::sqrt(m[0] * m[0] + m[4] * m[4] + m[8] * m[8]) * reach;
    float tall = std::sqrt(m[1] * m[1] + m[5] * m[5] + m[9] * m[9]) * reach;
    if(!flat && w + reach < 0) return false;                    // all of it behind the eye
    float edge = flat ? 1 : w + reach;
    return x - wide <= edge && x + wide >= -edge && y - tall <= edge && y + tall >= -edge;
}

// which models are drawn, and where: into g_drawn. for_sun: for the shadow map (everything that could
// throw a shadow into the picture, however far from the eye)
static void gather(g3_scene& scene, double time, const mat4& view_lens, bool for_sun) {
    const double* look = scene.look;
    float fog_to = look[LOOK_FOG_TO];
    float see_to = for_sun ? 1e30f : (fog_to > 0 ? fog_to : look[LOOK_FAR]);
    // (in water a thing is gone when 99 parts in 100 of it are the water's colour)
    if(!for_sun && look[LOOK_WATER_ON] > 0.5 && look[LOOK_WATER_HAZE] > 0) see_to = std::min(see_to, (float) (4.6 / look[LOOK_WATER_HAZE]));
    const bool everything = false;
    g_drawn.clear();
    scene.world.resize((size_t) scene.node_count);

    for(int i = 0; i < scene.node_count; i++) {
        const double* node = &scene.nodes[(size_t) i * NODE_SIZE];
        float t = (float) time;
        float y = node[NODE_Y] + (node[NODE_BOB] != 0 ? std::sin(t * node[NODE_BOB_RATE]) * node[NODE_BOB] : 0);
        if(node[NODE_MOTES] > 0) continue;              // motes are drawn by draw_motes, not as a model
        float x = node[NODE_X], z = node[NODE_Z], yaw = node[NODE_YAW] + node[NODE_SPIN_YAW] * t, roll = node[NODE_ROLL] + node[NODE_SPIN_ROLL] * t;
        if(node[NODE_WAG] != 0 || node[NODE_FLAP] != 0) {
            float beat = std::sin(t * node[NODE_BEAT]);
            yaw += node[NODE_WAG] * beat;
            roll += node[NODE_FLAP] * beat;
        }
        if(node[NODE_CIRCLE] > 0 && node[NODE_CIRCLE_TIME] != 0) {
            // round a circle, nose first: where it is, and the way it is going
            float angle = t * 2 * G3_PI / node[NODE_CIRCLE_TIME];
            x += std::cos(angle) * node[NODE_CIRCLE];
            z += std::sin(angle) * node[NODE_CIRCLE];
            float going = node[NODE_CIRCLE_TIME] > 0 ? 1.0f : -1.0f;
            yaw += std::atan2(-std::sin(angle) * going, -std::cos(angle) * going) * 180 / G3_PI;
        }
        mat4 local = placed(x, y, z, radians(yaw), radians(node[NODE_PITCH] + node[NODE_SPIN_PITCH] * t),
                            radians(roll), node[NODE_SX], node[NODE_SY], node[NODE_SZ]);
        int parent = (int) node[NODE_PARENT];
        mat4 world = parent >= 0 && parent < i ? times(scene.world[(size_t) parent], local) : local;
        int flags = (int) node[NODE_FLAGS];
        if(parent >= 0 && parent < i && ((int) scene.nodes[(size_t) parent * NODE_SIZE + NODE_FLAGS] & NODE_HIDDEN)) flags |= NODE_HIDDEN;

        if(flags & NODE_FACES_EYE) {
            // keep where it is and how large; turn it round the up axis to face the eye
            float dx = scene.eye[0] - world.m[12], dz = scene.eye[2] - world.m[14];
            float heading = std::atan2(dx, -dz) + G3_PI;
            float sx = std::sqrt(world.m[0] * world.m[0] + world.m[1] * world.m[1] + world.m[2] * world.m[2]);
            float sy = std::sqrt(world.m[4] * world.m[4] + world.m[5] * world.m[5] + world.m[6] * world.m[6]);
            world = placed(world.m[12], world.m[13], world.m[14], heading, 0, 0, sx, sy, sx);
        }
        scene.world[(size_t) i] = world;
        if(flags & NODE_HIDDEN) continue;

        const g3_model* model = model_of((int64_t) node[NODE_MODEL]);
        if(model == nullptr || model->parts.empty()) continue;
        float grown = std::max(std::max(std::fabs(node[NODE_SX]), std::fabs(node[NODE_SY])), std::fabs(node[NODE_SZ]));

        int64_t copies_id = (int64_t) node[NODE_COPIES];
        if(copies_id >= 1 && copies_id <= (int64_t) g_copies.size()) {
            // many copies: each at its own place (in this node's space), the far ones as the cheaper model
            const g3_copies& copies = g_copies[(size_t) copies_id - 1];
            const g3_model* cheap = model_of((int64_t) node[NODE_FAR_MODEL]);
            float cheap_from = node[NODE_FAR_FROM];
            const float flock = node[NODE_FLOCK];
            if(!everything && flock == 0) {
                // all of them further off than the eye sees: none is looked at
                float all[3];
                point_through(world, copies.middle, all);
                float ax = all[0] - scene.eye[0], ay = all[1] - scene.eye[1], az = all[2] - scene.eye[2];
                float all_reach = (copies.reach + model->reach * copies.largest) * grown;
                if(std::sqrt(ax * ax + ay * ay + az * az) - all_reach > see_to) continue;
            }
            for(size_t c = 0; c + 4 < copies.where.size(); c += 5) {
                const float* w = &copies.where[c];
                float moving[5];
                if(flock != 0) {
                    // A FLOCK: each copy goes round the thing at the distance it was set down at, nose first,
                    // the nearer ones quicker; it rises and sinks a little as it goes
                    float far = std::max(0.5f, std::sqrt(w[0] * w[0] + w[2] * w[2])), start = std::atan2(w[2], w[0]);
                    float angle = start + t * flock / far;
                    moving[0] = std::cos(angle) * far;
                    moving[1] = w[1] + std::sin(t * 0.7f + start * 3) * 0.3f;
                    moving[2] = std::sin(angle) * far;
                    float going = flock > 0 ? 1.0f : -1.0f;
                    moving[3] = std::atan2(-std::sin(angle) * going, -std::cos(angle) * going) * 180 / G3_PI;
                    moving[4] = w[4];
                    w = moving;
                }
                float at[3], here[3] = { w[0], w[1], w[2] };
                point_through(world, here, at);
                float dx = at[0] - scene.eye[0], dy = at[1] - scene.eye[1], dz = at[2] - scene.eye[2];
                float away = std::sqrt(dx * dx + dy * dy + dz * dz);
                const g3_model* which = cheap != nullptr && cheap_from > 0 && away > cheap_from ? cheap : model;
                float reach = which->reach * w[4] * grown + 0.01f;
                if(!everything && away - reach > see_to) continue;
                if(!for_sun && reach < away * 0.011f) continue;       // smaller than a few pixels: not worth drawing
                float middle[3] = { at[0], at[1] + which->middle[1] * w[4], at[2] };
                if(!in_view(view_lens, middle, reach * 1.2f, for_sun)) continue;
                drawn_model item;
                item.model = which;
                item.world = times(world, placed(w[0], w[1], w[2], radians(w[3]), 0, 0, w[4], w[4], w[4]));
                item.node = node;
                item.far_away = away;
                g_drawn.push_back(item);
            }
            continue;
        }

        float middle[3];
        point_through(world, model->middle, middle);
        float dx = middle[0] - scene.eye[0], dy = middle[1] - scene.eye[1], dz = middle[2] - scene.eye[2];
        float away = std::sqrt(dx * dx + dy * dy + dz * dz);
        // (how much the parents scale it is in the matrix: measure one of its axes)
        float scale = std::sqrt(world.m[0] * world.m[0] + world.m[1] * world.m[1] + world.m[2] * world.m[2]);
        float scale_y = std::sqrt(world.m[4] * world.m[4] + world.m[5] * world.m[5] + world.m[6] * world.m[6]);
        float scale_z = std::sqrt(world.m[8] * world.m[8] + world.m[9] * world.m[9] + world.m[10] * world.m[10]);
        float reach = model->reach * std::max(scale, std::max(scale_y, scale_z)) + 0.01f;
        if(away - reach > see_to || !in_view(view_lens, middle, reach, for_sun)) continue;
        drawn_model item;
        item.model = model;
        item.world = world;
        item.node = node;
        item.far_away = away;
        g_drawn.push_back(item);
    }
}

static void draw_sky(const g3_scene& scene, const float forward[3], const float sun[3], const float sun_color[4]) {
    const double* look = scene.look;
    // the directions the eye looks through the four corners of the picture
    float up_hint[3] = { 0, 1, 0 }, side[3], up[3];
    cross(forward, up_hint, side);
    unit(side);
    cross(side, forward, up);
    float tall = std::tan(radians(look[LOOK_FOV]) / 2), wide = tall * scene.seen_w / scene.seen_h;
    g3_vertex corner[4];
    const float cx[4] = { -1, 1, -1, 1 }, cy[4] = { -1, -1, 1, 1 };
    for(int i = 0; i < 4; i++) {
        memset(&corner[i], 0, sizeof(g3_vertex));
        corner[i].x = cx[i];
        corner[i].y = cy[i];
        corner[i].nx = forward[0] + side[0] * cx[i] * wide + up[0] * cy[i] * tall;
        corner[i].ny = forward[1] + side[1] * cx[i] * wide + up[1] * cy[i] * tall;
        corner[i].nz = forward[2] + side[2] * cx[i] * wide + up[2] * cy[i] * tall;
    }
    static GLuint buffer = 0;
    if(buffer == 0) glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corner), corner, GL_DYNAMIC_DRAW);
    const GLsizei step = (GLsizei) sizeof(g3_vertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, step, (const void*) offsetof(g3_vertex, x));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, step, (const void*) offsetof(g3_vertex, nx));
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);

    float top[4], horizon[4], below[4];
    color_of(look[LOOK_SKY_TOP], top);
    color_of(look[LOOK_SKY_HORIZON], horizon);
    color_of(look[LOOK_SKY_BELOW], below);
    glUseProgram(g_sky.id);
    glUniform3f(g_sky.top, top[0], top[1], top[2]);
    glUniform3f(g_sky.horizon, horizon[0], horizon[1], horizon[2]);
    glUniform3f(g_sky.below, below[0], below[1], below[2]);
    glUniform3f(g_sky.sun_dir, sun[0], sun[1], sun[2]);
    glUniform3f(g_sky.sun_color, sun_color[0], sun_color[1], sun_color[2]);
    glUniform3f(g_sky.eye, scene.eye[0], scene.eye[1], scene.eye[2]);
    glUniform1f(g_sky.time, g_light.time);
    glUniform4fv(g_sky.water, 1, g_light.water);
    glUniform3fv(g_sky.water_shallow, 1, g_light.water_shallow);
    glUniform3fv(g_sky.water_deep, 1, g_light.water_deep);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
}

//
// THE SHAFTS OF LIGHT under water. The surface is cut into squares of 9 metres; in about two of
// three a shaft comes down, where and how wide and long decided by the square's own numbers - so a
// shaft stays where it is in the world while the eye moves, and the same ones come back when the
// eye does. Each is one strip from the surface down the sun's slant, turned round its own length
// to face the eye, brighter and dimmer by itself, and gone into the water with distance.
//
static float cell_number(int x, int z, int which) {
    unsigned int h = (unsigned int) x * 374761393u + (unsigned int) z * 668265263u + (unsigned int) which * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (h & 0xffffff) / (float) 0x1000000;
}

static void draw_shafts(const g3_scene& scene, const mat4& view_lens, const float sun[3], const float sun_color[4]) {
    const double* look = scene.look;
    float strength = (float) look[LOOK_WATER_SHAFTS];
    if(look[LOOK_WATER_ON] < 0.5 || strength <= 0) return;
    const float surface = (float) look[LOOK_WATER_SURFACE], haze = (float) look[LOOK_WATER_HAZE];
    const float square = 9, reach = 70;
    const float down[3] = { -sun[0], -sun[1], -sun[2] };
    if(down[1] > -0.15f) return;                        // a sun at the horizon sends none down

    static std::vector<g3_vertex> strip;
    strip.clear();
    int from_x = (int) std::floor((scene.eye[0] - reach) / square), to_x = (int) std::floor((scene.eye[0] + reach) / square);
    int from_z = (int) std::floor((scene.eye[2] - reach) / square), to_z = (int) std::floor((scene.eye[2] + reach) / square);
    for(int cx = from_x; cx <= to_x; cx++) {
        for(int cz = from_z; cz <= to_z; cz++) {
            if(cell_number(cx, cz, 0) > 0.9f) continue;
            float top[3] = { (cx + cell_number(cx, cz, 1)) * square, surface, (cz + cell_number(cx, cz, 2)) * square };
            float width = 3.0f + cell_number(cx, cz, 3) * 9.0f, length = 34 + cell_number(cx, cz, 4) * 40;
            float bottom[3] = { top[0] + down[0] * length, top[1] + down[1] * length, top[2] + down[2] * length };
            float middle[3] = { (top[0] + bottom[0]) / 2, (top[1] + bottom[1]) / 2, (top[2] + bottom[2]) / 2 };
            float to_eye[3] = { scene.eye[0] - middle[0], scene.eye[1] - middle[1], scene.eye[2] - middle[2] };
            float away = std::sqrt(to_eye[0] * to_eye[0] + to_eye[1] * to_eye[1] + to_eye[2] * to_eye[2]);
            if(away > reach) continue;
            float side[3];
            cross(down, to_eye, side);
            unit(side);
            float flicker = 0.62f + 0.38f * std::sin(g_light.time * (0.35f + cell_number(cx, cz, 5) * 0.5f) + cell_number(cx, cz, 6) * 40);
            float bright = strength * 1.15f * flicker * (0.35f + 0.65f * cell_number(cx, cz, 7))
                         * std::exp(-away * haze * 0.9f) * std::min(1.0f, away / 6.0f) * (1.0f - away / reach);
            if(bright < 0.01f) continue;
            g3_vertex corner[4];
            for(int k = 0; k < 4; k++) {
                memset(&corner[k], 0, sizeof(g3_vertex));
                const float* end = k < 2 ? top : bottom;
                // (wider at the bottom: light spreads as it goes down)
                float half = (k < 2 ? width * 0.5f : width * 0.85f) * ((k & 1) ? 1.0f : -1.0f);
                corner[k].x = end[0] + side[0] * half;
                corner[k].y = end[1] + side[1] * half;
                corner[k].z = end[2] + side[2] * half;
                corner[k].u = (k & 1) ? 1.0f : 0.0f;
                corner[k].v = k < 2 ? 0.0f : 1.0f;
                corner[k].a = (unsigned char) std::min(255.0f, bright * 255);
            }
            const int order[6] = { 0, 1, 2, 2, 1, 3 };
            for(int k = 0; k < 6; k++) strip.push_back(corner[order[k]]);
        }
    }
    if(strip.empty()) return;

    static GLuint buffer = 0;
    if(buffer == 0) glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (strip.size() * sizeof(g3_vertex)), strip.data(), GL_DYNAMIC_DRAW);
    const GLsizei step = (GLsizei) sizeof(g3_vertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, step, (const void*) offsetof(g3_vertex, x));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, step, (const void*) offsetof(g3_vertex, u));
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, step, (const void*) offsetof(g3_vertex, r));
    float shallow[4];
    color_of(look[LOOK_WATER_SHALLOW], shallow);
    glUseProgram(g_shaft.id);
    glUniformMatrix4fv(g_shaft.view_lens, 1, GL_FALSE, view_lens.m);
    glUniform3f(g_shaft.color, shallow[0] * 0.55f + sun_color[0] * 0.35f, shallow[1] * 0.55f + sun_color[1] * 0.35f,
                shallow[2] * 0.55f + sun_color[2] * 0.35f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);                  // light is ADDED to what is behind it
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei) strip.size());
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

//
// THE GRAIN OF STONE (a thing's rough(...)): a picture 256 pixels a side that repeats without a seam.
// Blue is the height of the bumps - layers of noise, each twice as fine and half as strong as the
// one before, with soft ridges on them; red and green are its slopes across and down (which
// way a bump faces, for the light); alpha is slow blotches of lighter and darker stone.
//
static float grain_point(int x, int y, int period, int layer) {
    x = ((x % period) + period) % period;
    y = ((y % period) + period) % period;
    return cell_number(x + layer * 131, y + layer * 197, 11 + layer);
}

// smooth noise that repeats every `period` cells
static float grain_noise(float x, float y, int period, int layer) {
    int x0 = (int) std::floor(x), y0 = (int) std::floor(y);
    float fx = x - x0, fy = y - y0;
    fx = fx * fx * (3 - 2 * fx);
    fy = fy * fy * (3 - 2 * fy);
    float a = grain_point(x0, y0, period, layer), b = grain_point(x0 + 1, y0, period, layer);
    float c = grain_point(x0, y0 + 1, period, layer), d = grain_point(x0 + 1, y0 + 1, period, layer);
    return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy;
}

static float grain_layers(float u, float v, int first_period, int layers, int layer) {
    float sum = 0, strength = 0.5, all = 0;
    int period = first_period;
    for(int i = 0; i < layers; i++) {
        sum += strength * grain_noise(u * period, v * period, period, layer + i * 7);
        all += strength;
        strength *= 0.5;
        period *= 2;
    }
    return sum / all;
}

static float grain_height(float u, float v) {
    float body = grain_layers(u, v, 4, 6, 0);
    float ridge = 1 - std::fabs(grain_layers(u, v, 6, 5, 40) * 2 - 1);
    // (no sharp cracks: their edges showed as thin bright outlines on rock in the shade)
    return body * 0.68 + ridge * ridge * 0.32;
}

static GLuint g_grain = 0;

static void make_grain() {
    const int side = 256;
    std::vector<float> height((size_t) side * side);
    for(int y = 0; y < side; y++)
        for(int x = 0; x < side; x++)
            height[(size_t) y * side + x] = grain_height((x + 0.5f) / side, (y + 0.5f) / side);
    std::vector<unsigned char> pixels((size_t) side * side * 4);
    for(int y = 0; y < side; y++) {
        for(int x = 0; x < side; x++) {
            float left = height[(size_t) y * side + (x + side - 1) % side], right = height[(size_t) y * side + (x + 1) % side];
            float above = height[(size_t) ((y + side - 1) % side) * side + x], below = height[(size_t) ((y + 1) % side) * side + x];
            // A slope is squeezed into -1 .. 1 softly (v / (1 + |v|); the pixel code undoes it), never
            // cut off at the end of the range: a cut showed on the rock as a thin outline.
            float vx = (right - left) * side * 0.04f, vy = (below - above) * side * 0.04f;
            float sx = vx / (1 + std::fabs(vx)), sy = vy / (1 + std::fabs(vy));
            // Each number is one of only 256: rounded plainly, a gentle slope becomes a flight of
            // steps, and every step shows on shaded rock as a line - like the lines of a map. A
            // little randomness in the rounding (a different one for every pixel and number) breaks
            // the steps up into a grain too fine to see.
            unsigned char* p = &pixels[((size_t) y * side + x) * 4];
            float in[4] = { 0.5f + sx * 0.5f, 0.5f + sy * 0.5f, height[(size_t) y * side + x],
                            grain_layers((x + 0.5f) / side, (y + 0.5f) / side, 3, 5, 120) };
            for(int k = 0; k < 4; k++) {
                uint32_t h = (uint32_t) x * 374761393u + (uint32_t) y * 668265263u + (uint32_t) k * 2246822519u;
                h = (h ^ (h >> 13)) * 1274126177u;
                float chance = ((h ^ (h >> 16)) & 0xFFFF) / 65536.0f;
                p[k] = (unsigned char) std::max(0.0f, std::min(255.0f, std::floor(in[k] * 255 + chance)));
            }
        }
    }
    glGenTextures(1, &g_grain);
    glBindTexture(GL_TEXTURE_2D, g_grain);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, side, side, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

//
// THE MOTES of a scene (a thing with NODE_MOTES): so many small things adrift in a box round the
// eye. Each has a place of its own in a box `NODE_SY` metres a side, drifts up at `NODE_BOB` metres
// a second (down when negative: snow) and wanders `NODE_SZ` metres round its path; as the eye moves
// the box moves with it and a mote that leaves one side comes in at the other, so they are always
// all round and never run out. `NODE_SX` is how large one is, the thing's tint its colour and its
// glow how bright; with NODE_BUBBLES they are bubbles. Under water none is above the surface.
//
static void draw_motes(const g3_scene& scene, const mat4& view_lens, double time) {
    const double* look = scene.look;
    const float t = (float) time;
    // the directions "right" and "up" of the picture, in the world: the first two rows of the view
    const float right[3] = { scene.view.m[0], scene.view.m[4], scene.view.m[8] };
    const float up[3] = { scene.view.m[1], scene.view.m[5], scene.view.m[9] };
    static std::vector<g3_vertex> squares;

    for(int i = 0; i < scene.node_count; i++) {
        const double* node = &scene.nodes[(size_t) i * NODE_SIZE];
        int count = (int) node[NODE_MOTES];
        if(count <= 0 || ((int) node[NODE_FLAGS] & NODE_HIDDEN)) continue;
        const float size = (float) node[NODE_SX], box = std::max(1.0f, (float) node[NODE_SY]), wander = (float) node[NODE_SZ];
        const float rise = (float) node[NODE_BOB];
        const bool bubbles = ((int) node[NODE_FLAGS] & NODE_BUBBLES) != 0;
        float tint[4];
        color_of(node[NODE_TINT], tint);
        float glow = node[NODE_GLOW] >= 0 ? (float) node[NODE_GLOW] : 1.0f;
        const float haze = look[LOOK_WATER_ON] > 0.5 ? (float) look[LOOK_WATER_HAZE] : 0;

        squares.clear();
        for(int k = 0; k < count; k++) {
            float at[3] = { cell_number(k, i, 1) * box, cell_number(k, i, 2) * box, cell_number(k, i, 3) * box };
            float phase = cell_number(k, i, 4) * 40;
            at[1] += rise * t * (0.6f + 0.8f * cell_number(k, i, 5));
            at[0] += std::sin(t * 0.31f + phase) * wander;
            at[1] += std::sin(t * 0.23f + phase * 1.7f) * wander * 0.5f;
            at[2] += std::cos(t * 0.27f + phase * 0.6f) * wander;
            // into the box round the eye
            for(int a = 0; a < 3; a++) {
                float from_eye = at[a] - scene.eye[a] + box / 2;
                from_eye -= std::floor(from_eye / box) * box;
                at[a] = scene.eye[a] + from_eye - box / 2;
            }
            if(look[LOOK_WATER_ON] > 0.5 && at[1] > look[LOOK_WATER_SURFACE] - 0.3f) continue;
            float dx = at[0] - scene.eye[0], dy = at[1] - scene.eye[1], dz = at[2] - scene.eye[2];
            float away = std::sqrt(dx * dx + dy * dy + dz * dz);
            // fading out at the box's edge (no popping), into the water with distance, and right at the eye
            float shown = std::min(1.0f, (box / 2 - away) / (box * 0.12f)) * std::min(1.0f, away / 1.2f) * std::exp(-away * haze * 1.1f);
            if(shown <= 0.01f) continue;
            float twinkle = bubbles ? 1.0f : 0.55f + 0.45f * std::sin(t * (1.1f + cell_number(k, i, 6) * 2.3f) + phase);
            float bright = std::min(1.0f, shown * twinkle * glow * tint[3]);
            float large = size * (0.55f + 0.9f * cell_number(k, i, 7));
            g3_vertex corner[4];
            for(int c = 0; c < 4; c++) {
                float u = (c & 1) ? 1.0f : -1.0f, v = (c & 2) ? 1.0f : -1.0f;
                memset(&corner[c], 0, sizeof(g3_vertex));
                corner[c].x = at[0] + (right[0] * u + up[0] * v) * large;
                corner[c].y = at[1] + (right[1] * u + up[1] * v) * large;
                corner[c].z = at[2] + (right[2] * u + up[2] * v) * large;
                corner[c].u = u;
                corner[c].v = v;
                corner[c].r = (unsigned char) (tint[0] * 255);
                corner[c].g = (unsigned char) (tint[1] * 255);
                corner[c].b = (unsigned char) (tint[2] * 255);
                corner[c].a = (unsigned char) (bright * 255);
            }
            const int order[6] = { 0, 1, 2, 2, 1, 3 };
            for(int c = 0; c < 6; c++) squares.push_back(corner[order[c]]);
        }
        if(squares.empty()) continue;

        static GLuint buffer = 0;
        if(buffer == 0) glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (squares.size() * sizeof(g3_vertex)), squares.data(), GL_DYNAMIC_DRAW);
        const GLsizei step = (GLsizei) sizeof(g3_vertex);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, step, (const void*) offsetof(g3_vertex, x));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, step, (const void*) offsetof(g3_vertex, u));
        glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, step, (const void*) offsetof(g3_vertex, r));
        glUseProgram(g_mote.id);
        glUniformMatrix4fv(g_mote.view_lens, 1, GL_FALSE, view_lens.m);
        glUniform1f(g_mote.bubble, bubbles ? 1.0f : 0.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);              // light, added to what is behind it
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei) squares.size());
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

// the camera of a scene, for a picture `aspect` times as wide as it is high
static void camera_of(g3_scene& scene, float aspect, float forward[3]) {
    const double* look = scene.look;
    scene.eye[0] = look[LOOK_EYE_X];
    scene.eye[1] = look[LOOK_EYE_Y];
    scene.eye[2] = look[LOOK_EYE_Z];
    if(look[LOOK_AT_ON] > 0.5f) {
        forward[0] = look[LOOK_AT_X] - scene.eye[0];
        forward[1] = look[LOOK_AT_Y] - scene.eye[1];
        forward[2] = look[LOOK_AT_Z] - scene.eye[2];
        if(std::fabs(forward[0]) + std::fabs(forward[1]) + std::fabs(forward[2]) < 1e-6f) forward[2] = -1;
    } else {
        float heading = radians(look[LOOK_HEADING]), down = radians(look[LOOK_TILT]);
        forward[0] = std::sin(heading) * std::cos(down);
        forward[1] = -std::sin(down);
        forward[2] = -std::cos(heading) * std::cos(down);
    }
    unit(forward);
    // looking straight up or down, "up" cannot be the y axis
    float up_hint[3] = { 0, 1, 0 };
    if(std::fabs(forward[1]) > 0.999f) { up_hint[1] = 0; up_hint[2] = forward[1] > 0 ? 1.0f : -1.0f; }
    scene.view = looking(scene.eye, forward, up_hint);
    float near_z = look[LOOK_NEAR] > 0 ? look[LOOK_NEAR] : 0.1f;
    float far_z = look[LOOK_FAR] > near_z ? look[LOOK_FAR] : 1000;
    scene.lens = perspective(radians(look[LOOK_FOV] > 1 ? look[LOOK_FOV] : 60), aspect, near_z, far_z);
}

//
// READING THE PICTURE BACK. Asked for its pixels, the graphics card must first finish everything
// it was told to draw: the program stands still until it has. A scene that is drawn again and
// again need not wait: it asks for this frame's pixels to be put aside in a buffer on the card
// (which returns at once), and takes the pixels that were put aside the frame BEFORE, which are
// ready by now. What is shown is then one frame behind what was drawn - a sixtieth of a second -
// and the card is never made to wait.
//
// That is only right while the same scene is drawn frame after frame. A scene drawn once (a
// still picture), the first frame, another scene, another size: the pixels are read at once, as
// before, so that what is shown is always what was asked for.
//
#ifndef GL_PIXEL_PACK_BUFFER
#define GL_PIXEL_PACK_BUFFER 0x88EB
#endif
#ifndef GL_STREAM_READ
#define GL_STREAM_READ 0x88E1
#endif
#ifndef GL_MAP_READ_BIT
#define GL_MAP_READ_BIT 0x0001
#endif

static void read_picture(g3_target& t, int64_t scene_id, int width, int height) {
    static GLuint aside[2] = { 0, 0 };
    static bool filled[2] = { false, false };
    static int turn = 0, aside_w = 0, aside_h = 0;
    static int64_t last_scene = 0;
    static Uint32 last_at = 0;

    const size_t size = (size_t) width * height * 4;
    Uint32 now = SDL_GetTicks();
    bool can = !g3_es && glMapBufferRange != nullptr && glUnmapBuffer != nullptr;
    // (frame after frame: the same scene, the same size, drawn within the last tenth of a second)
    bool running = can && last_scene == scene_id && aside_w == width && aside_h == height && now - last_at < 100;
    last_scene = scene_id;
    last_at = now;

    if(can && (aside[0] == 0 || aside_w != width || aside_h != height)) {
        if(aside[0] != 0) glDeleteBuffers(2, aside);
        glGenBuffers(2, aside);
        for(int i = 0; i < 2; i++) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, aside[i]);
            glBufferData(GL_PIXEL_PACK_BUFFER, (GLsizeiptr) size, nullptr, GL_STREAM_READ);
            filled[i] = false;
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        aside_w = width;
        aside_h = height;
    }
    if(!can) {
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, t.pixels.data());
        return;
    }

    // this frame's pixels are put aside ...
    glBindBuffer(GL_PIXEL_PACK_BUFFER, aside[turn]);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    filled[turn] = true;
    int before = 1 - turn;
    bool taken = false;
    if(running && filled[before]) {
        // ... and the frame before is taken: it is ready
        glBindBuffer(GL_PIXEL_PACK_BUFFER, aside[before]);
        void* pixels = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, (GLsizeiptr) size, GL_MAP_READ_BIT);
        if(pixels != nullptr) {
            memcpy(t.pixels.data(), pixels, size);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            taken = true;
        }
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    if(!taken) {
        // not running, or nothing put aside yet: this frame itself, at once
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, t.pixels.data());
        filled[before] = false;
    }
    turn = before;
}

//
// g3_scene_draw(scene, x, y, w, h, time, opacity): draw the scene as it is at `time` seconds, and
// paint the picture on the area (x, y, w, h) of the window (in the program's units).
//
void sharpgui_g3_scene_draw(sni_env* env, int64_t scene_id, double x, double y, double w, double h, double time, int32_t opacity) {
    g3_scene* found = scene_of(scene_id);
    if(found == nullptr || w < 1 || h < 1) return;
    if(!g3_begin(env)) return;
    g3_scene& scene = *found;
    const double* look = scene.look;

    int width = std::max(1, (int) (w * g_scale + 0.5)), height = std::max(1, (int) (h * g_scale + 0.5));
    GLint largest = 4096;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &largest);
    width = std::min(width, (int) largest);
    height = std::min(height, (int) largest);
    if(!make_target(width, height)) {
        env->raise("std#illegal_state_exception", "the graphics card gave no picture to draw the scene into");
        g3_end();
        return;
    }
    g3_target& target = g_target;
    scene.seen_w = (float) width;
    scene.seen_h = (float) height;

    float forward[3];
    camera_of(scene, (float) width / height, forward);
    mat4 view_lens = times(scene.lens, scene.view);

    float sun[3] = { look[LOOK_SUN_X], look[LOOK_SUN_Y], look[LOOK_SUN_Z] };
    unit(sun);
    float sun_color[4], sky_light[4], ground_light[4], fog_color[4];
    color_of(look[LOOK_SUN_COLOR], sun_color);
    for(int k = 0; k < 3; k++) sun_color[k] *= look[LOOK_SUN_STRENGTH];
    color_of(look[LOOK_SKY_LIGHT], sky_light);
    color_of(look[LOOK_GROUND_LIGHT], ground_light);
    color_of(look[LOOK_FOG_COLOR], fog_color);

    const surface_program& s = g_surface;
    for(int i = 0; i < 4; i++) glEnableVertexAttribArray((GLuint) i);
    glActiveTexture(GL_TEXTURE0);

    g_light.time = (float) time;
    for(int k = 0; k < 3; k++) g_light.eye[k] = scene.eye[k];
    {
        float shallow[4], deep[4];
        color_of(look[LOOK_WATER_SHALLOW], shallow);
        color_of(look[LOOK_WATER_DEEP], deep);
        g_light.water[0] = look[LOOK_WATER_ON] > 0.5 ? 1.0f : 0.0f;
        g_light.water[1] = (float) look[LOOK_WATER_SURFACE];
        g_light.water[2] = (float) look[LOOK_WATER_CAUSTICS];
        g_light.water[3] = (float) look[LOOK_WATER_HAZE];
        for(int k = 0; k < 3; k++) { g_light.water_shallow[k] = shallow[k]; g_light.water_deep[k] = deep[k]; }
    }

    // ---- 1. the shadow map: the world's depth as the sun sees it, round where the eye is looking
    bool shadows = look[LOOK_SHADOWS] > 0.5f && make_shadow_map();
    mat4 sun_view = identity();
    if(shadows) {
        float reach = look[LOOK_SHADOW_REACH] > 1 ? look[LOOK_SHADOW_REACH] : 60;
        float middle[3] = { scene.eye[0] + forward[0] * reach * 0.6f, scene.eye[1] + forward[1] * reach * 0.6f,
                            scene.eye[2] + forward[2] * reach * 0.6f };
        float from[3] = { middle[0] + sun[0] * reach * 3, middle[1] + sun[1] * reach * 3, middle[2] + sun[2] * reach * 3 };
        float towards[3] = { -sun[0], -sun[1], -sun[2] };
        float up_hint[3] = { 0, 1, 0 };
        if(std::fabs(sun[1]) > 0.99f) { up_hint[1] = 0; up_hint[2] = 1; }
        sun_view = times(flat_view(reach, 0.5f, reach * 6), looking(from, towards, up_hint));

        gather(scene, time, sun_view, true);
        glBindFramebuffer(GL_FRAMEBUFFER, g_shadow_frame);
        glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
        glDisable(GL_SCISSOR_TEST);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_TRUE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glClear(GL_DEPTH_BUFFER_BIT);
        glDisable(GL_BLEND);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);
        draw_models(scene, sun_view, sun_view, true, false);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    // ---- 2. the picture
    gather(scene, time, view_lens, false);
    glBindFramebuffer(GL_FRAMEBUFFER, target.smooth_frame != 0 ? target.smooth_frame : target.frame);
    glViewport(0, 0, width, height);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    float background[4];
    color_of(look[LOOK_BACKGROUND], background);
    glClearColor(background[0], background[1], background[2], 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(look[LOOK_SKY_ON] > 0.5f || look[LOOK_WATER_ON] > 0.5) draw_sky(scene, forward, sun, sun_color);

    if(shadows) {
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, g_shadow_map);
        glActiveTexture(GL_TEXTURE0);
    }
    if(g_grain == 0) make_grain();
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, g_grain);
    glActiveTexture(GL_TEXTURE0);

    // the lamps: the four nearest the eye
    float lamp_at[16] = { 0 }, lamp_color[12] = { 0 };
    {
        struct lit { float away; float at[3]; float color[3]; float reach; };
        std::vector<lit> lamps;
        for(int i = 0; i < scene.lamp_count; i++) {
            const double* l = &scene.lamps[(size_t) i * LAMP_SIZE];
            lit one;
            float here[3] = { l[0], l[1], l[2] };
            int on = (int) l[6];
            if(on >= 0 && on < scene.node_count) point_through(scene.world[(size_t) on], here, one.at);
            else memcpy(one.at, here, sizeof(here));
            float color[4];
            color_of(l[3], color);
            for(int k = 0; k < 3; k++) one.color[k] = color[k] * l[5];
            one.reach = l[4];
            float dx = one.at[0] - scene.eye[0], dy = one.at[1] - scene.eye[1], dz = one.at[2] - scene.eye[2];
            one.away = dx * dx + dy * dy + dz * dz;
            lamps.push_back(one);
        }
        std::sort(lamps.begin(), lamps.end(), [](const lit& a, const lit& b) { return a.away < b.away; });
        for(size_t i = 0; i < lamps.size() && i < 4; i++) {
            memcpy(&lamp_at[i * 4], lamps[i].at, 3 * sizeof(float));
            lamp_at[i * 4 + 3] = lamps[i].reach;
            memcpy(&lamp_color[i * 3], lamps[i].color, 3 * sizeof(float));
        }
    }
    // what every program drawing this frame is handed (use_surface)
    for(int k = 0; k < 3; k++) {
        g_light.sun[k] = sun[k];
        g_light.sun_color[k] = sun_color[k];
        g_light.sky_light[k] = sky_light[k];
        g_light.ground_light[k] = ground_light[k];
        g_light.eye[k] = scene.eye[k];
        g_light.fog_color[k] = fog_color[k];
    }
    g_light.fog_from = (float) look[LOOK_FOG_FROM];
    g_light.fog_to = (float) look[LOOK_FOG_TO];
    g_light.shadows = shadows;
    memcpy(g_light.lamp_at, lamp_at, sizeof(lamp_at));
    memcpy(g_light.lamp_color, lamp_color, sizeof(lamp_color));

    int triangles = draw_models(scene, view_lens, sun_view, false, false);       // everything solid ...
    draw_shafts(scene, view_lens, sun, sun_color);                                // ... the light in the water ...
    draw_motes(scene, view_lens, time);                                           // ... what drifts in it ...
    for(int i = 0; i < 4; i++) glEnableVertexAttribArray((GLuint) i);
    // ... then what is see-through, the furthest first, without hiding what comes after it
    std::sort(g_drawn.begin(), g_drawn.end(), [](const drawn_model& a, const drawn_model& b) { return a.far_away > b.far_away; });
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    triangles += draw_models(scene, view_lens, sun_view, false, true);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    scene.drawn_triangles = triangles;
    scene.drawn_models = (int) g_drawn.size();

    // ---- 3. read the picture back
    if(target.smooth_frame != 0) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, target.smooth_frame);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.frame);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    if(look[LOOK_BLOOM] > 0) add_bloom(target, (float) look[LOOK_BLOOM]);
    glBindFramebuffer(GL_FRAMEBUFFER, target.frame);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    read_picture(target, scene_id, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    g3_end();

    // ---- 4. hand it to the 2D side (OpenGL's first row is the bottom one: it is shown flipped)
    int shown_w = 0, shown_h = 0;
    if(target.shown != nullptr) SDL_QueryTexture(target.shown, nullptr, nullptr, &shown_w, &shown_h);
    if(target.shown == nullptr || shown_w != width || shown_h != height) {
        if(target.shown != nullptr) SDL_DestroyTexture(target.shown);
        target.shown = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
        if(target.shown == nullptr) return;
    }
    SDL_UpdateTexture(target.shown, nullptr, target.pixels.data(), width * 4);
    SDL_SetTextureBlendMode(target.shown, opacity < 255 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    SDL_SetTextureAlphaMod(target.shown, (Uint8) (opacity < 0 ? 0 : opacity > 255 ? 255 : opacity));
    SDL_FRect to = { (float) x, (float) y, (float) w, (float) h };
    SDL_RenderCopyExF(g_renderer, target.shown, nullptr, &to, 0, nullptr, SDL_FLIP_VERTICAL);
}

// the 2D side's pictures end with the window (sharpgui.cpp calls this when it closes)
void g3_forget_window() {
    if(g_target.shown != nullptr) {
        SDL_DestroyTexture(g_target.shown);
        g_target.shown = nullptr;
    }
}

// g3_scene_drawn(scene): how many triangles the last draw sent to the graphics card
int64_t sharpgui_g3_scene_drawn(sni_env* env, int64_t scene_id) {
    g3_scene* scene = scene_of(scene_id);
    return scene == nullptr ? 0 : scene->drawn_triangles;
}

// g3_card(): what draws the scenes - the graphics card and its OpenGL - or why 3D is not available
sni_ref sharpgui_g3_card(sni_env* env) {
    if(g3_context == nullptr && !g3_failed && g3_begin(env)) g3_end();
    return env->new_text(g3_failed ? "3D is not available: " + g3_why : g3_card);
}

// =================================================================================
// 8. What is under a point of the picture (clicks)
// =================================================================================
//
// g3_scene_pick(scene, px, py, w, h, hit): which thing is under the point (px, py) of a scene shown
// w x h (as it was last drawn)? Returns the node's number, -1 for none; `hit` (three numbers) gets
// the point of the world that was hit. A thing is hit when the line from the eye through the point
// goes through the box round its model.
//
int64_t sharpgui_g3_scene_pick(sni_env* env, int64_t scene_id, double px, double py, double w, double h, sni_ref hit_ref) {
    g3_scene* found = scene_of(scene_id);
    sni_array hit;
    if(found == nullptr || w <= 0 || h <= 0) return -1;
    if(!gui_numbers(env, hit_ref, SNI_DECIMAL, 3, "the list for the point that was hit", &hit)) return -1;
    g3_scene& scene = *found;
    if((int) scene.world.size() < scene.node_count) return -1;         // never drawn

    // the line from the eye through that point of the picture
    float forward[3];
    camera_of(scene, (float) (w / h), forward);
    float up_hint[3] = { 0, 1, 0 }, side[3], up[3];
    if(std::fabs(forward[1]) > 0.999f) { up_hint[1] = 0; up_hint[2] = forward[1] > 0 ? 1.0f : -1.0f; }
    cross(forward, up_hint, side);
    unit(side);
    cross(side, forward, up);
    float tall = std::tan(radians(scene.look[LOOK_FOV] > 1 ? scene.look[LOOK_FOV] : 60) / 2), wide = tall * (float) (w / h);
    float across = (float) (px / w * 2 - 1) * wide, upward = (float) (1 - py / h * 2) * tall;
    float ray[3] = { forward[0] + side[0] * across + up[0] * upward, forward[1] + side[1] * across + up[1] * upward,
                     forward[2] + side[2] * across + up[2] * upward };
    unit(ray);

    int best = -1;
    float nearest = 1e30f;
    for(int i = 0; i < scene.node_count; i++) {
        const double* node = &scene.nodes[(size_t) i * NODE_SIZE];
        int flags = (int) node[NODE_FLAGS];
        if((flags & (NODE_HIDDEN | NODE_NOT_CLICKED)) || node[NODE_COPIES] > 0) continue;
        const g3_model* model = model_of((int64_t) node[NODE_MODEL]);
        if(model == nullptr || !model->any) continue;
        mat4 back;
        if(!undone(scene.world[(size_t) i], back)) continue;
        // the line, in the model's own space, against the model's box
        float from[3], along[3], end[3] = { scene.eye[0] + ray[0], scene.eye[1] + ray[1], scene.eye[2] + ray[2] }, there[3];
        point_through(back, scene.eye, from);
        point_through(back, end, there);
        for(int k = 0; k < 3; k++) along[k] = there[k] - from[k];
        float enter = 0, leave = 1e30f;
        bool missed = false;
        for(int k = 0; k < 3 && !missed; k++) {
            float low = model->low[k] - 0.001f, high = model->high[k] + 0.001f;
            if(std::fabs(along[k]) < 1e-9f) {
                if(from[k] < low || from[k] > high) missed = true;
            } else {
                float a = (low - from[k]) / along[k], b = (high - from[k]) / along[k];
                if(a > b) std::swap(a, b);
                enter = std::max(enter, a);
                leave = std::min(leave, b);
                if(enter > leave) missed = true;
            }
        }
        // (`enter` is in steps of `along`, which is one unit of the WORLD long: so it is the distance)
        if(!missed && enter < nearest) { nearest = enter; best = i; }
    }
    if(best >= 0) {
        double* at = (double*) hit.data;
        for(int k = 0; k < 3; k++) at[k] = scene.eye[k] + ray[k] * nearest;
    }
    return best;
}

// has the texture pixels that are see-through? (g3_load.cpp asks, for leaves and fences)
bool g3_texture_see_through(int64_t texture) {
    const g3_texture* found = texture_of(texture);
    return found != nullptr && found->see_through;
}

// a texture already made under this name (0: none): g3_make.cpp asks before drawing one again
int64_t g3_texture_known(const std::string& key) {
    auto known = g_texture_ids.find(key);
    return known == g_texture_ids.end() ? 0 : known->second;
}

// a texture from pixels the library made itself: width x height, four bytes each (red green blue alpha)
int64_t g3_texture_pixels(const std::string& key, const unsigned char* pixels, int width, int height) {
    return texture_from_pixels(key, pixels, width, height);
}

//
// g3_shader(code): a program's own last word on a pixel's colour. `code` is a function in the
// graphics card's shading language (GLSL):
//
//      vec3 look(vec3 color, vec3 normal, vec3 place, vec2 at, float time) { ... return color; }
//
//      color       what the pixel would be: its texture and colour, lit and shadowed
//      normal      which way the surface faces there (length 1)
//      place       where in the world the pixel is
//      at          its point of the texture
//      time        seconds since the window opened
//
// What it returns is the colour (fog is put over it afterwards). Returns the shader's id, for a
// thing's .shader(...); the same code gives the same id. A mistake in the code raises an exception
// with what the graphics card said about it.
//
static std::map<std::string, int64_t> g_own_ids;

int64_t sharpgui_g3_shader(sni_env* env, sni_ref code_ref) {
    std::string code = env->text(code_ref);
    auto known = g_own_ids.find(code);
    if(known != g_own_ids.end()) return known->second;
    if(!g3_begin(env)) return 0;
    surface_program made;
    std::string wrong;
    int64_t id = 0;
    if(surface_with(code.c_str(), made, wrong)) {
        g_own.push_back(made);
        id = (int64_t) g_own.size();
        g_own_ids[code] = id;
    } else {
        env->raise("std#illegal_argument_exception", "the shader was not accepted - " + wrong);
    }
    g3_end();
    return id;
}
