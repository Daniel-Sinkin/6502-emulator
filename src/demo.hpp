/* danielsinkin97@gmail.com */
#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include "6502/program_writer.hpp"
#include "global.hpp"

namespace DEMO {
enum class Mode {
    framebuffer_pattern,
    snake,
};

inline constexpr Address demo_start = static_cast<Address>(0x0600u);
inline constexpr Address interrupt_handler = static_cast<Address>(0x0700u);

struct SnakeState {
    static constexpr int width = 32;
    static constexpr int height = 32;
    static constexpr int cell_count = width * height;
    static constexpr Address framebuffer_base = static_cast<Address>(0x0200u);
    static constexpr Address input_dir_addr = static_cast<Address>(0x00F0u);
    static constexpr Address input_tick_addr = static_cast<Address>(0x00F1u);
    static constexpr Address score_addr = static_cast<Address>(0x00F2u);
    static constexpr Address game_over_addr = static_cast<Address>(0x00F3u);

    bool active = false;
    bool paused = false;
    bool game_over = false;
    std::size_t length = 0;
    int score = 0;
    int dir_x = 1;
    int dir_y = 0;
    int queued_dir_x = 1;
    int queued_dir_y = 0;
    uint16_t food_cell = 0;
    std::array<uint16_t, static_cast<std::size_t>(cell_count)> body{};
    std::chrono::steady_clock::time_point last_step{};
    std::chrono::milliseconds step_interval{120};
};

inline Mode mode = Mode::framebuffer_pattern;
inline SnakeState snake{};
inline uint32_t rng_state = 0xA5A5F00Du;

inline auto install_interrupt_handler() -> void {
    global.cpu.mem[interrupt_handler] = static_cast<Byte>(0x40u); // RTI
    global.cpu.mem[0xFFFA] = static_cast<Byte>(interrupt_handler & 0x00FFu);
    global.cpu.mem[0xFFFB] = static_cast<Byte>(interrupt_handler >> 8u);
    global.cpu.mem[0xFFFE] = static_cast<Byte>(interrupt_handler & 0x00FFu);
    global.cpu.mem[0xFFFF] = static_cast<Byte>(interrupt_handler >> 8u);
}

inline auto reset_debug_step_flags() -> void {
    global.sim.step_once = false;
    global.sim.step_back = false;
    global.sim.step_forward = false;
    global.cpu_history.clear();
}

inline auto load_framebuffer_pattern_demo() -> void {
    global.cpu = mos6502::CPU{};
    reset_debug_step_flags();

    constexpr Byte frame_counter_zp = static_cast<Byte>(0x10u);
    auto pw = mos6502::ProgramWriter(global.cpu, demo_start);

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
    pw(static_cast<Byte>(0x00u));
    pw.sta_zero_page();
    pw(frame_counter_zp);

    const Address frame_loop = pw.addr;
    pw.ldx_immediate();
    pw(static_cast<Byte>(0x00u));
    const Address pixel_loop = pw.addr;
    pw.txa();
    pw.clc();
    pw.adc_zero_page();
    pw(frame_counter_zp);
    pw.sta_absolute_x();
    pw(static_cast<Byte>(0x00u));
    pw(static_cast<Byte>(0x02u));
    pw.clc();
    pw.adc_immediate();
    pw(static_cast<Byte>(0x11u));
    pw.sta_absolute_x();
    pw(static_cast<Byte>(0x00u));
    pw(static_cast<Byte>(0x03u));
    pw.clc();
    pw.adc_immediate();
    pw(static_cast<Byte>(0x11u));
    pw.sta_absolute_x();
    pw(static_cast<Byte>(0x00u));
    pw(static_cast<Byte>(0x04u));
    pw.clc();
    pw.adc_immediate();
    pw(static_cast<Byte>(0x11u));
    pw.sta_absolute_x();
    pw(static_cast<Byte>(0x00u));
    pw(static_cast<Byte>(0x05u));
    pw.inx();
    pw.bne();
    emit_relative_operand(pixel_loop);
    pw.inc_zero_page();
    pw(frame_counter_zp);
    pw.jmp_absolute();
    pw(static_cast<Byte>(frame_loop & 0x00FFu));
    pw(static_cast<Byte>(frame_loop >> 8u));

    install_interrupt_handler();
    global.cpu.PC = demo_start;

    mode = Mode::framebuffer_pattern;
}

[[nodiscard]] inline auto snake_cell_to_addr(uint16_t cell) -> Address {
    return static_cast<Address>(SnakeState::framebuffer_base + cell);
}

inline auto snake_clear_framebuffer() -> void {
    constexpr Byte color_empty = static_cast<Byte>(0x00u);
    for (int cell = 0; cell < SnakeState::cell_count; ++cell) {
        global.cpu.mem[snake_cell_to_addr(static_cast<uint16_t>(cell))] = color_empty;
    }
}

[[nodiscard]] inline auto snake_direction_to_byte(int dx, int dy) -> Byte {
    if (dx == 0 && dy == -1) return static_cast<Byte>(1u); // up
    if (dx == 1 && dy == 0) return static_cast<Byte>(2u);  // right
    if (dx == 0 && dy == 1) return static_cast<Byte>(3u);  // down
    if (dx == -1 && dy == 0) return static_cast<Byte>(4u); // left
    return static_cast<Byte>(0u);
}

[[nodiscard]] inline auto
snake_cell_occupied(uint16_t cell, std::size_t count) -> bool {
    for (std::size_t i = 0; i < count; ++i) {
        if (snake.body[i] == cell) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] inline auto snake_random_u32() -> uint32_t {
    rng_state = (rng_state * 1664525u) + 1013904223u;
    return rng_state;
}

inline auto snake_draw() -> void {
    constexpr Byte color_body = static_cast<Byte>(0x0Cu);
    constexpr Byte color_head = static_cast<Byte>(0x3Fu);
    constexpr Byte color_food = static_cast<Byte>(0x33u);
    constexpr Byte color_crash = static_cast<Byte>(0x03u);

    snake_clear_framebuffer();
    if (!snake.active || snake.length == 0u) {
        return;
    }

    for (std::size_t i = 0; i < snake.length; ++i) {
        const Address addr = snake_cell_to_addr(snake.body[i]);
        global.cpu.mem[addr] = static_cast<Byte>(i == 0u ? color_head : color_body);
    }
    if (snake.game_over) {
        global.cpu.mem[snake_cell_to_addr(snake.body[0])] = color_crash;
    } else {
        global.cpu.mem[snake_cell_to_addr(snake.food_cell)] = color_food;
    }
}

inline auto snake_place_food() -> void {
    if (snake.length >= snake.body.size()) {
        snake.game_over = true;
        global.cpu.mem[SnakeState::game_over_addr] = static_cast<Byte>(1u);
        return;
    }

    const uint16_t start = static_cast<uint16_t>(snake_random_u32() % SnakeState::cell_count);
    for (int offset = 0; offset < SnakeState::cell_count; ++offset) {
        const uint16_t cell = static_cast<uint16_t>((start + offset) % SnakeState::cell_count);
        if (!snake_cell_occupied(cell, snake.length)) {
            snake.food_cell = cell;
            return;
        }
    }

    snake.game_over = true;
    global.cpu.mem[SnakeState::game_over_addr] = static_cast<Byte>(1u);
}

inline auto reset_snake() -> void {
    const auto preserved_interval = snake.step_interval;
    snake = SnakeState{};
    snake.step_interval = preserved_interval;
    snake.active = true;
    snake.length = 4u;
    snake.dir_x = 1;
    snake.dir_y = 0;
    snake.queued_dir_x = 1;
    snake.queued_dir_y = 0;
    snake.last_step = std::chrono::steady_clock::now();

    const int center_x = SnakeState::width / 2;
    const int center_y = SnakeState::height / 2;
    for (std::size_t i = 0; i < snake.length; ++i) {
        snake.body[i] = static_cast<uint16_t>((center_y * SnakeState::width) + (center_x - static_cast<int>(i)));
    }

    global.cpu.mem[SnakeState::input_dir_addr] = snake_direction_to_byte(snake.dir_x, snake.dir_y);
    global.cpu.mem[SnakeState::input_tick_addr] = static_cast<Byte>(0u);
    global.cpu.mem[SnakeState::score_addr] = static_cast<Byte>(0u);
    global.cpu.mem[SnakeState::game_over_addr] = static_cast<Byte>(0u);

    snake_place_food();
    snake_draw();
}

inline auto load_snake_demo() -> void {
    global.cpu = mos6502::CPU{};
    reset_debug_step_flags();

    auto pw = mos6502::ProgramWriter(global.cpu, demo_start);
    pw.jmp_absolute();
    pw(static_cast<Byte>(demo_start & 0x00FFu));
    pw(static_cast<Byte>(demo_start >> 8u));

    install_interrupt_handler();
    global.cpu.PC = demo_start;

    const uint32_t frame_seed = static_cast<uint32_t>(std::max(global.sim.frame_counter, 0));
    rng_state ^= (frame_seed + 1u) * 2654435761u;
    mode = Mode::snake;
    reset_snake();
}

inline auto load_mode(Mode new_mode) -> void {
    if (new_mode == Mode::snake) {
        load_snake_demo();
        return;
    }
    load_framebuffer_pattern_demo();
}

inline auto reset_active_mode() -> void {
    load_mode(mode);
}

[[nodiscard]] inline auto active_mode() -> Mode {
    return mode;
}

[[nodiscard]] inline auto active_mode_label() -> const char * {
    return mode == Mode::snake ? "Snake (Host I/O)" : "Pattern Program (6502)";
}

[[nodiscard]] inline auto snake_state() -> const SnakeState & {
    return snake;
}

[[nodiscard]] inline auto snake_tick_ms() -> int {
    return static_cast<int>(snake.step_interval.count());
}

inline auto set_snake_tick_ms(int ms) -> void {
    snake.step_interval = std::chrono::milliseconds(std::clamp(ms, 40, 400));
}

inline auto toggle_snake_pause() -> void {
    snake.paused = !snake.paused;
    if (!snake.paused) {
        snake.last_step = std::chrono::steady_clock::now();
    }
}

inline auto request_snake_direction(int dx, int dy) -> void {
    if (mode != Mode::snake || !snake.active || snake.game_over) {
        return;
    }
    if ((dx == 0 && dy == 0) || (dx == -snake.dir_x && dy == -snake.dir_y)) {
        return;
    }
    snake.queued_dir_x = dx;
    snake.queued_dir_y = dy;
    global.cpu.mem[SnakeState::input_dir_addr] = snake_direction_to_byte(dx, dy);
}

inline auto advance_snake_one_step() -> void {
    if (!snake.active || snake.game_over || snake.paused || snake.length == 0u) {
        return;
    }

    snake.dir_x = snake.queued_dir_x;
    snake.dir_y = snake.queued_dir_y;

    const uint16_t head_cell = snake.body[0];
    const int head_x = static_cast<int>(head_cell % SnakeState::width);
    const int head_y = static_cast<int>(head_cell / SnakeState::width);

    const int next_x = head_x + snake.dir_x;
    const int next_y = head_y + snake.dir_y;
    if (next_x < 0 || next_x >= SnakeState::width ||
        next_y < 0 || next_y >= SnakeState::height) {
        snake.game_over = true;
        global.cpu.mem[SnakeState::game_over_addr] = static_cast<Byte>(1u);
        snake_draw();
        return;
    }

    const uint16_t next_cell = static_cast<uint16_t>((next_y * SnakeState::width) + next_x);
    const bool grows = (next_cell == snake.food_cell);
    const std::size_t collision_limit = snake.length - (grows ? 0u : 1u);
    if (snake_cell_occupied(next_cell, collision_limit)) {
        snake.game_over = true;
        global.cpu.mem[SnakeState::game_over_addr] = static_cast<Byte>(1u);
        snake_draw();
        return;
    }

    if (grows && snake.length < snake.body.size()) {
        ++snake.length;
    }
    for (std::size_t i = snake.length - 1u; i > 0u; --i) {
        snake.body[i] = snake.body[i - 1u];
    }
    snake.body[0] = next_cell;

    if (grows) {
        ++snake.score;
        global.cpu.mem[SnakeState::score_addr] = static_cast<Byte>(std::min(snake.score, 255));
        snake_place_food();
    }
    global.cpu.mem[SnakeState::input_tick_addr] = static_cast<Byte>(global.cpu.mem[SnakeState::input_tick_addr] + 1u);
    snake_draw();
}

inline auto update() -> void {
    if (mode != Mode::snake || !snake.active || snake.game_over || snake.paused) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if ((now - snake.last_step) < snake.step_interval) {
        return;
    }
    snake.last_step = now;
    advance_snake_one_step();
}
} // namespace DEMO
