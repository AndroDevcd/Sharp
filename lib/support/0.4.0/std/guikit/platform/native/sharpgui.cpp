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
    EVT_MOUSE_MOVE = 6,     // a: x, b: y
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

    // the size was given in units: the window is that many units, whatever a unit is in pixels here
    g_scale = g_scale_wanted > 0 ? g_scale_wanted : screen_scale();

    std::string name = env->text(title);
    g_window = SDL_CreateWindow(name.c_str(),
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                (int) (width * g_scale), (int) (height * g_scale),
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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
    if(g_renderer != nullptr) { SDL_DestroyRenderer(g_renderer); g_renderer = nullptr; }
    if(g_window   != nullptr) { SDL_DestroyWindow(g_window);     g_window   = nullptr; }
    SDL_Quit();
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
// surface_present(): show the frame that was just painted. With a vsync renderer
// this also paces the loop to the screen's refresh rate.
//
void sharpgui_surface_present(sni_env* env) {
    if(g_renderer != nullptr)
        SDL_RenderPresent(g_renderer);
}
