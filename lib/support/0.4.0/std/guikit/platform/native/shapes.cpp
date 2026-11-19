//
// SHAPES  -  rectangles (square or round cornered), lines and circles.
//
// SDL2 itself only draws square rectangles and one-pixel lines. Everything
// else here is built from TRIANGLES handed to the graphics card
// (SDL_RenderGeometry), with one trick for smooth edges:
//
//      A shape is an OUTLINE: a ring of points, each with the direction that
//      points straight out of the shape there (its "normal"). We paint the
//      inside solid, and around it a one-pixel-wide band that fades from the
//      colour to nothing. That fading band is what makes a circle look round
//      instead of like a staircase (anti-aliasing).
//
// A circle and a round-cornered rectangle are both just outlines, so the same
// two functions (fill_outline, stroke_outline) draw all of them.
//
// SDL_RenderGeometry arrived in SDL 2.0.18. On an older SDL (an old Raspberry
// Pi OS) the same shapes are painted row by row instead - correct, only without
// the smooth edge.
//
#include "gui_internal.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#if SDL_VERSION_ATLEAST(2, 0, 18) && !defined(GUI_NO_GEOMETRY)
#define GUI_HAS_GEOMETRY 1
#else
#define GUI_HAS_GEOMETRY 0
#endif

static const float PI = 3.14159265358979f;

// One point of an outline: where it is, and which way is "out" there.
struct outline_point {
    float x, y;         // the point
    float nx, ny;       // the way out of the shape (length 1)
};

//
// The outline of a rectangle with round corners (radius 0: square corners).
// Goes around clockwise: top-right corner, bottom-right, bottom-left, top-left.
// Each corner is a quarter circle made of short straight pieces.
//
static void rounded_rect_outline(std::vector<outline_point>& out, float x, float y, float w, float h, float radius) {
    float r = std::max(0.0f, std::min(radius, std::min(w, h) / 2));
    int pieces = std::max(3, std::min(24, (int) std::ceil(r * 0.5f) + 2));

    // the centre of each corner's quarter circle, and the angle it starts at
    float centre_x[4] = { x + w - r, x + w - r, x + r,     x + r };
    float centre_y[4] = { y + r,     y + h - r, y + h - r, y + r };
    float start[4]    = { -PI / 2,   0,         PI / 2,    PI };

    for(int corner = 0; corner < 4; corner++) {
        for(int i = 0; i <= pieces; i++) {
            float angle = start[corner] + (PI / 2) * i / pieces;
            float nx = std::cos(angle), ny = std::sin(angle);
            out.push_back({ centre_x[corner] + nx * r, centre_y[corner] + ny * r, nx, ny });
        }
    }
}

// The outline of a circle: points evenly around it, more of them when it is big.
static void circle_outline(std::vector<outline_point>& out, float cx, float cy, float r) {
    int pieces = std::max(12, std::min(256, (int) std::ceil(2 * PI * r / 2.5f)));
    for(int i = 0; i < pieces; i++) {
        float angle = 2 * PI * i / pieces;
        float nx = std::cos(angle), ny = std::sin(angle);
        out.push_back({ cx + nx * r, cy + ny * r, nx, ny });
    }
}

#if GUI_HAS_GEOMETRY

static SDL_Vertex vertex(float x, float y, SDL_Color c, Uint8 alpha) {
    SDL_Vertex v;
    v.position.x = x;
    v.position.y = y;
    v.color.r = c.r;
    v.color.g = c.g;
    v.color.b = c.b;
    v.color.a = alpha;
    v.tex_coord.x = 0;
    v.tex_coord.y = 0;
    return v;
}

//
// A band between two rings of the same outline: ring A is every point moved
// `a` pixels along its normal with alpha `alpha_a`, ring B the same with `b`
// and `alpha_b`. (A solid band has both alphas equal; a fading edge has one 0.)
//
static void add_band(std::vector<SDL_Vertex>& verts, std::vector<int>& indices,
                     const std::vector<outline_point>& points, SDL_Color c,
                     float a, Uint8 alpha_a, float b, Uint8 alpha_b) {
    int first = (int) verts.size();
    int count = (int) points.size();
    for(const outline_point& p : points) {
        verts.push_back(vertex(p.x + p.nx * a, p.y + p.ny * a, c, alpha_a));
        verts.push_back(vertex(p.x + p.nx * b, p.y + p.ny * b, c, alpha_b));
    }
    for(int i = 0; i < count; i++) {
        int next = (i + 1) % count;
        int a0 = first + i * 2, b0 = a0 + 1, a1 = first + next * 2, b1 = a1 + 1;
        indices.push_back(a0); indices.push_back(b0); indices.push_back(a1);
        indices.push_back(a1); indices.push_back(b0); indices.push_back(b1);
    }
}

// Paint the inside of an outline solid, with a soft one-pixel edge.
static void fill_outline(const std::vector<outline_point>& points, SDL_Color c) {
    if(points.size() < 3) return;
    std::vector<SDL_Vertex> verts;
    std::vector<int> indices;

    // the solid inside: a fan of triangles from the middle to the ring half a pixel in
    float mx = 0, my = 0;
    for(const outline_point& p : points) { mx += p.x; my += p.y; }
    mx /= points.size();
    my /= points.size();

    int count = (int) points.size();
    verts.push_back(vertex(mx, my, c, c.a));
    for(const outline_point& p : points)
        verts.push_back(vertex(p.x - p.nx * 0.5f, p.y - p.ny * 0.5f, c, c.a));
    for(int i = 0; i < count; i++) {
        indices.push_back(0);
        indices.push_back(1 + i);
        indices.push_back(1 + (i + 1) % count);
    }

    // the soft edge: from half a pixel in (solid) to half a pixel out (nothing)
    add_band(verts, indices, points, c, -0.5f, c.a, 0.5f, 0);

    SDL_RenderGeometry(g_renderer, nullptr, verts.data(), (int) verts.size(), indices.data(), (int) indices.size());
}

// Paint only the line of an outline, `thickness` pixels wide, soft on both sides.
static void stroke_outline(const std::vector<outline_point>& points, float thickness, SDL_Color c) {
    if(points.size() < 3 || thickness <= 0) return;
    std::vector<SDL_Vertex> verts;
    std::vector<int> indices;

    // a line thinner than one pixel is drawn one pixel wide but fainter
    Uint8 alpha = thickness >= 1 ? c.a : (Uint8) (c.a * thickness);
    float half = std::max(thickness, 1.0f) / 2;

    add_band(verts, indices, points, c, half + 0.5f, 0,     half - 0.5f, alpha);     // outer soft edge
    add_band(verts, indices, points, c, half - 0.5f, alpha, -half + 0.5f, alpha);    // the solid line
    add_band(verts, indices, points, c, -half + 0.5f, alpha, -half - 0.5f, 0);       // inner soft edge

    SDL_RenderGeometry(g_renderer, nullptr, verts.data(), (int) verts.size(), indices.data(), (int) indices.size());
}

#else   // an SDL older than 2.0.18: no triangles, paint row by row

//
// How far a round-cornered rectangle is cut in at each side on pixel row `row`
// (0 at the top). Rows next to a corner are cut by the quarter circle; rows in
// the straight part are not cut at all.
//
static float corner_inset(float row, float h, float r) {
    float dy = 0;
    if(row < r) dy = r - row;
    else if(row > h - r) dy = row - (h - r);
    else return 0;
    return r - std::sqrt(std::max(0.0f, r * r - dy * dy));
}

// A filled round-cornered rectangle, one horizontal line per pixel row.
static void fill_rows(float x, float y, float w, float h, float radius, SDL_Color c) {
    float r = std::max(0.0f, std::min(radius, std::min(w, h) / 2));
    SDL_SetRenderDrawColor(g_renderer, c.r, c.g, c.b, c.a);
    for(int row = 0; row < (int) h; row++) {
        float inset = corner_inset(row + 0.5f, h, r);
        SDL_RenderDrawLineF(g_renderer, x + inset, y + row, x + w - inset - 1, y + row);
    }
}

// The line of a round-cornered rectangle: on each row, the outer shape minus the inner one.
static void stroke_rows(float x, float y, float w, float h, float radius, float t, SDL_Color c) {
    float r = std::max(0.0f, std::min(radius, std::min(w, h) / 2));
    float inner_r = std::max(0.0f, r - t);
    float inner_h = h - 2 * t;
    SDL_SetRenderDrawColor(g_renderer, c.r, c.g, c.b, c.a);
    for(int row = 0; row < (int) h; row++) {
        float outer = corner_inset(row + 0.5f, h, r);
        float inner_row = row + 0.5f - t;
        if(inner_row < 0 || inner_row > inner_h || inner_h <= 0) {
            SDL_RenderDrawLineF(g_renderer, x + outer, y + row, x + w - outer - 1, y + row);     // top and bottom edges
        } else {
            float inner = t + corner_inset(inner_row, inner_h, inner_r);
            SDL_RenderDrawLineF(g_renderer, x + outer, y + row, x + inner - 1, y + row);           // left edge
            SDL_RenderDrawLineF(g_renderer, x + w - inner, y + row, x + w - outer - 1, y + row);   // right edge
        }
    }
}

#endif

//
// draw_rect(x, y, w, h, radius, colour): a filled rectangle. radius 0 is a
// plain square-cornered one (painted directly, pixel-exact); more rounds the
// corners.
//
void sharpgui_draw_rect(sni_env* env, double x, double y, double w, double h, double radius, int64_t color) {
    if(!gui_ready(env) || w <= 0 || h <= 0) return;
    SDL_Color c = gui_color(color);

    if(radius <= 0) {
        SDL_FRect area = { (float) x, (float) y, (float) w, (float) h };
        SDL_SetRenderDrawColor(g_renderer, c.r, c.g, c.b, c.a);
        SDL_RenderFillRectF(g_renderer, &area);
        return;
    }

#if GUI_HAS_GEOMETRY
    std::vector<outline_point> points;
    rounded_rect_outline(points, (float) x, (float) y, (float) w, (float) h, (float) radius);
    fill_outline(points, c);
#else
    fill_rows((float) x, (float) y, (float) w, (float) h, (float) radius, c);
#endif
}

//
// draw_rect_line(x, y, w, h, radius, thickness, colour): only the border of a
// rectangle, drawn INSIDE the given area (so a border never grows the box).
//
void sharpgui_draw_rect_line(sni_env* env, double x, double y, double w, double h, double radius, double thickness, int64_t color) {
    if(!gui_ready(env) || w <= 0 || h <= 0 || thickness <= 0) return;
    SDL_Color c = gui_color(color);
    float t = (float) std::min(thickness, std::min(w, h) / 2);

    if(radius <= 0) {
        // four plain bars: top, bottom, left, right
        SDL_FRect bars[4] = {
            { (float) x,             (float) y,             (float) w, t },
            { (float) x,             (float) (y + h - t),   (float) w, t },
            { (float) x,             (float) y + t,         t,         (float) h - 2 * t },
            { (float) (x + w - t),   (float) y + t,         t,         (float) h - 2 * t }
        };
        SDL_SetRenderDrawColor(g_renderer, c.r, c.g, c.b, c.a);
        SDL_RenderFillRectsF(g_renderer, bars, 4);
        return;
    }

#if GUI_HAS_GEOMETRY
    // the outline that runs down the MIDDLE of the border
    std::vector<outline_point> points;
    rounded_rect_outline(points, (float) x + t / 2, (float) y + t / 2, (float) w - t, (float) h - t, (float) radius - t / 2);
    stroke_outline(points, t, c);
#else
    stroke_rows((float) x, (float) y, (float) w, (float) h, (float) radius, t, c);
#endif
}

//
// draw_line(x1, y1, x2, y2, thickness, colour): a straight line between two
// points. It is really a long thin four-cornered shape, so it can be any
// thickness and stays smooth at any angle.
//
void sharpgui_draw_line(sni_env* env, double x1, double y1, double x2, double y2, double thickness, int64_t color) {
    if(!gui_ready(env) || thickness <= 0) return;
    SDL_Color c = gui_color(color);

    float dx = (float) (x2 - x1), dy = (float) (y2 - y1);
    float length = std::sqrt(dx * dx + dy * dy);
    if(length < 0.0001f) return;

#if GUI_HAS_GEOMETRY
    float nx = -dy / length, ny = dx / length;          // straight out of the line's side
    float half = (float) thickness / 2;

    std::vector<outline_point> points;
    points.push_back({ (float) x1 + nx * half, (float) y1 + ny * half,  nx,  ny });
    points.push_back({ (float) x2 + nx * half, (float) y2 + ny * half,  nx,  ny });
    points.push_back({ (float) x2 - nx * half, (float) y2 - ny * half, -nx, -ny });
    points.push_back({ (float) x1 - nx * half, (float) y1 - ny * half, -nx, -ny });
    fill_outline(points, c);
#else
    SDL_SetRenderDrawColor(g_renderer, c.r, c.g, c.b, c.a);
    SDL_RenderDrawLineF(g_renderer, (float) x1, (float) y1, (float) x2, (float) y2);
#endif
}

// draw_circle(cx, cy, radius, colour): a filled circle around a centre point.
void sharpgui_draw_circle(sni_env* env, double cx, double cy, double radius, int64_t color) {
    if(!gui_ready(env) || radius <= 0) return;
    SDL_Color c = gui_color(color);

#if GUI_HAS_GEOMETRY
    std::vector<outline_point> points;
    circle_outline(points, (float) cx, (float) cy, (float) radius);
    fill_outline(points, c);
#else
    // a circle is a rectangle whose corners are rounded all the way
    fill_rows((float) (cx - radius), (float) (cy - radius), (float) radius * 2, (float) radius * 2, (float) radius, c);
#endif
}

// draw_circle_line(cx, cy, radius, thickness, colour): only the ring of a circle, inside the radius.
void sharpgui_draw_circle_line(sni_env* env, double cx, double cy, double radius, double thickness, int64_t color) {
    if(!gui_ready(env) || radius <= 0 || thickness <= 0) return;
    SDL_Color c = gui_color(color);
    float t = (float) std::min(thickness, radius);

#if GUI_HAS_GEOMETRY
    std::vector<outline_point> points;
    circle_outline(points, (float) cx, (float) cy, (float) radius - t / 2);
    stroke_outline(points, t, c);
#else
    stroke_rows((float) (cx - radius), (float) (cy - radius), (float) radius * 2, (float) radius * 2, (float) radius, t, c);
#endif
}

//
// draw_clip(x, y, w, h): from now on, paint only inside this area (a scrolling
// list paints its rows without spilling over its edge). draw_unclip() ends it.
//
void sharpgui_draw_clip(sni_env* env, double x, double y, double w, double h) {
    if(!gui_ready(env)) return;
    SDL_Rect area = { (int) std::floor(x), (int) std::floor(y), (int) std::ceil(w), (int) std::ceil(h) };
    SDL_RenderSetClipRect(g_renderer, &area);
}

void sharpgui_draw_unclip(sni_env* env) {
    if(g_renderer != nullptr)
        SDL_RenderSetClipRect(g_renderer, nullptr);
}


//
// Fill a shape given by its corners, in order around the shape - `numbers` is
// x y x y x y ... It is filled as a fan of triangles from the shape's middle,
// which is right for every shape that can be "seen" whole from its middle:
// triangles, diamonds, hexagons, stars, arrows. The edge is softened like every
// other shape's here.
static void fill_corners(const std::vector<float>& numbers, int64_t color);

//
// fill_polygon(xs, ys, count, shift_x, shift_y, colour): the shape whose first
// `count` corners are (xs[i] + shift_x, ys[i] + shift_y). The two arrays are
// read where they are - nothing is copied across or turned into text - which is
// what makes a scene of many shapes cheap. The shift is the painter's origin.
//
void sharpgui_fill_polygon(sni_env* env, sni_ref xs_ref, sni_ref ys_ref, int64_t count,
                           double shift_x, double shift_y, int64_t color) {
    if(!gui_ready(env)) return;
    sni_array xs = env->array(xs_ref);
    sni_array ys = env->array(ys_ref);
    if(xs.kind != SNI_DECIMAL || ys.kind != SNI_DECIMAL || xs.data == nullptr || ys.data == nullptr) return;
    if(count > xs.count) count = xs.count;
    if(count > ys.count) count = ys.count;
    if(count < 3) return;

    const double* x = (const double*) xs.data;
    const double* y = (const double*) ys.data;
    std::vector<float> numbers((size_t) count * 2);
    for(int64_t i = 0; i < count; i++) {
        numbers[i * 2] = (float) (x[i] + shift_x);
        numbers[i * 2 + 1] = (float) (y[i] + shift_y);
    }
    fill_corners(numbers, color);
}

//
// draw_polygon(points, colour): the same, with the corners as text -
// "x y x y x y ...". Kept for programs written against it; fill_polygon is the
// fast one (turning numbers into text and back costs far more than the drawing).
//
void sharpgui_draw_polygon(sni_env* env, sni_ref points_ref, int64_t color) {
    if(!gui_ready(env)) return;
    std::string text = env->text(points_ref);

    std::vector<float> numbers;
    const char* at = text.c_str();
    char* end = nullptr;
    for(;;) {
        float value = strtof(at, &end);
        if(end == at) break;
        numbers.push_back(value);
        at = end;
    }
    fill_corners(numbers, color);
}

static void fill_corners(const std::vector<float>& numbers, int64_t color) {
    size_t count = numbers.size() / 2;
    if(count < 3) return;

    float mx = 0, my = 0;
    for(size_t i = 0; i < count; i++) { mx += numbers[i * 2]; my += numbers[i * 2 + 1]; }
    mx /= count;
    my /= count;

    SDL_Color c = gui_color(color);
#if GUI_HAS_GEOMETRY
    // "the way out" at each corner: away from the middle (near enough for a soft edge)
    std::vector<outline_point> outline;
    for(size_t i = 0; i < count; i++) {
        outline_point p;
        p.x = numbers[i * 2];
        p.y = numbers[i * 2 + 1];
        float dx = p.x - mx, dy = p.y - my;
        float far = std::sqrt(dx * dx + dy * dy);
        p.nx = far > 0 ? dx / far : 0;
        p.ny = far > 0 ? dy / far : 0;
        outline.push_back(p);
    }
    fill_outline(outline, c);
#else
    // an old SDL without triangles: the outline only
    SDL_SetRenderDrawColor(g_renderer, c.r, c.g, c.b, c.a);
    for(size_t i = 0; i < count; i++) {
        size_t next = (i + 1) % count;
        SDL_RenderDrawLineF(g_renderer, numbers[i * 2], numbers[i * 2 + 1], numbers[next * 2], numbers[next * 2 + 1]);
    }
#endif
}
