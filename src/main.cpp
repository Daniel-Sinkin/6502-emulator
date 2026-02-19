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
#include "6502/program_writer.hpp"

auto load_example_framebuffer_demo() -> void {
    global.cpu = mos6502::CPU{};
    constexpr Address start = 0x0600;
    constexpr Byte frame_counter_zp = static_cast<Byte>(0x10);
    constexpr Address interrupt_handler = 0x0700;
    auto pw = mos6502::ProgramWriter(global.cpu, start);

    auto emit_relative_operand = [&](Address target) -> void {
        const int next_pc = static_cast<int>(pw.addr + 1u);
        const int rel = static_cast<int>(target) - next_pc;
        assert(rel >= -128 && rel <= 127);
        pw(static_cast<Byte>(rel & 0xFF));
    };

    // 32x32 framebuffer at $0200-$05FF.
    // Pattern updates every frame using X index + frame counter.
    pw.cld();
    pw.lda_immediate();
    pw(static_cast<Byte>(0x00));
    pw.sta_zero_page();
    pw(frame_counter_zp);

    const Address frame_loop = pw.addr;
    pw.ldx_immediate();
    pw(static_cast<Byte>(0x00));
    const Address pixel_loop = pw.addr;
    pw.txa();
    pw.clc();
    pw.adc_zero_page();
    pw(frame_counter_zp);
    pw.sta_absolute_x();
    pw(static_cast<Byte>(0x00));
    pw(static_cast<Byte>(0x02));
    pw.clc();
    pw.adc_immediate();
    pw(static_cast<Byte>(0x11));
    pw.sta_absolute_x();
    pw(static_cast<Byte>(0x00));
    pw(static_cast<Byte>(0x03));
    pw.clc();
    pw.adc_immediate();
    pw(static_cast<Byte>(0x11));
    pw.sta_absolute_x();
    pw(static_cast<Byte>(0x00));
    pw(static_cast<Byte>(0x04));
    pw.clc();
    pw.adc_immediate();
    pw(static_cast<Byte>(0x11));
    pw.sta_absolute_x();
    pw(static_cast<Byte>(0x00));
    pw(static_cast<Byte>(0x05));
    pw.inx();
    pw.bne();
    emit_relative_operand(pixel_loop);
    pw.inc_zero_page();
    pw(frame_counter_zp);
    pw.jmp_absolute();
    pw(static_cast<Byte>(frame_loop & 0x00FFu));
    pw(static_cast<Byte>(frame_loop >> 8));

    // Minimal interrupt handler for manual IRQ/NMI triggers in the UI.
    global.cpu.mem[interrupt_handler] = static_cast<Byte>(0x40); // RTI
    global.cpu.mem[0xFFFA] = static_cast<Byte>(interrupt_handler & 0x00FFu);
    global.cpu.mem[0xFFFB] = static_cast<Byte>(interrupt_handler >> 8);
    global.cpu.mem[0xFFFE] = static_cast<Byte>(interrupt_handler & 0x00FFu);
    global.cpu.mem[0xFFFF] = static_cast<Byte>(interrupt_handler >> 8);

    global.cpu.PC = start;
}

auto main() -> int {
    println("Application starting");
    if (!ENGINE::setup()) {
        println("Engine setup failed");
        return EXIT_FAILURE;
    }
    println("Engine setup complete");

    mos6502::initialize_instructions();
    load_example_framebuffer_demo();

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
