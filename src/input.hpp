/* danielsinkin97@gmail.com */
#pragma once

#include "constants.hpp"
#include "demo.hpp"
#include "types.hpp"
#include "utils.hpp"

#include "backends/imgui_impl_sdl2.h"
#include <SDL.h>

namespace INPUT {
inline auto update_mouse_position() -> void {
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    global.input.mouse_pos_x = static_cast<double>(mouse_x) / CONSTANTS::window_width;
    global.input.mouse_pos_y = static_cast<double>(mouse_y) / CONSTANTS::window_width;
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
                global.sim.step_forward = false;
                global.sim.is_debugging = false;
                global.cpu_history.clear();
            } else {
                global.sim.is_debugging = true;
            }
            // apply the correct background right when we toggle
            global.color.background = (global.sim.is_debugging ? CONSTANTS::COLOR::background_debug : CONSTANTS::COLOR::background);
            break;

        case SDLK_UP:
        case SDLK_w:
            DEMO::set_direction_up();
            break;

        case SDLK_RIGHT:
        case SDLK_d:
            DEMO::set_direction_right();
            break;

        case SDLK_DOWN:
        case SDLK_s:
            DEMO::set_direction_down();
            break;

        case SDLK_LEFT:
        case SDLK_a:
            DEMO::set_direction_left();
            break;

        case SDLK_r:
            DEMO::reset_active_demo();
            println("Demo reset");
            break;

        case SDLK_t:
            global.sim.realtime_clock_mode = !global.sim.realtime_clock_mode;
            global.sim.cpu_tick_fractional_remainder = 0.0;
            println("Run clock mode: %s",
                global.sim.realtime_clock_mode ? "real-time 6502 (~1 MHz)" : "fixed ticks/frame");
            break;

        case SDLK_n:
            global.sim.is_debugging = true;
            global.sim.step_once = true;
            global.sim.step_back = false;
            global.sim.step_forward = false;
            global.color.background = CONSTANTS::COLOR::background_debug;
            break;

        case SDLK_b:
            global.sim.is_debugging = true;
            global.sim.step_once = false;
            global.sim.step_back = true;
            global.sim.step_forward = false;
            global.color.background = CONSTANTS::COLOR::background_debug;
            break;

        case SDLK_f:
            global.sim.is_debugging = true;
            global.sim.step_once = false;
            global.sim.step_back = false;
            global.sim.step_forward = true;
            global.color.background = CONSTANTS::COLOR::background_debug;
            break;

        case SDLK_i:
            global.cpu.irq = !global.cpu.irq;
            println("IRQ line %s", global.cpu.irq ? "asserted" : "cleared");
            break;

        case SDLK_m:
            global.cpu.nmi = true;
            println("NMI pulse requested");
            break;

        case SDLK_ESCAPE:
            println("Escape key pressed — exiting");
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
