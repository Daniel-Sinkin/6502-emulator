// danielsinkin97@gmail.com
#pragma once

#include <SDL_mixer.h>
#include <stdexcept>
#include <string>

#include "constants.hpp"
#include "global.hpp"
#include "log.hpp"

namespace Audio {

inline bool initialized = false;

inline void init() {
    if (initialized) {
        LOG_WARN("Audio::init() called more than once — ignoring");
        return;
    }

    LOG_INFO("Initializing SDL_mixer...");

    int mix_flags = MIX_INIT_MP3;
    int initted = Mix_Init(mix_flags);
    if ((initted & mix_flags) != mix_flags) {
        throw std::runtime_error("Mix_Init failed: " + std::string(Mix_GetError()));
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        throw std::runtime_error("Mix_OpenAudio failed: " + std::string(Mix_GetError()));
    }

    global.audio.beep_sound = Mix_LoadWAV(CONSTANTS::fp_sound_beep);
    if (!global.audio.beep_sound) {
        throw std::runtime_error("Failed to load beep: " + std::string(Mix_GetError()));
    }

    initialized = true;
    LOG_INFO("Audio system initialized");
}

inline void shutdown() {
    if (!initialized) return;

    if (global.audio.beep_sound) {
        Mix_FreeChunk(global.audio.beep_sound);
        global.audio.beep_sound = nullptr;
    }

    Mix_CloseAudio();
    Mix_Quit();

    initialized = false;
    LOG_INFO("Audio system shut down");
}

} // namespace Audio