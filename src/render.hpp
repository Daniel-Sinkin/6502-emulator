#pragma once

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl2.h>
#include <glad/glad.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <string>

#include "global.hpp"
#include "utils.hpp"

namespace RENDER {
[[nodiscard]] constexpr auto
instruction_size(mos6502::AddressingMode mode) -> uint8_t {
    switch (mode) {
    case mos6502::AddressingMode::implied:
    case mos6502::AddressingMode::accum:
    case mos6502::AddressingMode::NONE:
        return 1;
    case mos6502::AddressingMode::immediate:
    case mos6502::AddressingMode::zero_page:
    case mos6502::AddressingMode::zero_page_x:
    case mos6502::AddressingMode::zero_page_y:
    case mos6502::AddressingMode::relative:
    case mos6502::AddressingMode::indirect_x:
    case mos6502::AddressingMode::indirect_y:
        return 2;
    case mos6502::AddressingMode::absolute:
    case mos6502::AddressingMode::absolute_x:
    case mos6502::AddressingMode::absolute_y:
    case mos6502::AddressingMode::indirect:
        return 3;
    }
    return 1;
}

[[nodiscard]] inline auto
opcode_size(const mos6502::CPU &cpu, Address addr) -> uint8_t {
    Byte opcode = cpu.mem[addr];
    const mos6502::Instruction &instr = mos6502::instructions[opcode];
    if (instr.type == mos6502::InstructionType::NONE) {
        return 1;
    }
    return instruction_size(instr.mode);
}

[[nodiscard]] inline auto
instr_mnemonic(mos6502::InstructionType type) -> std::string {
    std::string mnemonic = mos6502::to_string(type);
    if (mnemonic == "and_") {
        mnemonic = "and";
    }
    std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });
    return mnemonic;
}

[[nodiscard]] inline auto
format_operand(const mos6502::CPU &cpu, Address pc, const mos6502::Instruction &instr) -> std::string {
    auto read_at = [&](uint16_t offset) -> Byte {
        return cpu.mem[static_cast<Address>(pc + offset)];
    };
    Byte op1 = read_at(1);
    Byte op2 = read_at(2);
    Address word = static_cast<Address>((static_cast<Address>(op2) << 8) | static_cast<Address>(op1));

    char buffer[32];
    switch (instr.mode) {
    case mos6502::AddressingMode::immediate:
        std::snprintf(buffer, sizeof(buffer), "#$%02X", static_cast<unsigned int>(op1));
        return buffer;
    case mos6502::AddressingMode::zero_page:
        std::snprintf(buffer, sizeof(buffer), "$%02X", static_cast<unsigned int>(op1));
        return buffer;
    case mos6502::AddressingMode::zero_page_x:
        std::snprintf(buffer, sizeof(buffer), "$%02X,X", static_cast<unsigned int>(op1));
        return buffer;
    case mos6502::AddressingMode::zero_page_y:
        std::snprintf(buffer, sizeof(buffer), "$%02X,Y", static_cast<unsigned int>(op1));
        return buffer;
    case mos6502::AddressingMode::absolute:
        std::snprintf(buffer, sizeof(buffer), "$%04X", static_cast<unsigned int>(word));
        return buffer;
    case mos6502::AddressingMode::absolute_x:
        std::snprintf(buffer, sizeof(buffer), "$%04X,X", static_cast<unsigned int>(word));
        return buffer;
    case mos6502::AddressingMode::absolute_y:
        std::snprintf(buffer, sizeof(buffer), "$%04X,Y", static_cast<unsigned int>(word));
        return buffer;
    case mos6502::AddressingMode::indirect:
        std::snprintf(buffer, sizeof(buffer), "($%04X)", static_cast<unsigned int>(word));
        return buffer;
    case mos6502::AddressingMode::indirect_x:
        std::snprintf(buffer, sizeof(buffer), "($%02X,X)", static_cast<unsigned int>(op1));
        return buffer;
    case mos6502::AddressingMode::indirect_y:
        std::snprintf(buffer, sizeof(buffer), "($%02X),Y", static_cast<unsigned int>(op1));
        return buffer;
    case mos6502::AddressingMode::relative: {
        Address target = static_cast<Address>(pc + 2 + static_cast<int8_t>(op1));
        std::snprintf(buffer, sizeof(buffer), "$%04X", static_cast<unsigned int>(target));
        return buffer;
    }
    case mos6502::AddressingMode::implied:
    case mos6502::AddressingMode::accum:
    case mos6502::AddressingMode::NONE:
        return "";
    }
    return "";
}

[[nodiscard]] inline auto hex8(Byte value) -> std::string {
    char buffer[8];
    std::snprintf(buffer, sizeof(buffer), "%02X", static_cast<unsigned int>(value));
    return buffer;
}

[[nodiscard]] inline auto hex16(Address value) -> std::string {
    char buffer[8];
    std::snprintf(buffer, sizeof(buffer), "%04X", static_cast<unsigned int>(value));
    return buffer;
}

[[nodiscard]] inline auto
address_expr_from_mode(mos6502::AddressingMode mode, Byte op1, Address word, Address rel_target) -> std::string {
    switch (mode) {
    case mos6502::AddressingMode::zero_page:
        return "$00" + hex8(op1);
    case mos6502::AddressingMode::zero_page_x:
        return "($" + hex8(op1) + " + X) & $FF";
    case mos6502::AddressingMode::zero_page_y:
        return "($" + hex8(op1) + " + Y) & $FF";
    case mos6502::AddressingMode::absolute:
        return "$" + hex16(word);
    case mos6502::AddressingMode::absolute_x:
        return "$" + hex16(word) + " + X";
    case mos6502::AddressingMode::absolute_y:
        return "$" + hex16(word) + " + Y";
    case mos6502::AddressingMode::indirect:
        return "[$" + hex16(word) + "]";
    case mos6502::AddressingMode::indirect_x:
        return "(($" + hex8(op1) + " + X) & $FF)";
    case mos6502::AddressingMode::indirect_y:
        return "($" + hex8(op1) + ") + Y";
    case mos6502::AddressingMode::relative:
        return "$" + hex16(rel_target);
    case mos6502::AddressingMode::accum:
        return "A";
    case mos6502::AddressingMode::immediate:
    case mos6502::AddressingMode::implied:
    case mos6502::AddressingMode::NONE:
        return "";
    }
    return "";
}

[[nodiscard]] inline auto
source_expr_from_mode(mos6502::AddressingMode mode, Byte op1, Address word, Address rel_target) -> std::string {
    if (mode == mos6502::AddressingMode::immediate) {
        return "$" + hex8(op1);
    }
    if (mode == mos6502::AddressingMode::accum) {
        return "A";
    }
    const std::string addr_expr = address_expr_from_mode(mode, op1, word, rel_target);
    if (addr_expr.empty()) {
        return "";
    }
    return "mem[" + addr_expr + "]";
}

[[nodiscard]] inline auto
branch_condition_comment(mos6502::InstructionType type) -> std::string {
    switch (type) {
    case mos6502::InstructionType::bcc:
        return "C=0";
    case mos6502::InstructionType::bcs:
        return "C=1";
    case mos6502::InstructionType::beq:
        return "Z=1";
    case mos6502::InstructionType::bne:
        return "Z=0";
    case mos6502::InstructionType::bmi:
        return "N=1";
    case mos6502::InstructionType::bpl:
        return "N=0";
    case mos6502::InstructionType::bvc:
        return "V=0";
    case mos6502::InstructionType::bvs:
        return "V=1";
    default:
        return "";
    }
}

[[nodiscard]] inline auto
human_comment(const mos6502::CPU &cpu, Address pc, const mos6502::Instruction &instr) -> std::string {
    auto read_at = [&](uint16_t offset) -> Byte {
        return cpu.mem[static_cast<Address>(pc + offset)];
    };

    const Byte op1 = read_at(1);
    const Byte op2 = read_at(2);
    const Address word = static_cast<Address>((static_cast<Address>(op2) << 8) | static_cast<Address>(op1));
    const Address rel_target = static_cast<Address>(pc + 2 + static_cast<int8_t>(op1));
    const auto source = source_expr_from_mode(instr.mode, op1, word, rel_target);
    const auto target = address_expr_from_mode(instr.mode, op1, word, rel_target);

    switch (instr.type) {
    case mos6502::InstructionType::adc:
        return "A <- A + " + source + " + C";
    case mos6502::InstructionType::and_:
        return "A <- A & " + source;
    case mos6502::InstructionType::asl:
        return "shift left " + (source.empty() ? std::string("value") : source);
    case mos6502::InstructionType::bcc:
    case mos6502::InstructionType::bcs:
    case mos6502::InstructionType::beq:
    case mos6502::InstructionType::bne:
    case mos6502::InstructionType::bmi:
    case mos6502::InstructionType::bpl:
    case mos6502::InstructionType::bvc:
    case mos6502::InstructionType::bvs:
        return "if " + branch_condition_comment(instr.type) + " jump to " + target;
    case mos6502::InstructionType::bit:
        return "test bits in " + source + " against A";
    case mos6502::InstructionType::brk:
        return "software interrupt";
    case mos6502::InstructionType::clc:
        return "clear carry";
    case mos6502::InstructionType::cld:
        return "clear decimal mode";
    case mos6502::InstructionType::cli:
        return "enable IRQ";
    case mos6502::InstructionType::clv:
        return "clear overflow";
    case mos6502::InstructionType::cmp:
        return "compare A with " + source;
    case mos6502::InstructionType::cpx:
        return "compare X with " + source;
    case mos6502::InstructionType::cpy:
        return "compare Y with " + source;
    case mos6502::InstructionType::dec:
        return "decrement mem[" + target + "]";
    case mos6502::InstructionType::dex:
        return "X <- X - 1";
    case mos6502::InstructionType::dey:
        return "Y <- Y - 1";
    case mos6502::InstructionType::eor:
        return "A <- A ^ " + source;
    case mos6502::InstructionType::inc:
        return "increment mem[" + target + "]";
    case mos6502::InstructionType::inx:
        return "X <- X + 1";
    case mos6502::InstructionType::iny:
        return "Y <- Y + 1";
    case mos6502::InstructionType::jmp:
        return "jump to " + target;
    case mos6502::InstructionType::jsr:
        return "call subroutine " + target;
    case mos6502::InstructionType::lda:
        return "A <- " + source;
    case mos6502::InstructionType::ldx:
        return "X <- " + source;
    case mos6502::InstructionType::ldy:
        return "Y <- " + source;
    case mos6502::InstructionType::lsr:
        return "shift right " + (source.empty() ? std::string("value") : source);
    case mos6502::InstructionType::nop:
        return "no operation";
    case mos6502::InstructionType::ora:
        return "A <- A | " + source;
    case mos6502::InstructionType::pha:
        return "push A";
    case mos6502::InstructionType::php:
        return "push status flags";
    case mos6502::InstructionType::pla:
        return "pop stack into A";
    case mos6502::InstructionType::plp:
        return "pop stack into flags";
    case mos6502::InstructionType::rol:
        return "rotate left " + (source.empty() ? std::string("value") : source);
    case mos6502::InstructionType::ror:
        return "rotate right " + (source.empty() ? std::string("value") : source);
    case mos6502::InstructionType::rti:
        return "return from interrupt";
    case mos6502::InstructionType::rts:
        return "return from subroutine";
    case mos6502::InstructionType::sbc:
        return "A <- A - " + source + " - !C";
    case mos6502::InstructionType::sec:
        return "set carry";
    case mos6502::InstructionType::sed:
        return "set decimal mode";
    case mos6502::InstructionType::sei:
        return "disable IRQ";
    case mos6502::InstructionType::sta:
        return "mem[" + target + "] <- A";
    case mos6502::InstructionType::stx:
        return "mem[" + target + "] <- X";
    case mos6502::InstructionType::sty:
        return "mem[" + target + "] <- Y";
    case mos6502::InstructionType::tax:
        return "X <- A";
    case mos6502::InstructionType::tay:
        return "Y <- A";
    case mos6502::InstructionType::tsx:
        return "X <- SP";
    case mos6502::InstructionType::txa:
        return "A <- X";
    case mos6502::InstructionType::txs:
        return "SP <- X";
    case mos6502::InstructionType::tya:
        return "A <- Y";
    case mos6502::InstructionType::NONE:
        return "unknown opcode";
    }
    return "";
}

[[nodiscard]] inline auto current_focus_pc(const mos6502::CPU &cpu) -> Address {
    return (cpu.addr_result.type == mos6502::AddrResultType::load_instruction)
               ? cpu.PC
               : cpu.current_instruction_pc;
}

[[nodiscard]] inline auto
instruction_reference_text(mos6502::InstructionType type) -> const char * {
    switch (type) {
    case mos6502::InstructionType::adc: return "Add memory to accumulator with carry.";
    case mos6502::InstructionType::and_: return "Bitwise AND memory with accumulator.";
    case mos6502::InstructionType::asl: return "Arithmetic shift left by one bit.";
    case mos6502::InstructionType::bcc: return "Branch if carry flag is clear.";
    case mos6502::InstructionType::bcs: return "Branch if carry flag is set.";
    case mos6502::InstructionType::beq: return "Branch if zero flag is set.";
    case mos6502::InstructionType::bit: return "Test bits in memory against accumulator.";
    case mos6502::InstructionType::bmi: return "Branch if negative flag is set.";
    case mos6502::InstructionType::bne: return "Branch if zero flag is clear.";
    case mos6502::InstructionType::bpl: return "Branch if negative flag is clear.";
    case mos6502::InstructionType::brk: return "Force software interrupt.";
    case mos6502::InstructionType::bvc: return "Branch if overflow flag is clear.";
    case mos6502::InstructionType::bvs: return "Branch if overflow flag is set.";
    case mos6502::InstructionType::clc: return "Clear carry flag.";
    case mos6502::InstructionType::cld: return "Clear decimal mode flag.";
    case mos6502::InstructionType::cli: return "Clear interrupt disable flag (enable IRQ).";
    case mos6502::InstructionType::clv: return "Clear overflow flag.";
    case mos6502::InstructionType::cmp: return "Compare accumulator with memory.";
    case mos6502::InstructionType::cpx: return "Compare X register with memory.";
    case mos6502::InstructionType::cpy: return "Compare Y register with memory.";
    case mos6502::InstructionType::dec: return "Decrement memory by one.";
    case mos6502::InstructionType::dex: return "Decrement X register by one.";
    case mos6502::InstructionType::dey: return "Decrement Y register by one.";
    case mos6502::InstructionType::eor: return "Bitwise exclusive OR memory with accumulator.";
    case mos6502::InstructionType::inc: return "Increment memory by one.";
    case mos6502::InstructionType::inx: return "Increment X register by one.";
    case mos6502::InstructionType::iny: return "Increment Y register by one.";
    case mos6502::InstructionType::jmp: return "Jump to new program counter address.";
    case mos6502::InstructionType::jsr: return "Jump to subroutine (push return address).";
    case mos6502::InstructionType::lda: return "Load accumulator from memory.";
    case mos6502::InstructionType::ldx: return "Load X register from memory.";
    case mos6502::InstructionType::ldy: return "Load Y register from memory.";
    case mos6502::InstructionType::lsr: return "Logical shift right by one bit.";
    case mos6502::InstructionType::nop: return "No operation.";
    case mos6502::InstructionType::ora: return "Bitwise OR memory with accumulator.";
    case mos6502::InstructionType::pha: return "Push accumulator to stack.";
    case mos6502::InstructionType::php: return "Push processor status to stack.";
    case mos6502::InstructionType::pla: return "Pull accumulator from stack.";
    case mos6502::InstructionType::plp: return "Pull processor status from stack.";
    case mos6502::InstructionType::rol: return "Rotate bits left through carry.";
    case mos6502::InstructionType::ror: return "Rotate bits right through carry.";
    case mos6502::InstructionType::rti: return "Return from interrupt.";
    case mos6502::InstructionType::rts: return "Return from subroutine.";
    case mos6502::InstructionType::sbc: return "Subtract memory from accumulator with borrow.";
    case mos6502::InstructionType::sec: return "Set carry flag.";
    case mos6502::InstructionType::sed: return "Set decimal mode flag.";
    case mos6502::InstructionType::sei: return "Set interrupt disable flag (disable IRQ).";
    case mos6502::InstructionType::sta: return "Store accumulator to memory.";
    case mos6502::InstructionType::stx: return "Store X register to memory.";
    case mos6502::InstructionType::sty: return "Store Y register to memory.";
    case mos6502::InstructionType::tax: return "Transfer accumulator to X.";
    case mos6502::InstructionType::tay: return "Transfer accumulator to Y.";
    case mos6502::InstructionType::tsx: return "Transfer stack pointer to X.";
    case mos6502::InstructionType::txa: return "Transfer X to accumulator.";
    case mos6502::InstructionType::txs: return "Transfer X to stack pointer.";
    case mos6502::InstructionType::tya: return "Transfer Y to accumulator.";
    case mos6502::InstructionType::NONE: return "Unknown opcode slot.";
    }
    return "Unknown opcode slot.";
}

[[nodiscard]] inline auto
flags_affected_text(mos6502::InstructionType type) -> const char * {
    switch (type) {
    case mos6502::InstructionType::adc:
    case mos6502::InstructionType::sbc:
        return "N,Z,C,V updated";
    case mos6502::InstructionType::and_:
    case mos6502::InstructionType::eor:
    case mos6502::InstructionType::ora:
    case mos6502::InstructionType::lda:
    case mos6502::InstructionType::ldx:
    case mos6502::InstructionType::ldy:
    case mos6502::InstructionType::tax:
    case mos6502::InstructionType::tay:
    case mos6502::InstructionType::tsx:
    case mos6502::InstructionType::txa:
    case mos6502::InstructionType::tya:
    case mos6502::InstructionType::dex:
    case mos6502::InstructionType::dey:
    case mos6502::InstructionType::inx:
    case mos6502::InstructionType::iny:
    case mos6502::InstructionType::inc:
    case mos6502::InstructionType::dec:
    case mos6502::InstructionType::pla:
        return "N,Z updated";
    case mos6502::InstructionType::asl:
    case mos6502::InstructionType::lsr:
    case mos6502::InstructionType::rol:
    case mos6502::InstructionType::ror:
        return "N,Z,C updated";
    case mos6502::InstructionType::cmp:
    case mos6502::InstructionType::cpx:
    case mos6502::InstructionType::cpy:
        return "N,Z,C updated (register unchanged)";
    case mos6502::InstructionType::bit:
        return "Z,N,V updated";
    case mos6502::InstructionType::clc:
    case mos6502::InstructionType::sec:
        return "C updated";
    case mos6502::InstructionType::cld:
    case mos6502::InstructionType::sed:
        return "D updated";
    case mos6502::InstructionType::cli:
    case mos6502::InstructionType::sei:
        return "I updated";
    case mos6502::InstructionType::clv:
        return "V cleared";
    case mos6502::InstructionType::php:
    case mos6502::InstructionType::pha:
    case mos6502::InstructionType::sta:
    case mos6502::InstructionType::stx:
    case mos6502::InstructionType::sty:
    case mos6502::InstructionType::txs:
    case mos6502::InstructionType::jmp:
    case mos6502::InstructionType::jsr:
    case mos6502::InstructionType::rts:
    case mos6502::InstructionType::nop:
        return "Flags unchanged";
    case mos6502::InstructionType::plp:
    case mos6502::InstructionType::rti:
        return "Flags restored from stack";
    case mos6502::InstructionType::brk:
        return "I set; status pushed to stack";
    case mos6502::InstructionType::bcc:
    case mos6502::InstructionType::bcs:
    case mos6502::InstructionType::beq:
    case mos6502::InstructionType::bne:
    case mos6502::InstructionType::bmi:
    case mos6502::InstructionType::bpl:
    case mos6502::InstructionType::bvc:
    case mos6502::InstructionType::bvs:
    case mos6502::InstructionType::NONE:
        return "Flags read only";
    }
    return "Flags read only";
}

[[nodiscard]] inline auto
branch_condition_now(const mos6502::CPU &cpu, mos6502::InstructionType type) -> std::string {
    const bool c = (cpu.P & mos6502::C_FLAG) != 0u;
    const bool z = (cpu.P & mos6502::Z_FLAG) != 0u;
    const bool n = (cpu.P & mos6502::N_FLAG) != 0u;
    const bool v = (cpu.P & mos6502::V_FLAG) != 0u;
    bool taken = false;
    bool relevant = true;
    switch (type) {
    case mos6502::InstructionType::bcc: taken = !c; break;
    case mos6502::InstructionType::bcs: taken = c; break;
    case mos6502::InstructionType::beq: taken = z; break;
    case mos6502::InstructionType::bne: taken = !z; break;
    case mos6502::InstructionType::bmi: taken = n; break;
    case mos6502::InstructionType::bpl: taken = !n; break;
    case mos6502::InstructionType::bvc: taken = !v; break;
    case mos6502::InstructionType::bvs: taken = v; break;
    default: relevant = false; break;
    }
    if (!relevant) return "";
    return taken ? "branch condition is true right now" : "branch condition is false right now";
}

struct ResolvedOperandInfo {
    std::string addressing_line{};
    std::string live_line{};
};

[[nodiscard]] inline auto
resolve_operand_info(const mos6502::CPU &cpu, Address pc, const mos6502::Instruction &instr) -> ResolvedOperandInfo {
    auto read_at = [&](uint16_t offset) -> Byte {
        return cpu.mem[static_cast<Address>(pc + offset)];
    };

    const Byte op1 = read_at(1);
    const Byte op2 = read_at(2);
    const Address word = static_cast<Address>((static_cast<Address>(op2) << 8) | static_cast<Address>(op1));
    const Address rel_target = static_cast<Address>(pc + 2 + static_cast<int8_t>(op1));

    ResolvedOperandInfo info{};

    auto with_mem_value = [&](Address addr, const std::string &prefix) -> void {
        info.live_line = prefix + " mem[$" + hex16(addr) + "] = $" + hex8(cpu.mem[addr]);
    };

    switch (instr.mode) {
    case mos6502::AddressingMode::immediate:
        info.addressing_line = "Addressing: immediate operand #$" + hex8(op1);
        info.live_line = "Live: immediate value is $" + hex8(op1);
        break;
    case mos6502::AddressingMode::accum:
        info.addressing_line = "Addressing: accumulator (A)";
        info.live_line = "Live: A = $" + hex8(cpu.A);
        break;
    case mos6502::AddressingMode::implied:
        info.addressing_line = "Addressing: implied (no explicit operand)";
        info.live_line = "Live: SP=$" + hex8(cpu.SP) + " P=$" + hex8(cpu.P);
        break;
    case mos6502::AddressingMode::zero_page: {
        const Address addr = static_cast<Address>(op1);
        info.addressing_line = "Addressing: zero page $" + hex8(op1) + " -> $" + hex16(addr);
        with_mem_value(addr, "Live:");
        break;
    }
    case mos6502::AddressingMode::zero_page_x: {
        const Byte zp = static_cast<Byte>(op1 + cpu.X);
        const Address addr = static_cast<Address>(zp);
        info.addressing_line = "Addressing: ($" + hex8(op1) + " + X=$" + hex8(cpu.X) + ") & $FF -> $" + hex16(addr);
        with_mem_value(addr, "Live:");
        break;
    }
    case mos6502::AddressingMode::zero_page_y: {
        const Byte zp = static_cast<Byte>(op1 + cpu.Y);
        const Address addr = static_cast<Address>(zp);
        info.addressing_line = "Addressing: ($" + hex8(op1) + " + Y=$" + hex8(cpu.Y) + ") & $FF -> $" + hex16(addr);
        with_mem_value(addr, "Live:");
        break;
    }
    case mos6502::AddressingMode::absolute: {
        info.addressing_line = "Addressing: absolute $" + hex16(word);
        with_mem_value(word, "Live:");
        break;
    }
    case mos6502::AddressingMode::absolute_x: {
        const Address addr = static_cast<Address>(word + cpu.X);
        info.addressing_line = "Addressing: $" + hex16(word) + " + X=$" + hex8(cpu.X) + " -> $" + hex16(addr);
        with_mem_value(addr, "Live:");
        break;
    }
    case mos6502::AddressingMode::absolute_y: {
        const Address addr = static_cast<Address>(word + cpu.Y);
        info.addressing_line = "Addressing: $" + hex16(word) + " + Y=$" + hex8(cpu.Y) + " -> $" + hex16(addr);
        with_mem_value(addr, "Live:");
        break;
    }
    case mos6502::AddressingMode::indirect_x: {
        const Byte zp_ptr = static_cast<Byte>(op1 + cpu.X);
        const Address low_addr = static_cast<Address>(zp_ptr);
        const Address high_addr = static_cast<Address>(static_cast<Byte>(zp_ptr + 1u));
        const Address addr = static_cast<Address>((static_cast<Address>(cpu.mem[high_addr]) << 8) |
                                                  static_cast<Address>(cpu.mem[low_addr]));
        info.addressing_line = "Addressing: ($" + hex8(op1) + " + X=$" + hex8(cpu.X) + ") -> ptr $" + hex8(zp_ptr) + " -> $" + hex16(addr);
        with_mem_value(addr, "Live:");
        break;
    }
    case mos6502::AddressingMode::indirect_y: {
        const Address low_addr = static_cast<Address>(op1);
        const Address high_addr = static_cast<Address>(static_cast<Byte>(op1 + 1u));
        const Address base = static_cast<Address>((static_cast<Address>(cpu.mem[high_addr]) << 8) |
                                                  static_cast<Address>(cpu.mem[low_addr]));
        const Address addr = static_cast<Address>(base + cpu.Y);
        info.addressing_line = "Addressing: ($" + hex8(op1) + ") -> base $" + hex16(base) + " + Y=$" + hex8(cpu.Y) + " -> $" + hex16(addr);
        with_mem_value(addr, "Live:");
        break;
    }
    case mos6502::AddressingMode::indirect: {
        const Address ptr = word;
        Address high_ptr;
        const bool wraps = (ptr & 0x00FFu) == 0x00FFu;
        if (wraps && cpu.config.preserve_indirect_jump_page_cross_bug) {
            high_ptr = ptr & 0xFF00u;
        } else {
            high_ptr = static_cast<Address>(ptr + 1u);
        }
        const Address addr = static_cast<Address>((static_cast<Address>(cpu.mem[high_ptr]) << 8) |
                                                  static_cast<Address>(cpu.mem[ptr]));
        info.addressing_line = "Addressing: indirect ($" + hex16(ptr) + ") -> $" + hex16(addr);
        info.live_line = wraps && cpu.config.preserve_indirect_jump_page_cross_bug
                             ? "Live: page-wrap quirk used at $" + hex16(ptr)
                             : "Live: pointer read from $" + hex16(ptr) + "/$" + hex16(high_ptr);
        break;
    }
    case mos6502::AddressingMode::relative:
        info.addressing_line = "Addressing: relative offset $" + hex8(op1) + " -> target $" + hex16(rel_target);
        info.live_line = "Live: " + branch_condition_now(cpu, instr.type);
        break;
    case mos6502::AddressingMode::NONE:
        info.addressing_line = "Addressing: none";
        info.live_line = "Live: unknown opcode slot";
        break;
    }

    if (instr.type == mos6502::InstructionType::sta) {
        info.live_line += " ; A=$" + hex8(cpu.A);
    } else if (instr.type == mos6502::InstructionType::stx) {
        info.live_line += " ; X=$" + hex8(cpu.X);
    } else if (instr.type == mos6502::InstructionType::sty) {
        info.live_line += " ; Y=$" + hex8(cpu.Y);
    }

    return info;
}

inline auto draw_instruction_explainer_window(const mos6502::CPU &cpu) -> void {
    const Address pc = current_focus_pc(cpu);
    const Byte opcode = cpu.mem[pc];
    const mos6502::Instruction instr = mos6502::instructions[opcode];
    const std::string mnemonic = (instr.type == mos6502::InstructionType::NONE)
                                     ? "???"
                                     : instr_mnemonic(instr.type);
    const std::string operand = (instr.type == mos6502::InstructionType::NONE)
                                    ? ""
                                    : format_operand(cpu, pc, instr);

    ImGui::Begin("Instruction Explain");
    ImGui::Text("Now: %04X  %s %s",
        static_cast<unsigned int>(pc),
        mnemonic.c_str(),
        operand.c_str());
    ImGui::Separator();

    const auto resolved = resolve_operand_info(cpu, pc, instr);
    const std::string line1 = std::string("Reference: ") + instruction_reference_text(instr.type);
    const std::string line2 = resolved.addressing_line;
    const std::string line3 = resolved.live_line;
    const std::string line4 = std::string("Effect: ") + human_comment(cpu, pc, instr) + " | Flags: " + flags_affected_text(instr.type);

    ImGui::TextWrapped("%s", line1.c_str());
    ImGui::TextWrapped("%s", line2.c_str());
    ImGui::TextWrapped("%s", line3.c_str());
    ImGui::TextWrapped("%s", line4.c_str());
    ImGui::End();
}

[[nodiscard]] inline auto
find_prev_instruction(const mos6502::CPU &cpu, Address current) -> Address {
    for (uint16_t back = 1; back <= 3; ++back) {
        Address candidate = static_cast<Address>(current - back);
        Byte opcode = cpu.mem[candidate];
        const mos6502::Instruction &instr = mos6502::instructions[opcode];
        if (instr.type == mos6502::InstructionType::NONE) {
            continue;
        }
        if (instruction_size(instr.mode) == back) {
            return candidate;
        }
    }
    return static_cast<Address>(current - 1);
}

inline auto draw_code_window(const mos6502::CPU &cpu) -> void {
    ImGui::Begin("Code (PC +/- 5)");
    const Address focus_pc = (cpu.addr_result.type == mos6502::AddrResultType::load_instruction)
                                 ? cpu.PC
                                 : cpu.current_instruction_pc;
    std::array<Address, 11> lines{};
    lines[5] = focus_pc;

    for (int i = 4; i >= 0; --i) {
        lines[static_cast<size_t>(i)] = find_prev_instruction(cpu, lines[static_cast<size_t>(i + 1)]);
    }
    for (size_t i = 6; i < lines.size(); ++i) {
        Address prev = lines[i - 1];
        lines[i] = static_cast<Address>(prev + opcode_size(cpu, prev));
    }

    for (Address line_pc : lines) {
        Byte opcode = cpu.mem[line_pc];
        const mos6502::Instruction &instr = mos6502::instructions[opcode];
        uint8_t size = opcode_size(cpu, line_pc);

        std::string bytes;
        for (uint8_t i = 0; i < size; ++i) {
            if (!bytes.empty()) {
                bytes += ' ';
            }
            char b[8];
            std::snprintf(b, sizeof(b), "%02X", static_cast<unsigned int>(cpu.mem[static_cast<Address>(line_pc + i)]));
            bytes += b;
        }

        std::string mnemonic = (instr.type == mos6502::InstructionType::NONE)
                                   ? "???"
                                   : instr_mnemonic(instr.type);
        std::string operand = (instr.type == mos6502::InstructionType::NONE)
                                  ? ""
                                  : format_operand(cpu, line_pc, instr);
        std::string comment = (instr.type == mos6502::InstructionType::NONE)
                                  ? "unknown opcode"
                                  : human_comment(cpu, line_pc, instr);

        if (line_pc == focus_pc) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 220, 120, 255));
        }
        ImGui::Text("%04X  %-8s %-4s %-12s ; %s",
            static_cast<unsigned int>(line_pc),
            bytes.c_str(),
            mnemonic.c_str(),
            operand.c_str(),
            comment.c_str());
        if (line_pc == focus_pc) {
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();
}

[[nodiscard]] inline auto
framebuffer_color(Byte value) -> ImU32 {
    const int red = static_cast<int>((value & 0x03u) * 85u);
    const int green = static_cast<int>(((value >> 2u) & 0x03u) * 85u);
    const int blue = static_cast<int>(((value >> 4u) & 0x03u) * 85u);
    return IM_COL32(red, green, blue, 255);
}

inline auto draw_framebuffer_window(const mos6502::CPU &cpu) -> void {
    constexpr Address framebuffer_base = static_cast<Address>(0x0200u);
    constexpr int framebuffer_width = 32;
    constexpr int framebuffer_height = 32;
    static float pixel_size = 8.0f;

    ImGui::Begin("Framebuffer (0x0200-0x05FF)");
    ImGui::SliderFloat("Pixel Size", &pixel_size, 2.0f, 18.0f, "%.1f");

    const ImVec2 top_left = ImGui::GetCursorScreenPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    for (int y = 0; y < framebuffer_height; ++y) {
        for (int x = 0; x < framebuffer_width; ++x) {
            const Address offset = static_cast<Address>((y * framebuffer_width) + x);
            const Address addr = static_cast<Address>(framebuffer_base + offset);
            const Byte value = cpu.mem[addr];

            const float x0 = top_left.x + (static_cast<float>(x) * pixel_size);
            const float y0 = top_left.y + (static_cast<float>(y) * pixel_size);
            const float x1 = x0 + pixel_size;
            const float y1 = y0 + pixel_size;

            draw_list->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), framebuffer_color(value));
        }
    }

    const float total_width = static_cast<float>(framebuffer_width) * pixel_size;
    const float total_height = static_cast<float>(framebuffer_height) * pixel_size;
    draw_list->AddRect(
        top_left,
        ImVec2(top_left.x + total_width, top_left.y + total_height),
        IM_COL32(220, 220, 220, 255));

    ImGui::Dummy(ImVec2(total_width, total_height));
    ImGui::Text("addr = $0200 + y*32 + x");
    ImGui::Text("color bits: [1:0]=R [3:2]=G [5:4]=B");
    ImGui::End();
}

inline auto cpu_register(mos6502::CPU &cpu) -> void {
    ImGui::Text("Registers");
    if (ImGui::BeginTable("cpu_registers", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("PC   0x%04X", static_cast<unsigned int>(cpu.PC));
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("A    0x%02X", static_cast<unsigned int>(cpu.A));
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("X    0x%02X", static_cast<unsigned int>(cpu.X));
        ImGui::TableSetColumnIndex(3);
        ImGui::Text("Y    0x%02X", static_cast<unsigned int>(cpu.Y));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("SP   0x%02X", static_cast<unsigned int>(cpu.SP));
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("P    0x%02X", static_cast<unsigned int>(cpu.P));
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("ADDR 0x%04X", static_cast<unsigned int>(cpu.temporary_address_register));
        ImGui::TableSetColumnIndex(3);
        ImGui::Text("DB   0x%02X", static_cast<unsigned int>(cpu.data_bus));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("RW   %s", cpu.rw ? "read" : "write");
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
        ImGui::Text("TMP  %02X", static_cast<unsigned int>(cpu.tmp));
        ImGui::EndTable();
    }
    ImGui::Text(
        "%s %s [%d]",
        mos6502::to_string(cpu.instr.mode),
        mos6502::to_string(cpu.instr.type),
        cpu.instr_counter);
}

inline auto begin_dockspace() -> void {
    ImGuiIO &io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_DockingEnable) == 0) {
        return;
    }

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                    ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus |
                                    ImGuiWindowFlags_NoBackground;

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("MainDockSpaceHost", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    const ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();
}

inline auto gui_debug() -> void {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    begin_dockspace();

    /* ------------------------------------------------------------------ Debug */
    ImGui::Begin("Debug");
    ImGui::ColorEdit3("Background", &global.color.background.r);
    ImGui::ColorEdit3("Pixel On", &global.color.pixel_on.r);
    ImGui::ColorEdit3("Pixel Off", &global.color.pixel_off.r);
    ImGui::Text("Frame Counter: %d", global.sim.frame_counter);
    ImGui::Text("Runtime: %s",
        UTIL::format_duration(global.sim.total_runtime).c_str());
    ImGui::Text("Delta Time (ms): %.3f", global.sim.delta_time.count());
    ImGui::Text("Run Clock Mode: %s",
        global.sim.realtime_clock_mode ? "real-time 6502 (~1 MHz)" : "fixed ticks/frame");
    ImGui::Text("Ticks This Frame: %llu",
        static_cast<unsigned long long>(global.sim.last_ticks_executed));
    ImGui::Text("Instructions This Frame: %llu",
        static_cast<unsigned long long>(global.sim.last_instructions_executed));
    ImGui::Text("Instructions Total: %llu",
        static_cast<unsigned long long>(global.sim.instructions_executed_total));
    ImGui::Text("Mouse Position: (%.3f, %.3f)",
        global.input.mouse_pos_x, global.input.mouse_pos_y);
    ImGui::Text("Is Debugging %s", global.sim.is_debugging ? "true" : "false");
    ImGui::Text("Is Stepping      %s", global.sim.step_once ? "true" : "false");
    ImGui::Text("Is Back Stepping %s", global.sim.step_back ? "true" : "false");
    ImGui::Text("Is Fwd Stepping  %s", global.sim.step_forward ? "true" : "false");
    ImGui::Text("History Cursor   %zu / %zu",
        global.cpu_history.cursor(),
        global.cpu_history.size());
    ImGui::Text("History Capacity %zu steps", global.cpu_history.capacity());
    ImGui::Text("History Payload %.2f MB (%zu changed blocks @ %zu bytes)",
        UTIL::byte_to_mb(global.cpu_history.estimated_payload_bytes()),
        global.cpu_history.total_changed_blocks(),
        mos6502::history_block_size);
    ImGui::Text("Controls: N step | B back | F forward | SPACE run/pause | T real-time 6502 | I IRQ toggle | M NMI pulse");
    ImGui::End();

    ImGui::Begin("CPU");
    cpu_register(global.cpu);
    ImGui::End();

    draw_code_window(global.cpu);
    draw_instruction_explainer_window(global.cpu);
    draw_framebuffer_window(global.cpu);

    /* helper colors */
    constexpr ImU32 COLOR_PC = IM_COL32(255, 50, 50, 255);    // red
    constexpr ImU32 COLOR_ADDR = IM_COL32(50, 150, 255, 255); // blue
    constexpr ImU32 COLOR_BOTH = IM_COL32(255, 175, 0, 255);  // orange

    auto draw_memory_window = [&](const char *title,
                                  uint16_t center_addr,
                                  size_t display_lines) {
        ImGui::Begin(title);

        constexpr size_t BYTES_PER_LINE = 16;
        size_t mem_size = global.cpu.mem.size();
        size_t max_lines = (mem_size + BYTES_PER_LINE - 1) / BYTES_PER_LINE;
        size_t center_ln = center_addr / BYTES_PER_LINE;
        size_t half_ln = static_cast<size_t>(display_lines / 2);
        size_t start_ln;
        if (center_ln > half_ln) {
            start_ln = center_ln - half_ln;
        } else {
            start_ln = 0;
        }
        if (start_ln + display_lines > max_lines) {
            start_ln = (max_lines > display_lines ? max_lines - display_lines : 0);
        }

        for (size_t line = start_ln; line < start_ln + display_lines; ++line) {
            const size_t base = line * BYTES_PER_LINE;
            ImGui::Text("0x%04zX:", base);
            ImGui::SameLine();
            for (size_t j = 0; j < BYTES_PER_LINE; ++j) {
                const size_t idx = base + j;
                if (idx >= mem_size) break;

                bool is_pc = (idx == global.cpu.PC);
                bool is_addr = (idx == global.cpu.temporary_address_register);

                if (is_pc && is_addr) {
                    ImGui::PushStyleColor(ImGuiCol_Text, COLOR_BOTH);
                } else if (is_pc) {
                    ImGui::PushStyleColor(ImGuiCol_Text, COLOR_PC);
                } else if (is_addr) {
                    ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ADDR);
                }

                ImGui::Text("%02X", static_cast<unsigned int>(global.cpu.mem[idx]));

                if (is_pc || is_addr) ImGui::PopStyleColor();

                if (j < BYTES_PER_LINE - 1) ImGui::SameLine();
            }
        }
        ImGui::End();
    };

    /* Full PC-centered memory view */
    draw_memory_window("Memory (around PC)", global.cpu.PC, 16zu);

    /* Compact ADDR-centered view */
    draw_memory_window("Addr Memory Neighborhood", global.cpu.temporary_address_register, 6);

    /* ImGui Render */
    ImGui::Render();
}

inline auto frame() -> void {
    glViewport(0, 0,
        static_cast<int>(global.renderer.imgui_io.DisplaySize.x),
        static_cast<int>(global.renderer.imgui_io.DisplaySize.y));
    glClearColor(global.color.background.r,
        global.color.background.g,
        global.color.background.b,
        1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}
} // namespace RENDER
