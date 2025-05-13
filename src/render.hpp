/* danielsinkin97@gmail.com */
#pragma once

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl.h>
#include <glad/glad.h>
#include <imgui.h>

#include "global.hpp"
#include "utils.hpp"

namespace RENDER {
inline auto display_grid() -> void {
    { // 6502-Emulator
        ImGui::Begin("6502-Emulator");
        ImGui::Text("PC   0x%04X", global.cpu.PC);
        ImGui::Text("A    0x%02X", global.cpu.A);
        ImGui::Text("X    0x%02X", global.cpu.X);
        ImGui::Text("Y    0x%02X", global.cpu.Y);
        ImGui::Text("SP   0x%02X", global.cpu.SP);
        ImGui::Text("P    0x%02X", global.cpu.P);
        ImGui::Text("NMI  %s", global.cpu.nmi ? "true" : "false");
        ImGui::Text("IRQ  %s", global.cpu.irq ? "true" : "false");
        ImGui::Text("SYNC %s", global.cpu.sync ? "true" : "false");
        ImGui::Text("RDY  %s", global.cpu.rdy ? "true" : "false");
        ImGui::Text("DB   0x%02X", global.cpu.data_bus);
        ImGui::Text("RW   0x%02X", global.cpu.rw);
        ImGui::End();
    }
}

inline auto gui_debug() -> void {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(global.renderer.window);
    ImGui::NewFrame();

    ImGui::Begin("Debug");
    ImGui::ColorEdit3("Background", &global.color.background.r);
    ImGui::ColorEdit3("Pixel On", &global.color.pixel_on.r);
    ImGui::ColorEdit3("Pixel Off", &global.color.pixel_off.r);
    ImGui::Text("Frame Counter: %d", global.sim.frame_counter);
    ImGui::Text("Runtime: %s",
        format_duration(global.sim.total_runtime).c_str());
    ImGui::Text("Delta Time (ms): %.3f", global.sim.delta_time.count());
    ImGui::Text("Mouse Position: (%.3f, %.3f)",
        global.input.mouse_pos.x,
        global.input.mouse_pos.y);
    ImGui::End();

    display_grid();
    ImGui::Render();
}

inline auto frame() -> void {
    glViewport(0, 0,
        (int)global.renderer.imgui_io.DisplaySize.x,
        (int)global.renderer.imgui_io.DisplaySize.y);
    glClearColor(global.color.background.r,
        global.color.background.g,
        global.color.background.b,
        1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}
} // namespace RENDER