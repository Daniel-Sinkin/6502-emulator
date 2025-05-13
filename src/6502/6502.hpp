/*danielsinkin97@gmail.com*/
#pragma once

#include <array>

#include "../log.hpp"
#include "../types.hpp"

using TYPES::Byte;
using TYPES::Word;

namespace mos6502 {

#define INSTRUCTION_TYPE_LIST \
    X(_none)                  \
    X(lda)                    \
    X(jmp)                    \
    X(_enumcount_InstructionType)

enum class InstructionType {
#define X(name) name,
    INSTRUCTION_TYPE_LIST
#undef X
};

inline const char *to_string(InstructionType type) {
    switch (type) {
#define X(name)                 \
    case InstructionType::name: \
        return #name;
        INSTRUCTION_TYPE_LIST
#undef X
    default:
        return "<unknown>";
    }
}

enum class AddressingMode {
    immediate,
    absolute,
    zero_page,
    accum,
    implied,
    ind_x,
    ind_y,
    z_page_x,
    z_page_y,
    abs_x,
    abs_y,
    relative,
    indirect,
    _enumcount_AdressingMode
};

inline const char *to_string(AddressingMode mode) {
    switch (mode) {
    case AddressingMode::immediate:
        return "immediate";
    case AddressingMode::absolute:
        return "absolute";
    case AddressingMode::zero_page:
        return "zero_page";
    case AddressingMode::accum:
        return "accumulator";
    case AddressingMode::implied:
        return "implied";
    case AddressingMode::ind_x:
        return "indirect_x";
    case AddressingMode::ind_y:
        return "indirect_y";
    case AddressingMode::z_page_x:
        return "zero_page_x";
    case AddressingMode::z_page_y:
        return "zero_page_y";
    case AddressingMode::abs_x:
        return "absolute_x";
    case AddressingMode::abs_y:
        return "absolute_y";
    case AddressingMode::relative:
        return "relative";
    case AddressingMode::indirect:
        return "indirect";
    case AddressingMode::_enumcount_AdressingMode:
    default:
        PANIC();
    }
}

struct CPU;
using ExecFunction = void (*)(CPU &cpu);
struct Instruction {
    InstructionType type;
    Byte opcode;
    AddressingMode mode;
    ExecFunction exec;
};

struct CPU {
    Word PC = 0x0000;
    Byte A = 0x00;
    Byte X = 0x00;
    Byte Y = 0x00;
    Byte SP = 0x00;
    Byte P = 0x00;

    bool nmi = false;
    bool irq = false;

    bool sync = false;
    bool rdy = true;

    std::array<Byte, 64 * 1024> mem = {};
    Word addr = 0x0000;
    Byte data_bus = 0x00;
    bool rw;

    Instruction instr;
    int instr_counter = 0;

    Byte tmp = 0x00;
    uint64_t cycles = 0;
};

struct CPUSnapshot {
    CPU cpu;
};

[[nodiscard]] inline auto fetch(CPU &cpu) -> Byte { return cpu.mem[cpu.PC++]; }

auto exec_lda_immediate(CPU &cpu) -> void {
    if (cpu.instr_counter != 1) PANIC();
    cpu.A = fetch(cpu);
    cpu.instr_counter = 0;
}

auto exec_jmp_absolute(CPU &cpu) -> void {
    if (cpu.instr_counter == 1) {
        cpu.tmp = fetch(cpu);
        cpu.instr_counter = 2;
    } else if (cpu.instr_counter == 2) {
        cpu.PC = static_cast<Word>(fetch(cpu)) << 8 | cpu.tmp;
        cpu.instr_counter = 0;
    } else {
        PANIC();
    }
}

auto program_writer(CPU &cpu, Byte value, Word &addr) -> void {
    cpu.mem[addr++] = value;
}

std::array<Instruction, 2> instructions{
    {{InstructionType::lda, 0xA9, AddressingMode::immediate, exec_lda_immediate},
        {InstructionType::jmp, 0x4C, AddressingMode::absolute, exec_jmp_absolute}}};

inline auto tick(CPU &cpu) -> void {
    if (cpu.instr_counter == 0) {
        // Load instruction
        Byte opcode = fetch(cpu);
        bool found = false;
        for (size_t i = 0; i < instructions.size(); ++i) {
            Instruction instr = instructions[i];
            if (opcode == instr.opcode) {
                cpu.instr = instr;
                found = true;
            }
        }
        if (!found) PANIC_NOT_IMPLEMENTED(opcode);
        cpu.instr_counter = 1;
    } else {
        cpu.instr.exec(cpu);
    }
    ++cpu.cycles;
}
} // namespace mos6502
