/* danielsinkin97@gmail.com */

// Core system and OpenGL
#include <SDL.h>
#include <glad/glad.h>

// ImGui backends
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"

// Standard library
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>

// Project headers
#include "constants.hpp"
#include "engine.hpp"
#include "gl.hpp"
#include "global.hpp"
#include "input.hpp"
#include "render.hpp"
#include "types.hpp"
#include "utils.hpp"

#include "6502/6502.hpp"
#include "demo.hpp"

auto main() -> int {
    println("Application starting");
    if (!ENGINE::setup()) {
        println("Engine setup failed");
        return EXIT_FAILURE;
    }
    println("Engine setup complete");

    mos6502::initialize_instructions();
    DEMO::load_default_demo();

    global.debug_activate();

    global.is_running = true;
    global.sim.run_start_time = std::chrono::steady_clock::now();
    global.sim.frame_start_time = global.sim.run_start_time;

    println("Entering main loop");
    while (global.is_running) {
        global.validate();

        auto now = std::chrono::steady_clock::now();
        global.sim.delta_time = now - global.sim.frame_start_time;
        global.sim.frame_start_time = now;
        global.sim.total_runtime = now - global.sim.run_start_time;

        INPUT::handle_input();
        DEMO::update();

        global.sim.last_ticks_executed = 0;
        global.sim.last_instructions_executed = 0;
        auto tick_and_count_instruction = [&]() -> void {
            const bool was_executing_instruction =
                global.cpu.addr_result.type != mos6502::AddrResultType::load_instruction;
            mos6502::tick(global.cpu);
            const bool completed_instruction =
                was_executing_instruction &&
                global.cpu.addr_result.type == mos6502::AddrResultType::load_instruction &&
                global.cpu.instr_counter == 0;
            if (completed_instruction) {
                ++global.sim.last_instructions_executed;
                ++global.sim.instructions_executed_total;
            }
        };

        if (global.sim.is_debugging) {
            if (global.sim.step_once) {
                const auto before_state = mos6502::capture_core_state(global.cpu);
                const auto before_mem = global.cpu.mem;
                tick_and_count_instruction();
                global.cpu_history.record_step(before_state, before_mem, global.cpu);
                global.sim.step_once = false;
                global.sim.last_ticks_executed = 1;
            } else if (global.sim.step_back) {
                if (!global.cpu_history.step_back(global.cpu)) {
                    println("Tried to step back but history is empty");
                }
                global.sim.step_back = false;
            } else if (global.sim.step_forward) {
                if (!global.cpu_history.step_forward(global.cpu)) {
                    println("Tried to step forward but no future history is available");
                }
                global.sim.step_forward = false;
            }
        } else {
            std::size_t ticks_to_run = CONSTANTS::n_iter_per_frame;
            if (global.sim.realtime_clock_mode) {
                const double target_ticks = (global.sim.delta_time.count() * CONSTANTS::cpu_clock_hz_6502) +
                                            global.sim.cpu_tick_fractional_remainder;
                const double floored_ticks = std::floor(std::max(0.0, target_ticks));
                ticks_to_run = static_cast<std::size_t>(floored_ticks);
                global.sim.cpu_tick_fractional_remainder = target_ticks - floored_ticks;
                ticks_to_run = std::min(ticks_to_run, CONSTANTS::max_realtime_ticks_per_frame);
            } else {
                global.sim.cpu_tick_fractional_remainder = 0.0;
            }
            global.sim.last_ticks_executed = static_cast<uint64_t>(ticks_to_run);
            for (std::size_t i = 0; i < ticks_to_run; ++i) {
                tick_and_count_instruction();
            }
        }

        RENDER::gui_debug();
        RENDER::frame();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(global.renderer.window);

        global.sim.frame_counter += 1;
    }

    println("Main loop exited");
    ENGINE::cleanup();
    println("Engine cleanup complete");
    println("Application exiting successfully");

    return EXIT_SUCCESS;
}
