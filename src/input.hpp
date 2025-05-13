/* danielsinkin97@gmail.com */
#pragma once

#include "constants.hpp"
#include "log.hpp"
#include "types.hpp"
#include "utils.hpp"

#include "backends/imgui_impl_sdl.h"
#include <SDL.h>

namespace INPUT {
inline auto update_mouse_position() -> void {
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    global.input.mouse_pos = Position{
        static_cast<float>(mouse_x) / CONSTANTS::window_width,
        static_cast<float>(mouse_y) / CONSTANTS::window_height};
}

inline auto handle_event(const SDL_Event &event) -> void {
    ImGui_ImplSDL2_ProcessEvent(&event);

    switch (event.type) {
    case SDL_QUIT:
        LOG_INFO("Received SDL_QUIT event");
        global.is_running = false;
        break;

    case SDL_KEYDOWN: {
        if (event.key.keysym.sym == SDLK_ESCAPE) {
            LOG_INFO("Escape key pressed — exiting");
            global.is_running = false;
        }
        break;
    }
    }
}

inline auto handle_input() -> void {
    update_mouse_position();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        handle_event(event);
    }
}
} // namespace INPUT