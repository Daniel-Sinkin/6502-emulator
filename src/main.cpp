/* danielsinkin97@gmail.com */

// Core system and OpenGL
#include <SDL.h>
#include <glad/glad.h>

// ImGui backends
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl.h"
#include "imgui.h"

// Standard library
#include <array>
#include <bitset>
#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <thread>

using std::chrono::steady_clock;
using namespace std::chrono_literals;

// Project headers
#include "audio.hpp"
#include "constants.hpp"
#include "engine.hpp"
#include "gl.hpp"
#include "global.hpp"
#include "input.hpp"
#include "log.hpp"
#include "render.hpp"
#include "types.hpp"
#include "utils.hpp"

#include "6502/6502.hpp"

auto main() -> int {
    LOG_INFO("Application starting");
    if (!ENGINE::setup()) PANIC("Setup failed!");
    LOG_INFO("Engine setup complete");

    global.cpu = mos6502::CPU();
    global.cpu.mem[0] =

        global.is_running = true;
    global.sim.run_start_time = std::chrono::steady_clock::now();
    global.sim.frame_start_time = global.sim.run_start_time;
    constexpr std::chrono::milliseconds instruction_interval{250};

    auto last_instruction_time = std::chrono::steady_clock::now();
    LOG_INFO("Entering main loop");
    while (global.is_running) {
        global.validate();

        auto now = std::chrono::steady_clock::now();
        global.sim.delta_time = now - global.sim.frame_start_time;
        global.sim.frame_start_time = now;
        global.sim.total_runtime = now - global.sim.run_start_time;

        INPUT::handle_input();

        if (global.sim.is_debugging) {
            if (global.sim.step_once) {
                global.cpu_snapshots.push(mos6502::CPUSnapshot{.cpu = global.cpu});
                if (global.cpu_snapshots.size() > 100) {
                    LOG_WARN("There are more than 100 Snapshots stored, currently we copy entire memory buffer for every snapshot!");
                }
                mos6502::cpu_tick(global.cpu);
                global.sim.step_once = false;
            } else if (global.sim.step_back) {
                if (!global.cpu_snapshots.empty()) {
                    global.cpu = global.cpu_snapshots.top().cpu;
                    global.cpu_snapshots.pop();
                } else {
                    LOG_WARN("Tried to step back but empyt snapshot registry");
                }
                global.sim.step_back = false;
            }
        } else {
            mos6502::cpu_tick(global.cpu);
        }

        RENDER::gui_debug();
        RENDER::frame();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(global.renderer.window);

        global.sim.frame_counter += 1;
    }

    LOG_INFO("Main loop exited");
    ENGINE::cleanup();
    LOG_INFO("Engine cleanup complete");
    LOG_INFO("Application exiting successfully");

    return EXIT_SUCCESS;
}