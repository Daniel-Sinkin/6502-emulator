#pragma once

#include <cassert>
#include <print>

using std::println;

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl.h"
#include "imgui.h"
#include <SDL.h>
#include <glad/glad.h>

#include "global.hpp"
#include "utils.hpp"

namespace ENGINE {
[[nodiscard]] inline auto setup() -> bool {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        println(std::cerr, "{}", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

    global.renderer.window = SDL_CreateWindow(
        CONSTANTS::window_title.data(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        CONSTANTS::window_width, CONSTANTS::window_height,
        SDL_WINDOW_OPENGL);
    if (!global.renderer.window) {
        println(std::cerr, "{}", SDL_GetError());
        SDL_Quit();
        return false;
    }

    global.renderer.gl_context = SDL_GL_CreateContext(global.renderer.window);
    if (!global.renderer.gl_context) {
        println(std::cerr, "{}", SDL_GetError());
        SDL_DestroyWindow(global.renderer.window);
        SDL_Quit();
        return false;
    }

    SDL_GL_MakeCurrent(global.renderer.window, global.renderer.gl_context);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader(static_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        println(std::cerr, "{}", SDL_GetError());
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    global.renderer.imgui_io = io;

    ImFontConfig font_cfg;
    font_cfg.OversampleH = 3;
    font_cfg.OversampleV = 3;

    float dpi_scale = 1.0f;
    float ddpi;
    if (SDL_GetDisplayDPI(0, &ddpi, nullptr, nullptr) == 0 && ddpi > 0.0f) {
        dpi_scale = ddpi / 96.0f;
        // if (dpi_scale > 1.5f) dpi_scale = 1.5f; // Cap it for Retina
    }

    constexpr float base_font_size = 6.0f;
    ImFont *mono_font = io.Fonts->AddFontFromFileTTF(
        "assets/fonts/MonaspaceKrypton-Regular.otf",
        base_font_size * dpi_scale,
        &font_cfg);

    if (mono_font) {
        io.FontDefault = mono_font;
    }

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(global.renderer.window, global.renderer.gl_context);
    ImGui_ImplOpenGL3_Init("#version 410 core");

    return true;
}

inline auto cleanup() -> void {
    println("Cleaning up engine resources");

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(global.renderer.gl_context);
    SDL_DestroyWindow(global.renderer.window);
    SDL_Quit();
}
} // namespace ENGINE