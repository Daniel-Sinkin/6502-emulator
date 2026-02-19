/* danielsinkin97@gmail.com */

// Core system and OpenGL
#include <SDL.h>
#include <glad/glad.h>

// ImGui backends
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl.h"
#include "imgui.h"

// Standard library
#include <chrono>
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
    DEMO::load_mode(DEMO::Mode::framebuffer_pattern);

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

        if (global.sim.is_debugging) {
            if (global.sim.step_once) {
                const auto before_state = mos6502::capture_core_state(global.cpu);
                const auto before_mem = global.cpu.mem;
                mos6502::tick(global.cpu);
                global.cpu_history.record_step(before_state, before_mem, global.cpu);
                global.sim.step_once = false;
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
            mos6502::tick(global.cpu);
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
