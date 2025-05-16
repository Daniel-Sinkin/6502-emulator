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
#include "constants.hpp"
#include "engine.hpp"
#include "gl.hpp"
#include "global.hpp"
#include "input.hpp"
#include "render.hpp"
#include "types.hpp"
#include "utils.hpp"

#include "6502/6502.hpp"
#include "6502/program_writer.hpp"

auto load_example_simple() -> void {
    auto pw = mos6502::ProgramWriter(global.cpu);
    pw.lda_immediate();
    pw(0x44);

    pw.jmp_indirect();
    pw(0x10);
    pw(0x00);

    pw.jmp_absolute();
    pw(0x00);
    pw(0x00);

    pw.addr = 0x10;
    pw(0x20);

    pw.addr = 0x20;
    pw.jmp_absolute();
    pw(0x20);
    pw(0x00);
}

auto main() -> int {
    println("Application starting");
    if (!ENGINE::setup()) assert(false);
    println("Engine setup complete");

    mos6502::initialize_instructions();
    // global.cpu = mos6502::CPU();
    load_example_simple();
    // auto pw = mos6502::ProgramWriter(global.cpu);
    // pw.bne();
    // pw(0x05);

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

        if (global.sim.is_debugging) {
            if (global.sim.step_once) {
                global.cpu_snapshots.push(mos6502::CPUSnapshot{.cpu = global.cpu});
                if (global.cpu_snapshots.size() > 100) {
                    println("There are more than 100 Snapshots stored, currently we copy entire memory buffer for every snapshot!");
                }
                mos6502::tick(global.cpu);
                global.sim.step_once = false;
            } else if (global.sim.step_back) {
                if (!global.cpu_snapshots.empty()) {
                    global.cpu = global.cpu_snapshots.top().cpu;
                    global.cpu_snapshots.pop();
                } else {
                    println("Tried to step back but empyt snapshot registry");
                }
                global.sim.step_back = false;
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