//
// THE NATIVE SIDE OF THE "sharpgui" LIBRARY  -  the window, the frame, the events.
//
// This is the bottom layer of the Sharp GUI, on top of SDL2. It is declared once
// in gui/surface.sharp (the `native "sharpgui"` block) and loaded automatically
// the first time a program opens a window. Non-GUI programs never touch it.
//
// Each function here is one `def` from that block. The Sharp compiler writes
// sharpgui_sni.h (the prototypes) and the glue that unpacks arguments; we fill
// in the bodies with exactly the types the header declares.
//
// NAMING (Sharp Native Interface): a function `surface_open` of library
// "sharpgui" is `sharpgui_surface_open(sni_env* env, ...)` in C++. Numbers
// arrive as real C++ numbers; a string arrives as an sni_ref read with
// env->text().
//
// ONE WINDOW for now, kept in the two handles below. When the framework grows
// to many windows these become a small table keyed by the id surface_open
// returns - which is why Sharp already passes that id around.
//
#include "gui_internal.h"

SDL_Window*   g_window   = nullptr;
SDL_Renderer* g_renderer = nullptr;
float         g_scale    = 1;           // real pixels per unit (gui_internal.h)
static float  g_scale_wanted = 0;       // what the program asked for with surface_scale (0: follow the screen)

// How the window sits on the screen (surface_mode). These MUST match the
// window_mode enum in gui/platform/window.sharp.
enum { MODE_WINDOWED = 0, MODE_MAXIMIZED = 1, MODE_FULLSCREEN = 2 };
static int64_t g_mode = MODE_WINDOWED;  // the mode now - or, while no window is open, the one the next window opens in

// Does showing a frame wait for the screen's refresh (surface_paced, surface_pace)?
static bool g_waits_for_refresh = false;

//
// surface_scale(factor): how many real pixels one unit is - 1.5 on a screen at
// 150 %. Call it BEFORE the window opens; 0 means "ask the screen". Returns
// the scale in use (after the window is open, that is the real answer).
//
double sharpgui_surface_scale(sni_env* env, double factor) {
    if(g_window == nullptr) {
        g_scale_wanted = (float) (factor < 0 ? 0 : factor);
        return g_scale_wanted;
    }
    return g_scale;
}

// The scale the screen itself is set to: its dots per inch against the 96 a
// plain screen has, in quarter steps, never below 1.
static float screen_scale() {
    float dots = 0;
    if(SDL_GetDisplayDPI(0, &dots, nullptr, nullptr) != 0 || dots <= 0) return 1;
    float scale = (float) ((int) (dots / 96.0f * 4 + 0.5f)) / 4;
    if(scale < 1) scale = 1;
    if(scale > 4) scale = 4;
    return scale;
}

// The kinds of event we hand back to Sharp. These MUST match the EVENT_*
// constants in gui/event.sharp.
enum {
    EVT_NONE       = 0,
    EVT_QUIT       = 1,     // the window was closed
    EVT_KEY_DOWN   = 2,     // a: key code, b: modifier keys, c: 1 when it is a held key repeating
    EVT_KEY_UP     = 3,     // a: key code, b: modifier keys
    EVT_MOUSE_DOWN = 4,     // a: x, b: y, c: button (1 left, 2 middle, 3 right)
    EVT_MOUSE_UP   = 5,     // a: x, b: y, c: button
    EVT_MOUSE_MOVE = 6,     // a: x, b: y, c: how far it moved across, d: how far down
    EVT_WHEEL      = 7,     // a: x, b: y (of the mouse), c: sideways steps, d: up/down steps
    EVT_RESIZE     = 8,     // a: new width, b: new height
    EVT_TEXT       = 9      // typed text, read with surface_event_text()
};

// The details of the event surface_poll() last returned. Sharp asks for the
// ones the kind of event has (surface_event_a .. _d, surface_event_text).
static int64_t     g_event_a = 0, g_event_b = 0, g_event_c = 0, g_event_d = 0;
static std::string g_event_text;

// When the window opened, on the high-resolution clock (for surface_time()).
static Uint64 g_opened_at = 0;

bool gui_ready(sni_env* env) {
    if(g_renderer != nullptr) return true;
    env->raise("std#illegal_state_exception", "no window is open: make one with gui.window(...) before drawing");
    return false;
}

//
// surface_open(title, width, height): start SDL (once), make the window and a
// renderer to paint it with. Returns 1 (the window id) or 0 on failure, and on
// failure raises a Sharp exception that says why.
//
int64_t sharpgui_surface_open(sni_env* env, sni_ref title, int32_t width, int32_t height) {
    if(g_window != nullptr) {
        env->raise("std#illegal_state_exception", "a window is already open (one window at a time for now)");
        return 0;
    }
    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        env->raise("std#illegal_state_exception", std::string("SDL could not start: ") + SDL_GetError());
        return 0;
    }

    // Pictures and text are scaled smoothly instead of blocky.
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

#ifdef _WIN32
    // On Windows paint with Direct3D 11. SDL would pick Direct3D 9 first, and a Direct3D 9
    // window goes black for good the moment the 3D side makes its OpenGL context (g3_core.cpp);
    // Direct3D 11 is untroubled by it. (Where there is no Direct3D 11, SDL goes on down its
    // list; the environment variable SDL_RENDER_DRIVER still decides when it is set.)
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
#endif

    // the size was given in units: the window is that many units, whatever a unit is in pixels here
    g_scale = g_scale_wanted > 0 ? g_scale_wanted : screen_scale();

    // The size asked for, in real pixels - but never more than the screen has
    // room for: a window larger than a small screen (a Pi's 800 x 480 panel)
    // opens maximized instead of hanging off its edges.
    int pixels_wide = (int) (width * g_scale), pixels_high = (int) (height * g_scale);
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    SDL_Rect room;
    if(SDL_GetDisplayUsableBounds(0, &room) == 0 && room.w > 0 && room.h > 0
       && (pixels_wide > room.w || pixels_high > room.h)) {
        if(pixels_wide > room.w) pixels_wide = room.w;
        if(pixels_high > room.h) pixels_high = room.h;
        if(g_mode == MODE_WINDOWED) g_mode = MODE_MAXIMIZED;
    }

    // the mode chosen before the window opened (surface_mode). FULLSCREEN is the
    // whole screen at the resolution it already has: the screen is not switched
    // to another one, which is slow and unreliable on small machines.
    if(g_mode == MODE_MAXIMIZED) flags |= SDL_WINDOW_MAXIMIZED;
    if(g_mode == MODE_FULLSCREEN) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    std::string name = env->text(title);
    g_window = SDL_CreateWindow(name.c_str(),
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                pixels_wide, pixels_high, flags);
    if(g_window == nullptr) {
        env->raise("std#illegal_state_exception", std::string("the window would not open: ") + SDL_GetError());
        SDL_Quit();
        return 0;
    }

    // A GPU renderer paced to the screen's refresh; fall back to a software one
    // (older Pis, no accelerated driver) so a window always appears.
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(g_renderer == nullptr)
        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    if(g_renderer == nullptr) {
        env->raise("std#illegal_state_exception", std::string("nothing can paint the window: ") + SDL_GetError());
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
        SDL_Quit();
        return 0;
    }

    // which of the two did we get? (only the first waits for the screen)
    SDL_RendererInfo made;
    g_waits_for_refresh = SDL_GetRendererInfo(g_renderer, &made) == 0 && (made.flags & SDL_RENDERER_PRESENTVSYNC) != 0;

    // See-through colours mix with what is already painted.
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

    // every position and size the program draws with is multiplied by the scale from here on
    SDL_RenderSetScale(g_renderer, g_scale, g_scale);

    g_opened_at = SDL_GetPerformanceCounter();
    return 1;
}

//
// surface_close(id): take the window and renderer down (and the fonts and
// images that belonged to them) and shut SDL. After this the program can end
// cleanly - or open a new window.
//
void sharpgui_surface_close(sni_env* env, int64_t handle) {
    gui_forget_fonts();
    gui_forget_images();
    gui_forget_layers();
    g3_forget_window();
    if(g_renderer != nullptr) { SDL_DestroyRenderer(g_renderer); g_renderer = nullptr; }
    if(g_window   != nullptr) { SDL_DestroyWindow(g_window);     g_window   = nullptr; }
    g_mode = MODE_WINDOWED;                 // the next window starts as an ordinary one again
    g_waits_for_refresh = false;
    SDL_Quit();
}

//
// surface_mode(mode): how the window sits on the screen.
//
//      0   windowed     an ordinary window of the size it was opened with
//      1   maximized    fills the desktop; title bar and taskbar stay
//      2   fullscreen   the whole screen, no borders, at the screen's own resolution
//
// BEFORE a window is open it chooses the mode the next one opens in; with one
// open it switches on the spot (a game going fullscreen and back). Any other
// number changes nothing. Returns the mode in use.
//
int64_t sharpgui_surface_mode(sni_env* env, int64_t mode) {
    if(mode < MODE_WINDOWED || mode > MODE_FULLSCREEN) return g_mode;
    if(g_window == nullptr || mode == g_mode) { g_mode = mode; return g_mode; }

    if(mode == MODE_FULLSCREEN) {
        SDL_SetWindowFullscreen(g_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        if(g_mode == MODE_FULLSCREEN) SDL_SetWindowFullscreen(g_window, 0);
        if(mode == MODE_MAXIMIZED) SDL_MaximizeWindow(g_window);
        else SDL_RestoreWindow(g_window);
    }
    g_mode = mode;
    return g_mode;
}

//
// surface_screen_width() / surface_screen_height(): the size of the whole
// screen, in units - what a fullscreen window gets. Works before a window is
// open (SDL's video part is started for the question and stopped again).
//
static void screen_size(int* w, int* h) {
    *w = *h = 0;
    bool started_here = SDL_WasInit(SDL_INIT_VIDEO) == 0;
    if(started_here && SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) return;

    SDL_DisplayMode mode;
    int screen = g_window != nullptr ? SDL_GetWindowDisplayIndex(g_window) : 0;
    if(SDL_GetDesktopDisplayMode(screen < 0 ? 0 : screen, &mode) == 0) {
        float scale = g_window != nullptr ? g_scale : (g_scale_wanted > 0 ? g_scale_wanted : screen_scale());
        *w = (int) (mode.w / scale);
        *h = (int) (mode.h / scale);
    }
    if(started_here) SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

int32_t sharpgui_surface_screen_width(sni_env* env)  { int w, h; screen_size(&w, &h); return w; }
int32_t sharpgui_surface_screen_height(sni_env* env) { int w, h; screen_size(&w, &h); return h; }

//
// surface_paced(): how many frames a second surface_present() itself holds the
// program to - the screen's refresh rate when the renderer waits for it, and 0
// when nothing does (the software renderer, or after surface_pace(0)). The
// framework's frame limit reads it: a limit at or above this needs no timing of
// its own, and with 0 the limit is the ONLY thing that paces the frames.
//
int64_t sharpgui_surface_paced(sni_env* env) {
    if(g_window == nullptr || !g_waits_for_refresh) return 0;
    SDL_DisplayMode mode;
    int screen = SDL_GetWindowDisplayIndex(g_window);
    if(SDL_GetCurrentDisplayMode(screen < 0 ? 0 : screen, &mode) != 0 || mode.refresh_rate <= 0) return 60;
    return mode.refresh_rate;
}

//
// surface_pace(on): should showing a frame wait for the screen's refresh?
// 1 is how a window starts (when its renderer can); 0 lets frames be shown as
// fast as they are painted. Returns what is in force afterwards - a renderer
// that cannot wait (software) stays at 0, an old SDL cannot switch.
//
int64_t sharpgui_surface_pace(sni_env* env, int64_t on) {
    if(g_renderer == nullptr) return 0;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    if(SDL_RenderSetVSync(g_renderer, on != 0 ? 1 : 0) == 0)
        g_waits_for_refresh = on != 0;
#endif
    return g_waits_for_refresh ? 1 : 0;
}

//
// surface_keep_mouse(on): 1 keeps the mouse in the window - the pointer is
// hidden and cannot leave, and moving the mouse still reports how far it moved
// (EVT_MOUSE_MOVE c, d) with no edge to stop at: what a game that looks round
// with the mouse needs. 0 gives the pointer back. Any other number only asks.
// Returns whether the mouse is kept afterwards.
//
// The mouse is let go by itself when the window stops being the one in use
// (Alt+Tab, the Windows key): a program can never trap the pointer.
//
static bool g_mouse_kept = false;

int64_t sharpgui_surface_keep_mouse(sni_env* env, int64_t on) {
    if(g_window == nullptr) return 0;
    if(on == 0 || on == 1) {
        if(SDL_SetRelativeMouseMode(on == 1 ? SDL_TRUE : SDL_FALSE) == 0)
            g_mouse_kept = on == 1;
    }
    return g_mouse_kept ? 1 : 0;
}

//
// surface_title(title): the words in the window's title bar, which are also the
// app's name in the taskbar. Does nothing while no window is open.
//
void sharpgui_surface_title(sni_env* env, sni_ref title) {
    if(g_window == nullptr) return;
    std::string name = env->text(title);
    SDL_SetWindowTitle(g_window, name.c_str());
}

// surface_width() / surface_height(): the size of the drawing area right now
// (the window can be resized by the user).
int32_t sharpgui_surface_width(sni_env* env) {
    int w = 0, h = 0;
    if(g_renderer != nullptr) {
        SDL_Texture* into = SDL_GetRenderTarget(g_renderer);       // (a layer may be open: ask about the window itself)
        if(into != nullptr) SDL_GetWindowSize(g_window, &w, &h);
        else SDL_GetRendererOutputSize(g_renderer, &w, &h);
    }
    return (int32_t) (w / g_scale);
}

int32_t sharpgui_surface_height(sni_env* env) {
    int w = 0, h = 0;
    if(g_renderer != nullptr) {
        SDL_Texture* into = SDL_GetRenderTarget(g_renderer);
        if(into != nullptr) SDL_GetWindowSize(g_window, &w, &h);
        else SDL_GetRendererOutputSize(g_renderer, &w, &h);
    }
    return (int32_t) (h / g_scale);
}

// surface_time(): seconds since the window opened, as a decimal. Animations
// and games measure their steps with it.
double sharpgui_surface_time(sni_env* env) {
    if(g_window == nullptr) return 0;
    return (double) (SDL_GetPerformanceCounter() - g_opened_at) / (double) SDL_GetPerformanceFrequency();
}

// Which of shift / ctrl / alt are held, as the bits Sharp knows (event.sharp).
static int64_t modifier_bits(Uint16 mod) {
    int64_t bits = 0;
    if(mod & KMOD_SHIFT) bits |= 1;
    if(mod & KMOD_CTRL)  bits |= 2;
    if(mod & KMOD_ALT)   bits |= 4;
    return bits;
}

//
// The key Sharp is told about. Mostly the key itself; but the NUMBER PAD is
// two keyboards in one: with NumLock on its keys type digits, with NumLock off
// they are the navigation keys printed underneath (4 is Left, 7 is Home ...).
// A program should not have to know about that, so with NumLock off they are
// handed on as the navigation keys they act as. The number pad's Enter is
// always Enter.
//
static SDL_Keycode key_meant(SDL_Keycode key, Uint16 mod) {
    if(key == SDLK_KP_ENTER) return SDLK_RETURN;
    if(mod & KMOD_NUM) return key;              // NumLock on: digits (which arrive as typed text anyway)

    switch(key) {
        case SDLK_KP_4:      return SDLK_LEFT;
        case SDLK_KP_6:      return SDLK_RIGHT;
        case SDLK_KP_8:      return SDLK_UP;
        case SDLK_KP_2:      return SDLK_DOWN;
        case SDLK_KP_7:      return SDLK_HOME;
        case SDLK_KP_1:      return SDLK_END;
        case SDLK_KP_9:      return SDLK_PAGEUP;
        case SDLK_KP_3:      return SDLK_PAGEDOWN;
        case SDLK_KP_PERIOD: return SDLK_DELETE;
        default:             return key;
    }
}

//
// surface_poll(): the next thing that happened, or EVT_NONE when the queue is
// empty. Things Sharp has no use for (window moved, focus changed ...) are
// skipped right here, so the program only wakes up for real events.
//
int64_t sharpgui_surface_poll(sni_env* env) {
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0) {
        g_event_a = g_event_b = g_event_c = g_event_d = 0;

        switch(e.type) {
            case SDL_QUIT:
                return EVT_QUIT;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                g_event_a = key_meant(e.key.keysym.sym, e.key.keysym.mod);
                g_event_b = modifier_bits(e.key.keysym.mod);
                g_event_c = e.key.repeat != 0;
                return e.type == SDL_KEYDOWN ? EVT_KEY_DOWN : EVT_KEY_UP;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                g_event_a = (int64_t) (e.button.x / g_scale);      // the mouse is reported in real pixels: into units
                g_event_b = (int64_t) (e.button.y / g_scale);
                g_event_c = e.button.button;       // SDL: 1 left, 2 middle, 3 right - passed on as is
                return e.type == SDL_MOUSEBUTTONDOWN ? EVT_MOUSE_DOWN : EVT_MOUSE_UP;

            case SDL_MOUSEMOTION:
                g_event_a = (int64_t) (e.motion.x / g_scale);
                g_event_b = (int64_t) (e.motion.y / g_scale);
                g_event_c = e.motion.xrel;         // how far, in real pixels: a kept mouse has no position worth reading
                g_event_d = e.motion.yrel;
                return EVT_MOUSE_MOVE;

            case SDL_MOUSEWHEEL: {
                int x = 0, y = 0;
                SDL_GetMouseState(&x, &y);
                g_event_a = (int64_t) (x / g_scale);
                g_event_b = (int64_t) (y / g_scale);
                g_event_c = e.wheel.x;
                g_event_d = e.wheel.y;
                return EVT_WHEEL;
            }

            case SDL_TEXTINPUT:
                g_event_text = e.text.text;
                return EVT_TEXT;

            case SDL_WINDOWEVENT:
                if(e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    g_event_a = (int64_t) (e.window.data1 / g_scale);
                    g_event_b = (int64_t) (e.window.data2 / g_scale);
                    return EVT_RESIZE;
                }
                // the user maximized or restored the window with its own buttons: surface_mode() must say so
                if(e.window.event == SDL_WINDOWEVENT_MAXIMIZED && g_mode != MODE_FULLSCREEN) g_mode = MODE_MAXIMIZED;
                if(e.window.event == SDL_WINDOWEVENT_RESTORED && g_mode == MODE_MAXIMIZED) g_mode = MODE_WINDOWED;
                // another window is in use now: a kept mouse is let go (surface_keep_mouse)
                if(e.window.event == SDL_WINDOWEVENT_FOCUS_LOST && g_mouse_kept) {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    g_mouse_kept = false;
                }
                break;                              // other window news: skip

            default:
                break;                              // not something Sharp hears about: skip
        }
    }
    return EVT_NONE;
}

//
// surface_wait(ms): sleep until an event is waiting, at most `ms` milliseconds.
// Returns 1 when there is an event to read, 0 when the time ran out. This is
// what lets an idle app use no processor: instead of painting the same frame
// 60 times a second, the framework rests here until something happens.
//
int64_t sharpgui_surface_wait(sni_env* env, int64_t ms) {
    if(g_window == nullptr) return 0;
    return SDL_WaitEventTimeout(nullptr, (int) (ms < 0 ? 0 : ms)) != 0;
}

//
// surface_wake(): make a sleeping surface_wait() return at once. This is the
// one function here that may be called from ANY thread: a background thread
// that has finished its work uses it to say "the main thread has something to
// do" (on_main). It posts an event the program itself never sees.
//
void sharpgui_surface_wake(sni_env* env) {
    if(g_window == nullptr) return;
    SDL_Event nudge;
    SDL_zero(nudge);
    nudge.type = SDL_USEREVENT;
    SDL_PushEvent(&nudge);              // (SDL: safe to call from any thread)
}

// The details of the last event (what each one means: the EVT_ list above).
int64_t sharpgui_surface_event_a(sni_env* env) { return g_event_a; }
int64_t sharpgui_surface_event_b(sni_env* env) { return g_event_b; }
int64_t sharpgui_surface_event_c(sni_env* env) { return g_event_c; }
int64_t sharpgui_surface_event_d(sni_env* env) { return g_event_d; }

// The text of the last EVT_TEXT, as a new Sharp string.
sni_ref sharpgui_surface_event_text(sni_env* env) {
    return env->new_text(g_event_text);
}

//
// THE CLIPBOARD - the system's own, the one every other program shares: text
// copied in a Sharp app can be pasted into a browser, and the other way round.
// SDL keeps it alive for as long as the window is open.
//
// clipboard_read(): the text on the clipboard, as a new Sharp string ("" when
// it is empty or holds something that is not text).
//
sni_ref sharpgui_clipboard_read(sni_env* env) {
    if(g_window == nullptr || SDL_HasClipboardText() == SDL_FALSE)
        return env->new_text("");

    char* text = SDL_GetClipboardText();        // SDL hands us a copy: ours to free
    std::string copy = text == nullptr ? "" : text;
    if(text != nullptr) SDL_free(text);
    return env->new_text(copy);
}

// clipboard_write(text): put this text on the clipboard, replacing what was there.
void sharpgui_clipboard_write(sni_env* env, sni_ref text) {
    if(g_window == nullptr) return;
    SDL_SetClipboardText(env->text(text).c_str());
}

//
// surface_fill(colour): paint the whole surface one colour. The frame is not
// shown until surface_present().
//
void sharpgui_surface_fill(sni_env* env, int64_t color) {
    if(g_renderer == nullptr) return;
    SDL_Color c = gui_color(color);
    SDL_SetRenderDrawColor(g_renderer, c.r, c.g, c.b, 255);
    SDL_RenderClear(g_renderer);
}

//
// surface_pixel(x, y): the colour of the frame that is being painted, at a point (in units from the
// top left corner): 0xRRGGBBAA, or -1 when there is no window or the point is outside it.
//
// It answers for what was painted since surface_fill and BEFORE surface_present: once a frame is
// shown, the card may throw the picture away. (UI tests that check a colour: std.guikit.testing.)
//
int64_t sharpgui_surface_pixel(sni_env* env, double x, double y) {
    if(g_renderer == nullptr) return -1;
    int wide = 0, high = 0;
    SDL_GetRendererOutputSize(g_renderer, &wide, &high);
    SDL_Rect at = { (int) (x * g_scale), (int) (y * g_scale), 1, 1 };
    if(at.x < 0 || at.y < 0 || at.x >= wide || at.y >= high) return -1;
    unsigned char seen[4] = { 0, 0, 0, 255 };
    if(SDL_RenderReadPixels(g_renderer, &at, SDL_PIXELFORMAT_RGBA32, seen, 4) != 0) return -1;
    return ((int64_t) seen[0] << 24) | ((int64_t) seen[1] << 16) | ((int64_t) seen[2] << 8) | 255;
}

//
// surface_present(): show the frame that was just painted. With a vsync renderer
// this also paces the loop to the screen's refresh rate.
//
void sharpgui_surface_present(sni_env* env) {
    if(g_renderer != nullptr)
        SDL_RenderPresent(g_renderer);
}
