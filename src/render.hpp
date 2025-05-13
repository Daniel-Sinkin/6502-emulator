#pragma once

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl.h>
#include <glad/glad.h>
#include <imgui.h>

#include "global.hpp"
#include "utils.hpp"

namespace RENDER {
inline auto cpu_register(mos6502::CPU &cpu) -> void {
    ImGui::Text("Registers");
    if (ImGui::BeginTable("cpu_registers", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("PC   0x%04X", cpu.PC);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("A    0x%02X", cpu.A);
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("X    0x%02X", cpu.X);
        ImGui::TableSetColumnIndex(3);
        ImGui::Text("Y    0x%02X", cpu.Y);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("SP   0x%02X", cpu.SP);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("P    0x%02X", cpu.P);
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("ADDR 0x%04X", cpu.addr);
        ImGui::TableSetColumnIndex(3);
        ImGui::Text("DB   0x%02X", cpu.data_bus);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("RW   0x%02X", cpu.rw);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("NMI  %s", cpu.nmi ? "true" : "false");
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("IRQ  %s", cpu.irq ? "true" : "false");
        ImGui::TableSetColumnIndex(3);
        ImGui::Text("SYNC %s", cpu.sync ? "true" : "false");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("RDY  %s", cpu.rdy ? "true" : "false");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("TMP  %d", cpu.tmp);
        ImGui::EndTable();
    }
    ImGui::Text(
        "%s %s (0x%02X) [%d]",
        mos6502::to_string(cpu.instr.mode),
        mos6502::to_string(cpu.instr.type),
        cpu.instr.opcode,
        cpu.instr_counter);
}
inline auto gui_debug() -> void {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(global.renderer.window);
    ImGui::NewFrame();

    /* ------------------------------------------------------------------ Debug */
    ImGui::Begin("Debug");
    ImGui::ColorEdit3("Background", &global.color.background.r);
    ImGui::ColorEdit3("Pixel On", &global.color.pixel_on.r);
    ImGui::ColorEdit3("Pixel Off", &global.color.pixel_off.r);
    ImGui::Text("Frame Counter: %d", global.sim.frame_counter);
    ImGui::Text("Runtime: %s",
        UTIL::format_duration(global.sim.total_runtime).c_str());
    ImGui::Text("Delta Time (ms): %.3f", global.sim.delta_time.count());
    ImGui::Text("Mouse Position: (%.3f, %.3f)",
        global.input.mouse_pos.x, global.input.mouse_pos.y);
    ImGui::Text("Is paused        %s", global.sim.is_debugging ? "true" : "false");
    ImGui::Text("Is Stepping      %s", global.sim.step_once ? "true" : "false");
    ImGui::Text("Is Back Stepping %s", global.sim.step_back ? "true" : "false");
    ImGui::Text("CPU snapshots %zu (%.2f MB)",
        global.cpu_snapshots.size(),
        UTIL::byte_to_mb(global.cpu_snapshots.size() *
                         sizeof(mos6502::CPUSnapshot)));
    ImGui::End();

    ImGui::Begin("CPU");
    cpu_register(global.cpu);
    ImGui::End();

    if (!global.cpu_snapshots.empty()) {
        ImGui::Begin("CPU (Snapshot)");
        cpu_register(global.cpu_snapshots.top().cpu);
        ImGui::End();
    }

    /* helper colors */
    constexpr ImU32 COLOR_PC = IM_COL32(255, 50, 50, 255);    // red
    constexpr ImU32 COLOR_ADDR = IM_COL32(50, 150, 255, 255); // blue
    constexpr ImU32 COLOR_BOTH = IM_COL32(255, 175, 0, 255);  // orange

    auto draw_memory_window = [&](const char *title,
                                  uint16_t center_addr,
                                  int display_lines) {
        ImGui::Begin(title);

        constexpr int BYTES_PER_LINE = 16;
        const int mem_size = static_cast<int>(global.cpu.mem.size());
        const int max_lines = (mem_size + BYTES_PER_LINE - 1) / BYTES_PER_LINE;
        const int center_ln = center_addr / BYTES_PER_LINE;
        const int half_ln = display_lines / 2;

        int start_ln = center_ln - half_ln;
        if (start_ln < 0) start_ln = 0;
        if (start_ln + display_lines > max_lines)
            start_ln = max_lines - display_lines;
        if (start_ln < 0) start_ln = 0;

        for (int line = start_ln; line < start_ln + display_lines; ++line) {
            const int base = line * BYTES_PER_LINE;
            ImGui::Text("0x%04X:", base);
            ImGui::SameLine();
            for (int j = 0; j < BYTES_PER_LINE; ++j) {
                const int idx = base + j;
                if (idx >= mem_size) break;

                bool is_pc = (idx == global.cpu.PC);
                bool is_addr = (idx == global.cpu.addr);

                if (is_pc && is_addr)
                    ImGui::PushStyleColor(ImGuiCol_Text, COLOR_BOTH);
                else if (is_pc)
                    ImGui::PushStyleColor(ImGuiCol_Text, COLOR_PC);
                else if (is_addr)
                    ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ADDR);

                ImGui::Text("%02X", global.cpu.mem[idx]);

                if (is_pc || is_addr) ImGui::PopStyleColor();

                if (j < BYTES_PER_LINE - 1) ImGui::SameLine();
            }
        }
        ImGui::End();
    };

    /* Full PC-centered memory view */
    draw_memory_window("Memory (around PC)", global.cpu.PC, 16);

    /* Compact ADDR-centered view */
    draw_memory_window("Addr Memory Neighborhood", global.cpu.addr, 6);

    /* ImGui Render */
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
