/* danielsinkin97@gmail.com */
#pragma once

#include <cassert>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include "6502/program_writer.hpp"
#include "global.hpp"

namespace DEMO {
inline constexpr Address demo_start = static_cast<Address>(0x0600u);
inline constexpr Address interrupt_handler = static_cast<Address>(0x0F00u);

inline constexpr Address input_dir_addr = static_cast<Address>(0x00F0u);  // 1=up 2=right 3=down 4=left
inline constexpr Address input_tick_addr = static_cast<Address>(0x00F1u); // host increments each game step
inline constexpr Address score_out_addr = static_cast<Address>(0x00F2u);  // score mirror for UI/debug
inline constexpr Address state_out_addr = static_cast<Address>(0x00F3u);  // 0=running 1=game-over

inline constexpr std::chrono::milliseconds snake_tick_interval{120};
inline std::chrono::steady_clock::time_point last_host_tick_time{};

inline auto install_interrupt_handler() -> void {
    global.cpu.mem[interrupt_handler] = static_cast<Byte>(0x40u); // RTI
    global.cpu.mem[0xFFFA] = static_cast<Byte>(interrupt_handler & 0x00FFu);
    global.cpu.mem[0xFFFB] = static_cast<Byte>(interrupt_handler >> 8u);
    global.cpu.mem[0xFFFE] = static_cast<Byte>(interrupt_handler & 0x00FFu);
    global.cpu.mem[0xFFFF] = static_cast<Byte>(interrupt_handler >> 8u);
}

inline auto reset_debug_state() -> void {
    global.sim.step_once = false;
    global.sim.step_back = false;
    global.sim.step_forward = false;
    global.sim.last_ticks_executed = 0;
    global.sim.last_instructions_executed = 0;
    global.sim.instructions_executed_total = 0;
    global.sim.cpu_tick_fractional_remainder = 0.0;
    global.cpu_history.clear();
}

inline auto set_direction(Byte dir) -> void {
    if (dir < static_cast<Byte>(1u) || dir > static_cast<Byte>(4u)) {
        return;
    }
    global.cpu.mem[input_dir_addr] = dir;
}

inline auto set_direction_up() -> void { set_direction(static_cast<Byte>(1u)); }
inline auto set_direction_right() -> void { set_direction(static_cast<Byte>(2u)); }
inline auto set_direction_down() -> void { set_direction(static_cast<Byte>(3u)); }
inline auto set_direction_left() -> void { set_direction(static_cast<Byte>(4u)); }

inline auto load_default_demo() -> void {
    global.cpu = mos6502::CPU{};
    reset_debug_state();

    auto pw = mos6502::ProgramWriter(global.cpu, demo_start);

    struct RelativeFixup {
        Address operand_addr;
        std::string label;
    };
    struct AbsoluteFixup {
        Address low_byte_addr;
        std::string label;
    };
    std::unordered_map<std::string, Address> labels;
    std::vector<RelativeFixup> rel_fixups;
    std::vector<AbsoluteFixup> abs_fixups;

    auto mark = [&](const std::string &label) -> void {
        labels[label] = pw.addr;
    };
    auto rel_to = [&](const std::string &label) -> void {
        rel_fixups.push_back({pw.addr, label});
        pw(static_cast<Byte>(0x00u));
    };
    auto jmp_to = [&](const std::string &label) -> void {
        pw.jmp_absolute();
        abs_fixups.push_back({pw.addr, label});
        pw(static_cast<Byte>(0x00u));
        pw(static_cast<Byte>(0x00u));
    };
    auto jsr_to = [&](const std::string &label) -> void {
        pw.jsr_absolute();
        abs_fixups.push_back({pw.addr, label});
        pw(static_cast<Byte>(0x00u));
        pw(static_cast<Byte>(0x00u));
    };

    // Zero-page variables used by the snake program.
    constexpr Byte HEAD_X = static_cast<Byte>(0x00u);
    constexpr Byte HEAD_Y = static_cast<Byte>(0x01u);
    constexpr Byte TAIL_X = static_cast<Byte>(0x02u);
    constexpr Byte TAIL_Y = static_cast<Byte>(0x03u);
    constexpr Byte DIR = static_cast<Byte>(0x04u);
    constexpr Byte LAST_TICK = static_cast<Byte>(0x05u);
    constexpr Byte RNG = static_cast<Byte>(0x06u);
    constexpr Byte TMP_X = static_cast<Byte>(0x07u);
    constexpr Byte TMP_Y = static_cast<Byte>(0x08u);
    constexpr Byte ADDR_LO = static_cast<Byte>(0x09u);
    constexpr Byte ADDR_HI = static_cast<Byte>(0x0Au);
    constexpr Byte NEW_X = static_cast<Byte>(0x0Bu);
    constexpr Byte NEW_Y = static_cast<Byte>(0x0Cu);
    constexpr Byte CELL = static_cast<Byte>(0x0Du);
    constexpr Byte SCORE = static_cast<Byte>(0x0Eu);
    constexpr Byte GROW = static_cast<Byte>(0x0Fu);

    constexpr Byte DIR_UP = static_cast<Byte>(0x01u);
    constexpr Byte DIR_RIGHT = static_cast<Byte>(0x02u);
    constexpr Byte DIR_DOWN = static_cast<Byte>(0x03u);
    constexpr Byte DIR_LEFT = static_cast<Byte>(0x04u);
    constexpr Byte BODY_COLOR_MASK = static_cast<Byte>(0x20u);
    constexpr Byte FOOD_VALUE = static_cast<Byte>(0x7Fu);
    constexpr Byte CRASH_VALUE = static_cast<Byte>(0x0Fu);

    // Program start: initialize host I/O and snake state.
    pw.lda_immediate();
    pw(DIR_RIGHT);
    pw.sta_absolute();
    pw(static_cast<Byte>(input_dir_addr & 0x00FFu));
    pw(static_cast<Byte>(input_dir_addr >> 8u));

    pw.lda_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.sta_absolute();
    pw(static_cast<Byte>(input_tick_addr & 0x00FFu));
    pw(static_cast<Byte>(input_tick_addr >> 8u));
    pw.sta_absolute();
    pw(static_cast<Byte>(score_out_addr & 0x00FFu));
    pw(static_cast<Byte>(score_out_addr >> 8u));
    pw.sta_absolute();
    pw(static_cast<Byte>(state_out_addr & 0x00FFu));
    pw(static_cast<Byte>(state_out_addr >> 8u));

    pw.lda_immediate();
    pw(static_cast<Byte>(16u));
    pw.sta_zero_page();
    pw(HEAD_X);
    pw.sta_zero_page();
    pw(HEAD_Y);
    pw.sta_zero_page();
    pw(TAIL_Y);

    pw.lda_immediate();
    pw(static_cast<Byte>(13u));
    pw.sta_zero_page();
    pw(TAIL_X);

    pw.lda_immediate();
    pw(DIR_RIGHT);
    pw.sta_zero_page();
    pw(DIR);

    pw.lda_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.sta_zero_page();
    pw(SCORE);
    pw.sta_zero_page();
    pw(GROW);

    pw.lda_immediate();
    pw(static_cast<Byte>(0x5Au));
    pw.sta_zero_page();
    pw(RNG);

    // Clear framebuffer $0200-$05FF.
    pw.lda_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.sta_zero_page();
    pw(ADDR_LO);

    pw.lda_immediate();
    pw(static_cast<Byte>(0x02u));
    pw.sta_zero_page();
    pw(ADDR_HI);

    mark("clear_loop");
    pw.ldy_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.lda_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.sta_indirect_y();
    pw(ADDR_LO);
    pw.inc_zero_page();
    pw(ADDR_LO);
    pw.bne();
    rel_to("clear_loop");
    pw.inc_zero_page();
    pw(ADDR_HI);
    pw.lda_zero_page();
    pw(ADDR_HI);
    pw.cmp_immediate();
    pw(static_cast<Byte>(0x06u));
    pw.bne();
    rel_to("clear_loop");

    // Initial snake body: x=13..16 at y=16, pointing right.
    pw.ldx_immediate();
    pw(static_cast<Byte>(13u));
    mark("init_body_loop");
    pw.ldy_immediate();
    pw(static_cast<Byte>(16u));
    jsr_to("coord_to_addr");
    pw.ldy_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.lda_immediate();
    pw(static_cast<Byte>(BODY_COLOR_MASK | DIR_RIGHT));
    pw.sta_indirect_y();
    pw(ADDR_LO);
    pw.inx();
    pw.cpx_immediate();
    pw(static_cast<Byte>(17u));
    pw.bne();
    rel_to("init_body_loop");

    // Place first food and sync last_tick.
    jsr_to("place_food");
    pw.lda_absolute();
    pw(static_cast<Byte>(input_tick_addr & 0x00FFu));
    pw(static_cast<Byte>(input_tick_addr >> 8u));
    pw.sta_zero_page();
    pw(LAST_TICK);

    mark("wait_tick");
    pw.lda_absolute();
    pw(static_cast<Byte>(input_tick_addr & 0x00FFu));
    pw(static_cast<Byte>(input_tick_addr >> 8u));
    pw.cmp_zero_page();
    pw(LAST_TICK);
    pw.beq();
    rel_to("wait_tick");
    pw.sta_zero_page();
    pw(LAST_TICK);

    // Read requested direction from host I/O and block direct reverse turns.
    pw.lda_absolute();
    pw(static_cast<Byte>(input_dir_addr & 0x00FFu));
    pw(static_cast<Byte>(input_dir_addr >> 8u));
    pw.cmp_immediate();
    pw(DIR_UP);
    pw.beq();
    rel_to("req_up");
    pw.cmp_immediate();
    pw(DIR_RIGHT);
    pw.beq();
    rel_to("req_right");
    pw.cmp_immediate();
    pw(DIR_DOWN);
    pw.beq();
    rel_to("req_down");
    pw.cmp_immediate();
    pw(DIR_LEFT);
    pw.beq();
    rel_to("req_left");
    jmp_to("after_input");

    mark("req_up");
    pw.lda_zero_page();
    pw(DIR);
    pw.cmp_immediate();
    pw(DIR_DOWN);
    pw.beq();
    rel_to("after_input");
    pw.lda_immediate();
    pw(DIR_UP);
    pw.sta_zero_page();
    pw(DIR);
    jmp_to("after_input");

    mark("req_right");
    pw.lda_zero_page();
    pw(DIR);
    pw.cmp_immediate();
    pw(DIR_LEFT);
    pw.beq();
    rel_to("after_input");
    pw.lda_immediate();
    pw(DIR_RIGHT);
    pw.sta_zero_page();
    pw(DIR);
    jmp_to("after_input");

    mark("req_down");
    pw.lda_zero_page();
    pw(DIR);
    pw.cmp_immediate();
    pw(DIR_UP);
    pw.beq();
    rel_to("after_input");
    pw.lda_immediate();
    pw(DIR_DOWN);
    pw.sta_zero_page();
    pw(DIR);
    jmp_to("after_input");

    mark("req_left");
    pw.lda_zero_page();
    pw(DIR);
    pw.cmp_immediate();
    pw(DIR_RIGHT);
    pw.beq();
    rel_to("after_input");
    pw.lda_immediate();
    pw(DIR_LEFT);
    pw.sta_zero_page();
    pw(DIR);

    mark("after_input");
    // new = head
    pw.lda_zero_page();
    pw(HEAD_X);
    pw.sta_zero_page();
    pw(NEW_X);
    pw.lda_zero_page();
    pw(HEAD_Y);
    pw.sta_zero_page();
    pw(NEW_Y);

    pw.lda_zero_page();
    pw(DIR);
    pw.cmp_immediate();
    pw(DIR_UP);
    pw.beq();
    rel_to("move_up");
    pw.cmp_immediate();
    pw(DIR_RIGHT);
    pw.beq();
    rel_to("move_right");
    pw.cmp_immediate();
    pw(DIR_DOWN);
    pw.beq();
    rel_to("move_down");

    // move left
    mark("move_left");
    pw.lda_zero_page();
    pw(NEW_X);
    pw.bne();
    rel_to("move_left_ok");
    jmp_to("game_over");
    mark("move_left_ok");
    pw.sec();
    pw.sbc_immediate();
    pw(static_cast<Byte>(1u));
    pw.sta_zero_page();
    pw(NEW_X);
    jmp_to("after_move");

    mark("move_right");
    pw.lda_zero_page();
    pw(NEW_X);
    pw.cmp_immediate();
    pw(static_cast<Byte>(31u));
    pw.bne();
    rel_to("move_right_ok");
    jmp_to("game_over");
    mark("move_right_ok");
    pw.clc();
    pw.adc_immediate();
    pw(static_cast<Byte>(1u));
    pw.sta_zero_page();
    pw(NEW_X);
    jmp_to("after_move");

    mark("move_down");
    pw.lda_zero_page();
    pw(NEW_Y);
    pw.cmp_immediate();
    pw(static_cast<Byte>(31u));
    pw.bne();
    rel_to("move_down_ok");
    jmp_to("game_over");
    mark("move_down_ok");
    pw.clc();
    pw.adc_immediate();
    pw(static_cast<Byte>(1u));
    pw.sta_zero_page();
    pw(NEW_Y);
    jmp_to("after_move");

    mark("move_up");
    pw.lda_zero_page();
    pw(NEW_Y);
    pw.bne();
    rel_to("move_up_ok");
    jmp_to("game_over");
    mark("move_up_ok");
    pw.sec();
    pw.sbc_immediate();
    pw(static_cast<Byte>(1u));
    pw.sta_zero_page();
    pw(NEW_Y);

    mark("after_move");
    pw.ldx_zero_page();
    pw(NEW_X);
    pw.ldy_zero_page();
    pw(NEW_Y);
    jsr_to("coord_to_addr");
    pw.ldy_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.lda_indirect_y();
    pw(ADDR_LO);
    pw.sta_zero_page();
    pw(CELL);
    pw.beq();
    rel_to("cell_empty");
    pw.cmp_immediate();
    pw(FOOD_VALUE);
    pw.beq();
    rel_to("ate_food");
    jmp_to("game_over");

    mark("cell_empty");
    pw.lda_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.sta_zero_page();
    pw(GROW);
    jsr_to("move_tail");
    jmp_to("commit_head");

    mark("ate_food");
    pw.lda_immediate();
    pw(static_cast<Byte>(0x01u));
    pw.sta_zero_page();
    pw(GROW);
    pw.inc_zero_page();
    pw(SCORE);
    pw.lda_zero_page();
    pw(SCORE);
    pw.sta_absolute();
    pw(static_cast<Byte>(score_out_addr & 0x00FFu));
    pw(static_cast<Byte>(score_out_addr >> 8u));

    mark("commit_head");
    // old head becomes body segment pointing at the new head.
    pw.ldx_zero_page();
    pw(HEAD_X);
    pw.ldy_zero_page();
    pw(HEAD_Y);
    jsr_to("coord_to_addr");
    pw.ldy_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.lda_zero_page();
    pw(DIR);
    pw.ora_immediate();
    pw(BODY_COLOR_MASK);
    pw.sta_indirect_y();
    pw(ADDR_LO);

    // write new head position
    pw.lda_zero_page();
    pw(NEW_X);
    pw.sta_zero_page();
    pw(HEAD_X);
    pw.lda_zero_page();
    pw(NEW_Y);
    pw.sta_zero_page();
    pw(HEAD_Y);

    pw.ldx_zero_page();
    pw(HEAD_X);
    pw.ldy_zero_page();
    pw(HEAD_Y);
    jsr_to("coord_to_addr");
    pw.ldy_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.lda_zero_page();
    pw(DIR);
    pw.ora_immediate();
    pw(BODY_COLOR_MASK);
    pw.sta_indirect_y();
    pw(ADDR_LO);

    pw.lda_zero_page();
    pw(GROW);
    pw.beq();
    rel_to("skip_place_food_after_commit");
    jsr_to("place_food");
    mark("skip_place_food_after_commit");

    jmp_to("wait_tick");

    mark("game_over");
    pw.lda_immediate();
    pw(static_cast<Byte>(1u));
    pw.sta_absolute();
    pw(static_cast<Byte>(state_out_addr & 0x00FFu));
    pw(static_cast<Byte>(state_out_addr >> 8u));

    pw.ldx_zero_page();
    pw(HEAD_X);
    pw.ldy_zero_page();
    pw(HEAD_Y);
    jsr_to("coord_to_addr");
    pw.ldy_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.lda_immediate();
    pw(CRASH_VALUE);
    pw.sta_indirect_y();
    pw(ADDR_LO);

    mark("halt");
    jmp_to("halt");

    // ------------------------------------------------------- Subroutines

    // coord_to_addr: input X=x, Y=y, output ADDR_LO/ADDR_HI = $0200 + y*32 + x
    mark("coord_to_addr");
    pw.stx_zero_page();
    pw(TMP_X);
    pw.sty_zero_page();
    pw(TMP_Y);
    pw.lda_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.sta_zero_page();
    pw(ADDR_HI);
    pw.lda_zero_page();
    pw(TMP_Y);
    pw.asl_accumulator();
    pw.rol_zero_page();
    pw(ADDR_HI);
    pw.asl_accumulator();
    pw.rol_zero_page();
    pw(ADDR_HI);
    pw.asl_accumulator();
    pw.rol_zero_page();
    pw(ADDR_HI);
    pw.asl_accumulator();
    pw.rol_zero_page();
    pw(ADDR_HI);
    pw.asl_accumulator();
    pw.rol_zero_page();
    pw(ADDR_HI);
    pw.clc();
    pw.adc_zero_page();
    pw(TMP_X);
    pw.sta_zero_page();
    pw(ADDR_LO);
    pw.lda_zero_page();
    pw(ADDR_HI);
    pw.adc_immediate();
    pw(static_cast<Byte>(0x02u));
    pw.sta_zero_page();
    pw(ADDR_HI);
    pw.rts();

    // move_tail: clear old tail pixel and advance tail by stored cell direction.
    mark("move_tail");
    pw.ldx_zero_page();
    pw(TAIL_X);
    pw.ldy_zero_page();
    pw(TAIL_Y);
    jsr_to("coord_to_addr");
    pw.ldy_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.lda_indirect_y();
    pw(ADDR_LO);
    pw.and_immediate();
    pw(static_cast<Byte>(0x0Fu));
    pw.sta_zero_page();
    pw(CELL);
    pw.lda_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.sta_indirect_y();
    pw(ADDR_LO);

    pw.lda_zero_page();
    pw(CELL);
    pw.cmp_immediate();
    pw(DIR_UP);
    pw.beq();
    rel_to("tail_up");
    pw.cmp_immediate();
    pw(DIR_RIGHT);
    pw.beq();
    rel_to("tail_right");
    pw.cmp_immediate();
    pw(DIR_DOWN);
    pw.beq();
    rel_to("tail_down");

    // left
    pw.lda_zero_page();
    pw(TAIL_X);
    pw.sec();
    pw.sbc_immediate();
    pw(static_cast<Byte>(1u));
    pw.sta_zero_page();
    pw(TAIL_X);
    pw.rts();

    mark("tail_right");
    pw.lda_zero_page();
    pw(TAIL_X);
    pw.clc();
    pw.adc_immediate();
    pw(static_cast<Byte>(1u));
    pw.sta_zero_page();
    pw(TAIL_X);
    pw.rts();

    mark("tail_down");
    pw.lda_zero_page();
    pw(TAIL_Y);
    pw.clc();
    pw.adc_immediate();
    pw(static_cast<Byte>(1u));
    pw.sta_zero_page();
    pw(TAIL_Y);
    pw.rts();

    mark("tail_up");
    pw.lda_zero_page();
    pw(TAIL_Y);
    pw.sec();
    pw.sbc_immediate();
    pw(static_cast<Byte>(1u));
    pw.sta_zero_page();
    pw(TAIL_Y);
    pw.rts();

    // place_food: pseudo-random search for an empty cell and write FOOD_VALUE.
    mark("place_food");
    pw.lda_zero_page();
    pw(RNG);
    pw.clc();
    pw.adc_immediate();
    pw(static_cast<Byte>(17u));
    pw.sta_zero_page();
    pw(RNG);

    mark("food_try");
    pw.lda_zero_page();
    pw(RNG);
    pw.and_immediate();
    pw(static_cast<Byte>(0x1Fu));
    pw.sta_zero_page();
    pw(NEW_X);

    pw.lda_zero_page();
    pw(RNG);
    pw.lsr_accumulator();
    pw.lsr_accumulator();
    pw.lsr_accumulator();
    pw.and_immediate();
    pw(static_cast<Byte>(0x1Fu));
    pw.sta_zero_page();
    pw(NEW_Y);

    pw.ldx_zero_page();
    pw(NEW_X);
    pw.ldy_zero_page();
    pw(NEW_Y);
    jsr_to("coord_to_addr");
    pw.ldy_immediate();
    pw(static_cast<Byte>(0x00u));
    pw.lda_indirect_y();
    pw(ADDR_LO);
    pw.beq();
    rel_to("food_place");

    pw.lda_zero_page();
    pw(RNG);
    pw.clc();
    pw.adc_immediate();
    pw(static_cast<Byte>(17u));
    pw.sta_zero_page();
    pw(RNG);
    jmp_to("food_try");

    mark("food_place");
    pw.lda_immediate();
    pw(FOOD_VALUE);
    pw.sta_indirect_y();
    pw(ADDR_LO);
    pw.rts();

    // --------------------------- resolve label fixups
    for (const auto &fix : rel_fixups) {
        const Address target = labels.at(fix.label);
        const int next_pc = static_cast<int>(fix.operand_addr + 1u);
        const int rel = static_cast<int>(target) - next_pc;
        assert(rel >= -128 && rel <= 127);
        global.cpu.mem[fix.operand_addr] = static_cast<Byte>(rel & 0xFF);
    }
    for (const auto &fix : abs_fixups) {
        const Address target = labels.at(fix.label);
        global.cpu.mem[fix.low_byte_addr] = static_cast<Byte>(target & 0x00FFu);
        global.cpu.mem[static_cast<Address>(fix.low_byte_addr + 1u)] = static_cast<Byte>(target >> 8u);
    }

    install_interrupt_handler();
    global.cpu.PC = demo_start;
    global.cpu.mem[input_dir_addr] = DIR_RIGHT;
    global.cpu.mem[input_tick_addr] = static_cast<Byte>(0u);
    global.cpu.mem[score_out_addr] = static_cast<Byte>(0u);
    global.cpu.mem[state_out_addr] = static_cast<Byte>(0u);

    // Pre-run enough CPU ticks so initial snake + food are visible immediately.
    constexpr int bootstrap_ticks = 50000;
    for (int i = 0; i < bootstrap_ticks; ++i) {
        mos6502::tick(global.cpu);
    }
    last_host_tick_time = std::chrono::steady_clock::now();
}

inline auto reset_active_demo() -> void {
    load_default_demo();
}

inline auto update() -> void {
    if (global.sim.is_debugging) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if ((now - last_host_tick_time) < snake_tick_interval) {
        return;
    }
    last_host_tick_time = now;
    global.cpu.mem[input_tick_addr] = static_cast<Byte>(global.cpu.mem[input_tick_addr] + 1u);
}
} // namespace DEMO
