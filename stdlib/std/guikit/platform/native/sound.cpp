//
// SOUND MADE BY THE PROGRAM  -  no sound files: every sample is worked out as it is played.
//
// A program sets up VOICES (gui/sound.sharp); each goes on by itself until it is taken away:
//
//      hum         a steady note with a slow swell
//      rumble      deep noise that swells and fades: the sea, wind, a far-off engine
//      chime       a bell-like note that rings out, now and then, on one of five notes that sound well together
//      call        a note that glides from one pitch to another: a whale, a siren, a bird
//      pops        short rising blips, now and then: bubbles, drips
//
// SDL calls mix_sound() on ITS OWN THREAD whenever the sound card wants more samples. The voices
// are changed only while that thread is held off (SDL_LockAudioDevice), so the two never meet.
// Nothing here allocates or waits while mixing.
//
#include "gui_internal.h"
#include <vector>
#include <cmath>
#include <cstdint>

enum { VOICE_HUM = 0, VOICE_RUMBLE = 1, VOICE_CHIME = 2, VOICE_CALL = 3, VOICE_POPS = 4 };

struct sound_voice {
    int kind = VOICE_HUM;
    float from = 110, to = 110;         // its pitch (a call glides from one to the other), in turns a second
    float seconds = 3;                  // how long one sounding lasts (chime, call)
    float volume = 0.2f;
    float every_least = 0, every_most = 0;      // how long it rests between soundings (0, 0: it never rests)

    // while it plays
    double turn = 0, turn2 = 0;         // where its waves are, 0 .. 1
    float age = 0;                      // seconds into this sounding (below 0: resting until 0)
    float pitch = 0;                    // the pitch of this sounding
    float low = 0, lower = 0;           // rumble: the noise, smoothed twice
    uint32_t chance = 12345;
};

static std::vector<sound_voice> g_voices;
static SDL_AudioDeviceID g_sound_device = 0;
static bool g_sound_muted = false;
static float g_sound_level = 0;         // rises to 1 when sound starts, so that it does not begin with a click
static const float SOUND_RATE = 44100;

static float voice_chance(sound_voice& v) {
    v.chance = v.chance * 1664525u + 1013904223u;
    return (v.chance >> 8) / (float) 0x1000000;
}

static float wave(double turn) { return (float) std::sin(turn * 6.283185307179586); }

// when a voice that rests has finished sounding: rest, then begin again
static void rest(sound_voice& v) {
    v.age = -(v.every_least + (v.every_most - v.every_least) * voice_chance(v));
}

static float one_sample(sound_voice& v) {
    const float dt = 1 / SOUND_RATE;
    float out = 0;
    switch(v.kind) {
        case VOICE_HUM: {
            v.turn += v.from * dt;
            v.turn2 += v.from * 1.503f * dt;                // a fifth above, a little out of tune: it beats slowly
            if(v.turn >= 1) v.turn -= 1;
            if(v.turn2 >= 1) v.turn2 -= 1;
            v.age += dt;
            float swell = 0.7f + 0.3f * std::sin(v.age * 0.31f);
            out = (wave(v.turn) + 0.35f * wave(v.turn2)) * swell * 0.7f;
            break;
        }
        case VOICE_RUMBLE: {
            // white noise smoothed twice: only its deepest part is left; it swells and fades over many seconds
            float white = voice_chance(v) * 2 - 1;
            float keep = std::exp(-6.2832f * v.from * dt);  // `from` is where the smoothing lets go: 60 .. 300
            v.low = v.low * keep + white * (1 - keep);
            v.lower = v.lower * keep + v.low * (1 - keep);
            v.age += dt;
            float swell = 0.6f + 0.4f * std::sin(v.age * 0.17f) * std::sin(v.age * 0.071f + 1.3f);
            out = v.lower * 9 * swell;
            break;
        }
        case VOICE_CHIME: {
            if(v.age < 0) { v.age += dt; if(v.age >= 0) { v.age = 0; v.turn = v.turn2 = 0; v.pitch = 0; } break; }
            if(v.pitch == 0) {
                // one of five notes that sound well together in any order, over two octaves
                static const float steps[5] = { 1.0f, 1.125f, 1.25f, 1.5f, 1.6667f };
                v.pitch = v.from * steps[(int) (voice_chance(v) * 5) % 5] * (voice_chance(v) < 0.4f ? 2.0f : 1.0f);
            }
            v.turn += v.pitch * dt;
            v.turn2 += v.pitch * 2.76f * dt;                // a bell's upper tone is not a whole multiple
            if(v.turn >= 1) v.turn -= 1;
            if(v.turn2 >= 1) v.turn2 -= 1;
            float fade = std::exp(-v.age * 4.2f / v.seconds) * std::min(1.0f, v.age * 200);
            out = (wave(v.turn) + 0.3f * wave(v.turn2) * std::exp(-v.age * 5)) * fade;
            v.age += dt;
            if(v.age > v.seconds) { if(v.every_most > 0) rest(v); else { v.age = 0; } v.pitch = 0; }
            break;
        }
        case VOICE_CALL: {
            if(v.age < 0) { v.age += dt; if(v.age >= 0) { v.age = 0; v.turn = 0; } break; }
            float t = v.age / v.seconds;
            float glide = t * t * (3 - 2 * t);
            float pitch = v.from + (v.to - v.from) * glide;
            pitch *= 1 + 0.012f * std::sin(v.age * 31);     // a tremble, as a voice has
            v.turn += pitch * dt;
            if(v.turn >= 1) v.turn -= 1;
            float rise = std::sin(3.14159f * t);
            out = (wave(v.turn) + 0.25f * wave(v.turn * 2) + 0.1f * wave(v.turn * 3)) * rise * rise * 0.8f;
            v.age += dt;
            if(v.age > v.seconds) { if(v.every_most > 0) rest(v); else v.age = 0; }
            break;
        }
        case VOICE_POPS: {
            if(v.age < 0) { v.age += dt; if(v.age >= 0) { v.age = 0; v.turn = 0; v.pitch = v.from * (0.7f + 0.9f * voice_chance(v)); } break; }
            const float lasts = 0.07f;
            float t = v.age / lasts;
            v.turn += v.pitch * (1 + 1.6f * t) * dt;        // it rises as it goes: a bubble
            if(v.turn >= 1) v.turn -= 1;
            out = wave(v.turn) * std::sin(3.14159f * t);
            v.age += dt;
            if(v.age > lasts) rest(v);
            break;
        }
    }
    return out * v.volume;
}

static void mix_sound(void*, Uint8* stream, int bytes) {
    float* out = (float*) stream;
    int count = bytes / (int) sizeof(float);
    for(int i = 0; i < count; i++) {
        float sum = 0;
        for(sound_voice& v : g_voices) sum += one_sample(v);
        float wanted = g_sound_muted ? 0.0f : 1.0f;
        g_sound_level += (wanted - g_sound_level) * 0.0004f;        // on and off in a fifth of a second: no click
        sum *= g_sound_level;
        out[i] = sum / (1 + std::fabs(sum));                        // loud passages are pressed down softly, never cut off
    }
}

// open the sound card when the first voice is made; false when the machine has none (the program goes on, silent)
static bool sound_ready() {
    if(g_sound_device != 0) return true;
    if(SDL_WasInit(SDL_INIT_AUDIO) == 0 && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return false;
    SDL_AudioSpec want, got;
    SDL_zero(want);
    want.freq = (int) SOUND_RATE;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = mix_sound;
    g_sound_device = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0);
    if(g_sound_device == 0) return false;
    SDL_PauseAudioDevice(g_sound_device, 0);
    return true;
}

//
// sound_voice(kind, from, to, seconds): a new voice (VOICE_... above). Returns its id, 0 when the
// machine has no sound.
//
int64_t sharpgui_sound_voice(sni_env* env, int64_t kind, double from, double to, double seconds) {
    if(!sound_ready()) return 0;
    sound_voice v;
    v.kind = (int) kind;
    v.from = (float) from;
    v.to = (float) to;
    v.seconds = seconds > 0.01 ? (float) seconds : 0.01f;
    SDL_LockAudioDevice(g_sound_device);
    v.chance = 12345u + (uint32_t) g_voices.size() * 7919u;
    g_voices.push_back(v);
    int64_t id = (int64_t) g_voices.size();
    SDL_UnlockAudioDevice(g_sound_device);
    return id;
}

// sound_set(voice, volume, every_least, every_most): how loud it is (0 .. 1) and how long it rests between soundings
void sharpgui_sound_set(sni_env* env, int64_t voice, double volume, double every_least, double every_most) {
    if(g_sound_device == 0) return;
    SDL_LockAudioDevice(g_sound_device);
    if(voice >= 1 && voice <= (int64_t) g_voices.size()) {
        sound_voice& v = g_voices[(size_t) voice - 1];
        v.volume = (float) volume;
        bool rested = v.every_most > 0;
        v.every_least = (float) every_least;
        v.every_most = (float) (every_most > every_least ? every_most : every_least);
        if(!rested && v.every_most > 0) v.age = -v.every_least * voice_chance(v);   // its first sounding comes soon
    }
    SDL_UnlockAudioDevice(g_sound_device);
}

// sound_mute(on): 1 silences everything (the voices go on unheard), 0 lets them be heard again
void sharpgui_sound_mute(sni_env* env, int64_t on) {
    g_sound_muted = on != 0;
}

// sound_clear(): take every voice away
void sharpgui_sound_clear(sni_env* env) {
    if(g_sound_device == 0) return;
    SDL_LockAudioDevice(g_sound_device);
    g_voices.clear();
    SDL_UnlockAudioDevice(g_sound_device);
}
