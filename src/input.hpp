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
    case SDL_KEYDOWN: {
        switch (event.key.keysym.sym) {
        case SDLK_SPACE:
            if (global.sim.is_debugging) {
                global.sim.step_once = false;
                global.sim.step_back = false;
                global.sim.is_debugging = false;
                // Flush the snapshot buffer
                std::stack<mos6502::CPUSnapshot>().swap(global.cpu_snapshots);
            } else {
                global.sim.is_debugging = true;
            }
            // apply the correct background right when we toggle
            global.color.background = (global.sim.is_debugging ? CONSTANTS::COLOR::background_debug : CONSTANTS::COLOR::background);
            break;

        case SDLK_n:
            global.sim.is_debugging = true;
            global.sim.step_once = true;
            global.sim.step_back = false;
            global.color.background = CONSTANTS::COLOR::background_debug;
            break;

        case SDLK_b:
            global.sim.is_debugging = true;
            global.sim.step_once = false;
            global.sim.step_back = true;
            global.color.background = CONSTANTS::COLOR::background_debug;
            break;

        case SDLK_ESCAPE:
            LOG_INFO("Escape key pressed — exiting");
            global.is_running = false;
            break;
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