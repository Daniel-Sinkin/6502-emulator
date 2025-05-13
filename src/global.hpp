#pragma once

#include <stack>

#include <SDL.h>
#include <chrono>
#include <imgui.h>

#include "6502/6502.hpp"
#include "constants.hpp"
#include "gl.hpp"
#include "types.hpp"

using TYPES::Position;
using TYPES::COLOR::Color;

struct RendererState {
    SDL_Window *window = nullptr;
    SDL_GLContext gl_context = nullptr;
    ImGuiIO imgui_io;

    GL::GeometryBuffers geom_square;
    GL::GeometryBuffers geom_circle;
    GL::GeometryBuffers geom_triangle;

    GL::GeometryBuffers blit_quad;
    GLuint chip8_texture = 0;
    GL::ShaderProgram blit_shader;

    int gl_success;
    char gl_error_buffer[512];

    auto panic_gl(const char *reason) -> void {
        std::cerr << reason << "\n"
                  << gl_error_buffer << "\n";
        PANIC("GL Error");
    }
};

struct SimulationState {
    int frame_counter = 0;
    std::chrono::steady_clock::time_point run_start_time;
    std::chrono::steady_clock::time_point frame_start_time;
    std::chrono::duration<float> delta_time;
    std::chrono::duration<float> total_runtime;
    bool is_debugging = false;
    bool step_once = false;
    bool step_back = false;

    auto validate() -> void {
        if (step_once && !is_debugging) PANIC("Step once true but not debugging!");
    }
};

struct InputState {
    Position mouse_pos;
};

struct ColorPalette {
    Color background = TYPES::COLOR::from_u8(15, 15, 21);
    Color pixel_on = Color{1.0f, 1.0f, 1.0f};
    Color pixel_off = Color{0.0f, 0.0f, 0.0f};
};

struct AudioState {
    Mix_Chunk *beep_sound = nullptr;
};

struct Global {
    bool is_running = false;
    RendererState renderer;
    SimulationState sim;
    InputState input;
    ColorPalette color;
    AudioState audio;
    mos6502::CPU cpu;

    std::stack<mos6502::CPUSnapshot> cpu_snapshots;

    auto validate() -> void {
        sim.validate();
    }
    auto debug_activate() -> void {
        sim.is_debugging = true;
        color.background = CONSTANTS::COLOR::background_debug;
    }
    auto debug_deactivate() -> void {
        sim.is_debugging = true;
        sim.step_once = false;
        sim.step_back = false;
        color.background = CONSTANTS::COLOR::background;
    }
};
inline Global global;