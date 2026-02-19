/*danielsinkin97@gmail.com*/
#pragma once

#include <array>
#include <cassert>
#include <optional>
using std::optional;
#include <variant>

#include "../types.hpp"

using TYPES::Byte;
using TYPES::Word;
using Address = Word;

namespace mos6502 {

#define INSTRUCTION_TYPE_LIST                 \
    X(NONE) /* No Instruction */              \
    X(adc)  /* Add with Carry */              \
    X(and_) /* Bitwise AND (6502 “AND”) */    \
    X(asl)  /* Arithmetic Shift Left */       \
    X(bcc)  /* Branch if Carry Clear */       \
    X(bcs)  /* Branch if Carry Set */         \
    X(beq)  /* Branch if Equal */             \
    X(bit)  /* Bit Test */                    \
    X(bmi)  /* Branch if Minus */             \
    X(bne)  /* Branch if Not Equal */         \
    X(bpl)  /* Branch if Plus */              \
    X(brk)  /* Force Break */                 \
    X(bvc)  /* Branch if Overflow Clear */    \
    X(bvs)  /* Branch if Overflow Set */      \
    X(clc)  /* Clear Carry */                 \
    X(cld)  /* Clear Decimal */               \
    X(cli)  /* Clear Interrupt Disable */     \
    X(clv)  /* Clear Overflow */              \
    X(cmp)  /* Compare Accumulator */         \
    X(cpx)  /* Compare X Register */          \
    X(cpy)  /* Compare Y Register */          \
    X(dec)  /* Decrement Memory */            \
    X(dex)  /* Decrement X Register */        \
    X(dey)  /* Decrement Y Register */        \
    X(eor)  /* Exclusive OR */                \
    X(inc)  /* Increment Memory */            \
    X(inx)  /* Increment X Register */        \
    X(iny)  /* Increment Y Register */        \
    X(jmp)  /* Jump */                        \
    X(jsr)  /* Jump to Subroutine */          \
    X(lda)  /* Load Accumulator */            \
    X(ldx)  /* Load X Register */             \
    X(ldy)  /* Load Y Register */             \
    X(lsr)  /* Logical Shift Right */         \
    X(nop)  /* No Operation */                \
    X(ora)  /* Bitwise OR with Accumulator */ \
    X(pha)  /* Push Accumulator */            \
    X(php)  /* Push Processor Status */       \
    X(pla)  /* Pull Accumulator */            \
    X(plp)  /* Pull Processor Status */       \
    X(rol)  /* Rotate Left */                 \
    X(ror)  /* Rotate Right */                \
    X(rti)  /* Return from Interrupt */       \
    X(rts)  /* Return from Subroutine */      \
    X(sbc)  /* Subtract with Carry */         \
    X(sec)  /* Set Carry */                   \
    X(sed)  /* Set Decimal */                 \
    X(sei)  /* Set Interrupt Disable */       \
    X(sta)  /* Store Accumulator */           \
    X(stx)  /* Store X Register */            \
    X(sty)  /* Store Y Register */            \
    X(tax)  /* Transfer Accumulator to X */   \
    X(tay)  /* Transfer Accumulator to Y */   \
    X(tsx)  /* Transfer Stack Pointer to X */ \
    X(txa)  /* Transfer X to Accumulator */   \
    X(txs)  /* Transfer X to Stack Pointer */ \
    X(tya)  /* Transfer Y to Accumulator */

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

[[nodiscard]] constexpr auto
is_rmw_instruction(InstructionType t) -> bool {
    constexpr std::array rmw_instructions = {
        InstructionType::asl,
        InstructionType::lsr,
        InstructionType::rol,
        InstructionType::ror,
        InstructionType::inc,
        InstructionType::dec};
    return std::ranges::contains(rmw_instructions, t);
}

[[nodiscard]] constexpr auto
is_branching_instruction(InstructionType t) -> bool {
    constexpr std::array branch_instructions = {
        InstructionType::bcc, // Branch if Carry Clear
        InstructionType::bcs, // Branch if Carry Set
        InstructionType::beq, // Branch if Equal (Zero Set)
        InstructionType::bne, // Branch if Not Equal (Zero Clear)
        InstructionType::bmi, // Branch if Minus (Negative Set)
        InstructionType::bpl, // Branch if Plus (Negative Clear)
        InstructionType::bvc, // Branch if Overflow Clear
        InstructionType::bvs  // Branch if Overflow Set
    };
    return std::ranges::contains(branch_instructions, t);
}

enum class AddressingMode {
    NONE,
    immediate,
    absolute,
    zero_page,
    accum,
    implied,
    indirect_x,
    indirect_y,
    zero_page_x,
    zero_page_y,
    absolute_x,
    absolute_y,
    relative,
    indirect,
};

inline const char *to_string(AddressingMode mode) {
    switch (mode) {
    case AddressingMode::NONE:
        return "NONE";
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
    case AddressingMode::indirect_x:
        return "indirect_x";
    case AddressingMode::indirect_y:
        return "indirect_y";
    case AddressingMode::zero_page_x:
        return "zero_page_x";
    case AddressingMode::zero_page_y:
        return "zero_page_y";
    case AddressingMode::absolute_x:
        return "absolute_x";
    case AddressingMode::absolute_y:
        return "absolute_y";
    case AddressingMode::relative:
        return "relative";
    case AddressingMode::indirect:
        return "indirect";
    default:
        assert(false);
        return "<unknown>";
    }
}

enum class AddrResultType {
    load_instruction,
    in_progress,
    complete,
    complete_value,
    complete_address,

}; // namespace mos6502

[[nodiscard]] auto is_addr_result_complete(AddrResultType type) -> bool {
    constexpr std::array<AddrResultType, 3> complete_types = {
        AddrResultType::complete,
        AddrResultType::complete_value,
        AddrResultType::complete_address,
    };
    return std::ranges::contains(complete_types, type);
}

struct AddrResult {
    AddrResultType type = AddrResultType::load_instruction;
    optional<Byte> value = std::nullopt;
    optional<Address> addr = std::nullopt;

    [[nodiscard]] auto is_complete() const -> bool {
        constexpr std::array<AddrResultType, 3> complete_types = {
            AddrResultType::complete,
            AddrResultType::complete_value,
            AddrResultType::complete_address,
        };
        return std::ranges::contains(complete_types, type);
    }

    auto validate() const -> void {
        if (type == AddrResultType::load_instruction || type == AddrResultType::in_progress) {
            assert(!value.has_value());
            assert(!addr.has_value());
        } else if (type == AddrResultType::complete_value) {
            assert(value.has_value());
            assert(!addr.has_value());
        } else if (type == AddrResultType::complete_address) {
            assert(!value.has_value());
            assert(addr.has_value());
        } else if (type == AddrResultType::complete) {
            assert(!value.has_value());
            assert(!addr.has_value());
        } else {
            assert(false);
        }
    }
};

struct CPU;
using ExecFunc = void (*)(CPU &cpu, optional<Byte>, optional<Address>);
using AddrModeFunc = AddrResult (*)(CPU, bool /*is_read*/, bool /*page_penalty*/);

struct Instruction {
    InstructionType type;
    AddressingMode mode;
};

struct Config {
    // There was a hardware bug which causes the high byte of the read address to wrap
    // around to the same page, emulators usually preserve this bugged behavior
    bool preserve_indirect_jump_page_cross_bug = true;
};
struct CPU {
    Address PC = 0x0000;
    Address current_instruction_pc = 0x0000;
    Byte A = 0x00;
    Byte X = 0x00;
    Byte Y = 0x00;
    Byte SP = 0xFD;
    Byte P = 0x00;

    bool nmi = false;
    bool irq = false;

    bool sync = false;
    bool rdy = true;

    std::array<Byte, 64 * 1024> mem = {};
    Address addr = 0x0000;
    Address temporary_address_register = 0x0000; // TAR
    Byte data_bus = 0x00;
    bool rw = true;

    Instruction instr;
    int instr_counter = 0;

    Byte tmp = 0x00;
    uint64_t cycles = 0;

    AddrResult addr_result;
    Config config;
};

constexpr Byte C_FLAG = 0b00000001; // Carry
constexpr Byte Z_FLAG = 0b00000010; // Zero
constexpr Byte I_FLAG = 0b00000100; // Interrupt Disable
constexpr Byte D_FLAG = 0b00001000; // Decimal Mode
constexpr Byte B_FLAG = 0b00010000; // Break Command
constexpr Byte U_FLAG = 0b00100000; // Unused
constexpr Byte V_FLAG = 0b01000000; // Overflow
constexpr Byte N_FLAG = 0b10000000; // Negative
inline auto set_flags_ZN(CPU &cpu, Byte v) -> void {
    cpu.P &= static_cast<Byte>(~(Z_FLAG | N_FLAG));
    if (v == 0) cpu.P |= Z_FLAG;
    if (v & N_FLAG) cpu.P |= N_FLAG;
}
inline auto set_flag_C(CPU &cpu, bool do_set) -> void {
    if (do_set) {
        cpu.P |= C_FLAG;
    } else {
        cpu.P &= static_cast<Byte>(~C_FLAG);
    }
}
inline auto set_flag_D(CPU &cpu, bool do_set) -> void {
    if (do_set) {
        cpu.P |= D_FLAG;
    } else {
        cpu.P &= static_cast<Byte>(~D_FLAG);
    }
}
inline auto set_flag_I(CPU &cpu, bool do_set) -> void {
    if (do_set) {
        cpu.P |= I_FLAG;
    } else {
        cpu.P &= static_cast<Byte>(~I_FLAG);
    }
}
inline auto set_flag_V(CPU &cpu, bool do_set) -> void {
    if (do_set) {
        cpu.P |= V_FLAG;
    } else {
        cpu.P &= static_cast<Byte>(~V_FLAG);
    }
}

[[nodiscard]] inline auto decimal_mode_enabled(const CPU &cpu) -> bool {
    return (cpu.P & D_FLAG) != 0u;
}

inline auto adc_with_mode(CPU &cpu, Byte operand) -> void {
    const auto carry_in = static_cast<Word>((cpu.P & C_FLAG) != 0u ? 1u : 0u);
    const Word binary_sum = static_cast<Word>(cpu.A) + static_cast<Word>(operand) + carry_in;
    const Byte binary_result = static_cast<Byte>(binary_sum & 0x00FFu);

    set_flag_V(cpu, ((~(cpu.A ^ operand) & (cpu.A ^ binary_result)) & 0x80u) != 0u);

    if (decimal_mode_enabled(cpu)) {
        Word decimal_sum = binary_sum;
        if (((cpu.A & 0x0Fu) + (operand & 0x0Fu) + carry_in) > 9u) {
            decimal_sum = static_cast<Word>(decimal_sum + 0x06u);
        }
        if (decimal_sum > 0x99u) {
            decimal_sum = static_cast<Word>(decimal_sum + 0x60u);
            set_flag_C(cpu, true);
        } else {
            set_flag_C(cpu, false);
        }
        cpu.A = static_cast<Byte>(decimal_sum & 0x00FFu);
    } else {
        cpu.A = binary_result;
        set_flag_C(cpu, binary_sum > 0x00FFu);
    }

    set_flags_ZN(cpu, cpu.A);
}

inline auto sbc_with_mode(CPU &cpu, Byte operand) -> void {
    const auto borrow_in = static_cast<Word>((cpu.P & C_FLAG) == 0u ? 1u : 0u);
    const Word binary_diff = static_cast<Word>(cpu.A) - static_cast<Word>(operand) - borrow_in;
    const Byte binary_result = static_cast<Byte>(binary_diff & 0x00FFu);

    set_flag_V(cpu, (((cpu.A ^ binary_result) & (cpu.A ^ operand)) & 0x80u) != 0u);
    set_flag_C(cpu, binary_diff < 0x0100u);

    if (decimal_mode_enabled(cpu)) {
        int low = static_cast<int>(cpu.A & 0x0Fu) - static_cast<int>(operand & 0x0Fu) - static_cast<int>(borrow_in);
        int high = static_cast<int>(cpu.A >> 4) - static_cast<int>(operand >> 4);

        if (low < 0) {
            low -= 6;
            --high;
        }
        if (high < 0) {
            high -= 6;
        }

        cpu.A = static_cast<Byte>(((high << 4) & 0xF0) | (low & 0x0F));
    } else {
        cpu.A = binary_result;
    }

    set_flags_ZN(cpu, cpu.A);
}

[[nodiscard]] constexpr auto stack_addr(Byte sp) -> Address {
    return static_cast<Address>(0x0100u | sp);
}

inline auto push(CPU &cpu, Byte value) -> void {
    cpu.mem[stack_addr(cpu.SP)] = value;
    --cpu.SP;
}

[[nodiscard]] inline auto pop(CPU &cpu) -> Byte {
    ++cpu.SP;
    return cpu.mem[stack_addr(cpu.SP)];
}

inline auto push_word(CPU &cpu, Address value) -> void {
    push(cpu, static_cast<Byte>((value >> 8) & 0x00FFu));
    push(cpu, static_cast<Byte>(value & 0x00FFu));
}

[[nodiscard]] inline auto pop_word(CPU &cpu) -> Address {
    Address low = static_cast<Address>(pop(cpu));
    Address high = static_cast<Address>(pop(cpu));
    return static_cast<Address>((high << 8) | low);
}

[[nodiscard]] constexpr auto instruction_uses_memory_value(InstructionType type) -> bool {
    switch (type) {
    case InstructionType::adc:
    case InstructionType::and_:
    case InstructionType::bit:
    case InstructionType::cmp:
    case InstructionType::cpx:
    case InstructionType::cpy:
    case InstructionType::eor:
    case InstructionType::lda:
    case InstructionType::ldx:
    case InstructionType::ldy:
    case InstructionType::ora:
    case InstructionType::sbc:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr auto instruction_uses_memory_address(InstructionType type) -> bool {
    switch (type) {
    case InstructionType::jmp:
    case InstructionType::jsr:
    case InstructionType::sta:
    case InstructionType::stx:
    case InstructionType::sty:
        return true;
    default:
        return is_rmw_instruction(type);
    }
}

[[nodiscard]] inline auto
resolve_operand_from_address(CPU &cpu, Address address) -> AddrResult {
    if (instruction_uses_memory_value(cpu.instr.type)) {
        return {AddrResultType::complete_value, .value = cpu.mem[address]};
    }
    if (instruction_uses_memory_address(cpu.instr.type)) {
        return {AddrResultType::complete_address, .addr = address};
    }
    assert(false);
    return {AddrResultType::complete};
}

[[nodiscard]] constexpr auto zp_add(Byte base, Byte index) -> Byte {
    return static_cast<Byte>(base + index);
}

inline auto check_branching_condition(CPU &cpu) -> bool {
    switch (cpu.instr.type) {
    case InstructionType::bcc: // Branch if Carry Clear
        return ((cpu.P & C_FLAG) == 0);
    case InstructionType::bcs: // Branch if Carry Set
        return ((cpu.P & C_FLAG) != 0);
    case InstructionType::beq: // Branch if Equal (Zero Set)
        return ((cpu.P & Z_FLAG) != 0);
    case InstructionType::bne: // Branch if Not Equal (Zero Clear)
        return ((cpu.P & Z_FLAG) == 0);
    case InstructionType::bmi: // Branch if Minus (Negative Set)
        return ((cpu.P & N_FLAG) != 0);
    case InstructionType::bpl: // Branch if Plus (Negative Clear)
        return ((cpu.P & N_FLAG) == 0);
    case InstructionType::bvc: // Branch if Overflow Clear
        return ((cpu.P & V_FLAG) == 0);
    case InstructionType::bvs: // Branch if Overflow Set
        return ((cpu.P & V_FLAG) != 0);
    default:
        assert(false);
        return false;
    }
}

[[nodiscard]] inline auto read_vector(CPU &cpu, Address vector_low_addr) -> Address {
    const Address low = static_cast<Address>(cpu.mem[vector_low_addr]);
    const Address high = static_cast<Address>(cpu.mem[static_cast<Address>(vector_low_addr + 1u)]);
    return static_cast<Address>((high << 8) | low);
}

inline auto service_interrupt(CPU &cpu, Address vector_low_addr) -> void {
    push_word(cpu, cpu.PC);
    push(cpu, static_cast<Byte>((cpu.P | U_FLAG) & static_cast<Byte>(~B_FLAG)));
    set_flag_I(cpu, true);
    cpu.PC = read_vector(cpu, vector_low_addr);
    cpu.current_instruction_pc = cpu.PC;
    cpu.addr_result = {AddrResultType::load_instruction};
    cpu.instr_counter = 0;
}

struct CPUSnapshot {
    CPU cpu;
};

// Note that uint16_t overflowing is part of the C++ standard and not UB so this is safe
[[nodiscard]] auto fetch(CPU &cpu) -> Byte { return cpu.mem[cpu.PC++]; }
auto fetch_to_tmp(CPU &cpu) -> void { cpu.tmp = fetch(cpu); }
[[nodiscard]] auto read(CPU &cpu, Address addr) -> Byte { return cpu.mem[addr]; }
auto read_tar(CPU &cpu) -> void { cpu.tmp = cpu.mem[cpu.temporary_address_register]; }

// Combines current location of PC as high part with cpu.tmp value as low part into one address
// and stores it in cpu.temporary_address_register
auto fetch_to_tar(CPU &cpu) -> void {
    cpu.temporary_address_register = static_cast<Address>(fetch(cpu) << 8) | static_cast<Address>(cpu.tmp);
}
auto write(CPU &cpu, Address addr, Byte val) -> void { cpu.mem[addr] = val; }

inline auto exec_func(CPU &cpu, optional<Byte> value, optional<Address> addr) -> void {
    auto cmp = [&](Byte lhs, Byte rhs) -> void {
        set_flag_C(cpu, lhs >= rhs);
        set_flags_ZN(cpu, static_cast<Byte>(lhs - rhs));
    };

    switch (cpu.instr.type) {
    case InstructionType::adc: {
        assert(value.has_value());
        adc_with_mode(cpu, *value);
        break;
    }
    case InstructionType::and_: {
        assert(value.has_value());
        cpu.A = static_cast<Byte>(cpu.A & *value);
        set_flags_ZN(cpu, cpu.A);
        break;
    }
    case InstructionType::asl: {
        Byte read_value;
        if (cpu.instr.mode == AddressingMode::accum) {
            read_value = cpu.A;
        } else {
            assert(addr.has_value());
            read_value = read(cpu, *addr);
        }
        set_flag_C(cpu, (read_value & 0x80u) != 0u);
        read_value = static_cast<Byte>(read_value << 1);
        set_flags_ZN(cpu, read_value);
        if (cpu.instr.mode == AddressingMode::accum) {
            cpu.A = read_value;
        } else {
            write(cpu, *addr, read_value);
        }
        break;
    }
    case InstructionType::bit: {
        assert(value.has_value());
        Byte masked = static_cast<Byte>(cpu.A & *value);
        if (masked == 0u) {
            cpu.P |= Z_FLAG;
        } else {
            cpu.P &= static_cast<Byte>(~Z_FLAG);
        }
        if ((*value & N_FLAG) != 0u) {
            cpu.P |= N_FLAG;
        } else {
            cpu.P &= static_cast<Byte>(~N_FLAG);
        }
        if ((*value & V_FLAG) != 0u) {
            cpu.P |= V_FLAG;
        } else {
            cpu.P &= static_cast<Byte>(~V_FLAG);
        }
        break;
    }
    case InstructionType::brk: {
        Address return_addr = static_cast<Address>(cpu.PC + 1);
        push_word(cpu, return_addr);
        push(cpu, static_cast<Byte>(cpu.P | B_FLAG | U_FLAG));
        set_flag_I(cpu, true);
        Address vector = static_cast<Address>((static_cast<Address>(read(cpu, static_cast<Address>(0xFFFFu))) << 8) |
                                              static_cast<Address>(read(cpu, static_cast<Address>(0xFFFEu))));
        cpu.PC = vector;
        break;
    }
    case InstructionType::clc:
        set_flag_C(cpu, false);
        break;
    case InstructionType::cld:
        set_flag_D(cpu, false);
        break;
    case InstructionType::cli:
        set_flag_I(cpu, false);
        break;
    case InstructionType::clv:
        set_flag_V(cpu, false);
        break;
    case InstructionType::cmp:
        assert(value.has_value());
        cmp(cpu.A, *value);
        break;
    case InstructionType::cpx:
        assert(value.has_value());
        cmp(cpu.X, *value);
        break;
    case InstructionType::cpy:
        assert(value.has_value());
        cmp(cpu.Y, *value);
        break;
    case InstructionType::dec: {
        assert(addr.has_value());
        Byte result = static_cast<Byte>(read(cpu, *addr) - 1u);
        write(cpu, *addr, result);
        set_flags_ZN(cpu, result);
        break;
    }
    case InstructionType::dex:
        cpu.X = static_cast<Byte>(cpu.X - 1u);
        set_flags_ZN(cpu, cpu.X);
        break;
    case InstructionType::dey:
        cpu.Y = static_cast<Byte>(cpu.Y - 1u);
        set_flags_ZN(cpu, cpu.Y);
        break;
    case InstructionType::eor:
        assert(value.has_value());
        cpu.A = static_cast<Byte>(cpu.A ^ *value);
        set_flags_ZN(cpu, cpu.A);
        break;
    case InstructionType::inc: {
        assert(addr.has_value());
        Byte result = static_cast<Byte>(read(cpu, *addr) + 1u);
        write(cpu, *addr, result);
        set_flags_ZN(cpu, result);
        break;
    }
    case InstructionType::inx:
        cpu.X = static_cast<Byte>(cpu.X + 1u);
        set_flags_ZN(cpu, cpu.X);
        break;
    case InstructionType::iny:
        cpu.Y = static_cast<Byte>(cpu.Y + 1u);
        set_flags_ZN(cpu, cpu.Y);
        break;
    case InstructionType::jmp:
        assert(addr.has_value());
        cpu.PC = *addr;
        break;
    case InstructionType::jsr: {
        assert(addr.has_value());
        push_word(cpu, static_cast<Address>(cpu.PC - 1u));
        cpu.PC = *addr;
        break;
    }
    case InstructionType::lda:
        assert(value.has_value());
        cpu.A = *value;
        set_flags_ZN(cpu, cpu.A);
        break;
    case InstructionType::ldx:
        assert(value.has_value());
        cpu.X = *value;
        set_flags_ZN(cpu, cpu.X);
        break;
    case InstructionType::ldy:
        assert(value.has_value());
        cpu.Y = *value;
        set_flags_ZN(cpu, cpu.Y);
        break;
    case InstructionType::lsr: {
        Byte read_value;
        if (cpu.instr.mode == AddressingMode::accum) {
            read_value = cpu.A;
        } else {
            assert(addr.has_value());
            read_value = read(cpu, *addr);
        }
        set_flag_C(cpu, (read_value & 0x01u) != 0u);
        read_value = static_cast<Byte>(read_value >> 1);
        set_flags_ZN(cpu, read_value);
        if (cpu.instr.mode == AddressingMode::accum) {
            cpu.A = read_value;
        } else {
            write(cpu, *addr, read_value);
        }
        break;
    }
    case InstructionType::nop:
        break;
    case InstructionType::ora:
        assert(value.has_value());
        cpu.A = static_cast<Byte>(cpu.A | *value);
        set_flags_ZN(cpu, cpu.A);
        break;
    case InstructionType::pha:
        push(cpu, cpu.A);
        break;
    case InstructionType::php:
        push(cpu, static_cast<Byte>(cpu.P | B_FLAG | U_FLAG));
        break;
    case InstructionType::pla:
        cpu.A = pop(cpu);
        set_flags_ZN(cpu, cpu.A);
        break;
    case InstructionType::plp:
        cpu.P = static_cast<Byte>((pop(cpu) | U_FLAG) & static_cast<Byte>(~B_FLAG));
        break;
    case InstructionType::rol: {
        Byte read_value;
        if (cpu.instr.mode == AddressingMode::accum) {
            read_value = cpu.A;
        } else {
            assert(addr.has_value());
            read_value = read(cpu, *addr);
        }
        Byte carry_in = ((cpu.P & C_FLAG) != 0u) ? 1u : 0u;
        set_flag_C(cpu, (read_value & 0x80u) != 0u);
        Byte result = static_cast<Byte>((read_value << 1) | carry_in);
        set_flags_ZN(cpu, result);
        if (cpu.instr.mode == AddressingMode::accum) {
            cpu.A = result;
        } else {
            write(cpu, *addr, result);
        }
        break;
    }
    case InstructionType::ror: {
        Byte read_value;
        if (cpu.instr.mode == AddressingMode::accum) {
            read_value = cpu.A;
        } else {
            assert(addr.has_value());
            read_value = read(cpu, *addr);
        }
        Byte carry_in = ((cpu.P & C_FLAG) != 0u) ? 0x80u : 0x00u;
        set_flag_C(cpu, (read_value & 0x01u) != 0u);
        Byte result = static_cast<Byte>((read_value >> 1) | carry_in);
        set_flags_ZN(cpu, result);
        if (cpu.instr.mode == AddressingMode::accum) {
            cpu.A = result;
        } else {
            write(cpu, *addr, result);
        }
        break;
    }
    case InstructionType::rti:
        cpu.P = static_cast<Byte>((pop(cpu) | U_FLAG) & static_cast<Byte>(~B_FLAG));
        cpu.PC = pop_word(cpu);
        break;
    case InstructionType::rts:
        cpu.PC = static_cast<Address>(pop_word(cpu) + 1u);
        break;
    case InstructionType::sbc: {
        assert(value.has_value());
        sbc_with_mode(cpu, *value);
        break;
    }
    case InstructionType::sec:
        set_flag_C(cpu, true);
        break;
    case InstructionType::sed:
        set_flag_D(cpu, true);
        break;
    case InstructionType::sei:
        set_flag_I(cpu, true);
        break;
    case InstructionType::sta:
        assert(addr.has_value());
        write(cpu, *addr, cpu.A);
        break;
    case InstructionType::stx:
        assert(addr.has_value());
        write(cpu, *addr, cpu.X);
        break;
    case InstructionType::sty:
        assert(addr.has_value());
        write(cpu, *addr, cpu.Y);
        break;
    case InstructionType::tax:
        cpu.X = cpu.A;
        set_flags_ZN(cpu, cpu.X);
        break;
    case InstructionType::tay:
        cpu.Y = cpu.A;
        set_flags_ZN(cpu, cpu.Y);
        break;
    case InstructionType::tsx:
        cpu.X = cpu.SP;
        set_flags_ZN(cpu, cpu.X);
        break;
    case InstructionType::txa:
        cpu.A = cpu.X;
        set_flags_ZN(cpu, cpu.A);
        break;
    case InstructionType::txs:
        cpu.SP = cpu.X;
        break;
    case InstructionType::tya:
        cpu.A = cpu.Y;
        set_flags_ZN(cpu, cpu.A);
        break;
    default:
        assert(false);
    }
}

std::array<Instruction, 256> instructions{};
void initialize_instructions() {
    /* 1.  Set every slot to “no instruction” ------------------------ */
    instructions.fill({InstructionType::NONE, AddressingMode::NONE});

    /* 2.  Official 6510/6502 instruction set (151 opcodes) ---------- */

    /* $00-$1F -------------------------------------------------------- */
    instructions[0x00] = {InstructionType::brk, AddressingMode::implied};
    instructions[0x01] = {InstructionType::ora, AddressingMode::indirect_x};
    instructions[0x05] = {InstructionType::ora, AddressingMode::zero_page};
    instructions[0x06] = {InstructionType::asl, AddressingMode::zero_page};
    instructions[0x08] = {InstructionType::php, AddressingMode::implied};
    instructions[0x09] = {InstructionType::ora, AddressingMode::immediate};
    instructions[0x0A] = {InstructionType::asl, AddressingMode::accum};
    instructions[0x0D] = {InstructionType::ora, AddressingMode::absolute};
    instructions[0x0E] = {InstructionType::asl, AddressingMode::absolute};
    instructions[0x10] = {InstructionType::bpl, AddressingMode::relative};
    instructions[0x11] = {InstructionType::ora, AddressingMode::indirect_y};
    instructions[0x15] = {InstructionType::ora, AddressingMode::zero_page_x};
    instructions[0x16] = {InstructionType::asl, AddressingMode::zero_page_x};
    instructions[0x18] = {InstructionType::clc, AddressingMode::implied};
    instructions[0x19] = {InstructionType::ora, AddressingMode::absolute_y};
    instructions[0x1D] = {InstructionType::ora, AddressingMode::absolute_x};
    instructions[0x1E] = {InstructionType::asl, AddressingMode::absolute_x};

    /* $20-$3F -------------------------------------------------------- */
    instructions[0x20] = {InstructionType::jsr, AddressingMode::absolute};
    instructions[0x21] = {InstructionType::and_, AddressingMode::indirect_x};
    instructions[0x24] = {InstructionType::bit, AddressingMode::zero_page};
    instructions[0x25] = {InstructionType::and_, AddressingMode::zero_page};
    instructions[0x26] = {InstructionType::rol, AddressingMode::zero_page};
    instructions[0x28] = {InstructionType::plp, AddressingMode::implied};
    instructions[0x29] = {InstructionType::and_, AddressingMode::immediate};
    instructions[0x2A] = {InstructionType::rol, AddressingMode::accum};
    instructions[0x2C] = {InstructionType::bit, AddressingMode::absolute};
    instructions[0x2D] = {InstructionType::and_, AddressingMode::absolute};
    instructions[0x2E] = {InstructionType::rol, AddressingMode::absolute};
    instructions[0x30] = {InstructionType::bmi, AddressingMode::relative};
    instructions[0x31] = {InstructionType::and_, AddressingMode::indirect_y};
    instructions[0x35] = {InstructionType::and_, AddressingMode::zero_page_x};
    instructions[0x36] = {InstructionType::rol, AddressingMode::zero_page_x};
    instructions[0x38] = {InstructionType::sec, AddressingMode::implied};
    instructions[0x39] = {InstructionType::and_, AddressingMode::absolute_y};
    instructions[0x3D] = {InstructionType::and_, AddressingMode::absolute_x};
    instructions[0x3E] = {InstructionType::rol, AddressingMode::absolute_x};

    /* $40-$5F -------------------------------------------------------- */
    instructions[0x40] = {InstructionType::rti, AddressingMode::implied};
    instructions[0x41] = {InstructionType::eor, AddressingMode::indirect_x};
    instructions[0x45] = {InstructionType::eor, AddressingMode::zero_page};
    instructions[0x46] = {InstructionType::lsr, AddressingMode::zero_page};
    instructions[0x48] = {InstructionType::pha, AddressingMode::implied};
    instructions[0x49] = {InstructionType::eor, AddressingMode::immediate};
    instructions[0x4A] = {InstructionType::lsr, AddressingMode::accum};
    instructions[0x4C] = {InstructionType::jmp, AddressingMode::absolute};
    instructions[0x4D] = {InstructionType::eor, AddressingMode::absolute};
    instructions[0x4E] = {InstructionType::lsr, AddressingMode::absolute};
    instructions[0x50] = {InstructionType::bvc, AddressingMode::relative};
    instructions[0x51] = {InstructionType::eor, AddressingMode::indirect_y};
    instructions[0x55] = {InstructionType::eor, AddressingMode::zero_page_x};
    instructions[0x56] = {InstructionType::lsr, AddressingMode::zero_page_x};
    instructions[0x58] = {InstructionType::cli, AddressingMode::implied};
    instructions[0x59] = {InstructionType::eor, AddressingMode::absolute_y};
    instructions[0x5D] = {InstructionType::eor, AddressingMode::absolute_x};
    instructions[0x5E] = {InstructionType::lsr, AddressingMode::absolute_x};

    /* $60-$7F -------------------------------------------------------- */
    instructions[0x60] = {InstructionType::rts, AddressingMode::implied};
    instructions[0x61] = {InstructionType::adc, AddressingMode::indirect_x};
    instructions[0x65] = {InstructionType::adc, AddressingMode::zero_page};
    instructions[0x66] = {InstructionType::ror, AddressingMode::zero_page};
    instructions[0x68] = {InstructionType::pla, AddressingMode::implied};
    instructions[0x69] = {InstructionType::adc, AddressingMode::immediate};
    instructions[0x6A] = {InstructionType::ror, AddressingMode::accum};
    instructions[0x6C] = {InstructionType::jmp, AddressingMode::indirect};
    instructions[0x6D] = {InstructionType::adc, AddressingMode::absolute};
    instructions[0x6E] = {InstructionType::ror, AddressingMode::absolute};
    instructions[0x70] = {InstructionType::bvs, AddressingMode::relative};
    instructions[0x71] = {InstructionType::adc, AddressingMode::indirect_y};
    instructions[0x75] = {InstructionType::adc, AddressingMode::zero_page_x};
    instructions[0x76] = {InstructionType::ror, AddressingMode::zero_page_x};
    instructions[0x78] = {InstructionType::sei, AddressingMode::implied};
    instructions[0x79] = {InstructionType::adc, AddressingMode::absolute_y};
    instructions[0x7D] = {InstructionType::adc, AddressingMode::absolute_x};
    instructions[0x7E] = {InstructionType::ror, AddressingMode::absolute_x};

    /* $80-$9F -------------------------------------------------------- */
    instructions[0x81] = {InstructionType::sta, AddressingMode::indirect_x};
    instructions[0x84] = {InstructionType::sty, AddressingMode::zero_page};
    instructions[0x85] = {InstructionType::sta, AddressingMode::zero_page};
    instructions[0x86] = {InstructionType::stx, AddressingMode::zero_page};
    instructions[0x88] = {InstructionType::dey, AddressingMode::implied};
    instructions[0x8A] = {InstructionType::txa, AddressingMode::implied};
    instructions[0x8C] = {InstructionType::sty, AddressingMode::absolute};
    instructions[0x8D] = {InstructionType::sta, AddressingMode::absolute};
    instructions[0x8E] = {InstructionType::stx, AddressingMode::absolute};
    instructions[0x90] = {InstructionType::bcc, AddressingMode::relative};
    instructions[0x91] = {InstructionType::sta, AddressingMode::indirect_y};
    instructions[0x94] = {InstructionType::sty, AddressingMode::zero_page_x};
    instructions[0x95] = {InstructionType::sta, AddressingMode::zero_page_x};
    instructions[0x96] = {InstructionType::stx, AddressingMode::zero_page_y};
    instructions[0x98] = {InstructionType::tya, AddressingMode::implied};
    instructions[0x99] = {InstructionType::sta, AddressingMode::absolute_y};
    instructions[0x9A] = {InstructionType::txs, AddressingMode::implied};
    instructions[0x9D] = {InstructionType::sta, AddressingMode::absolute_x};

    /* $A0-$BF -------------------------------------------------------- */
    instructions[0xA0] = {InstructionType::ldy, AddressingMode::immediate};
    instructions[0xA1] = {InstructionType::lda, AddressingMode::indirect_x};
    instructions[0xA2] = {InstructionType::ldx, AddressingMode::immediate};
    instructions[0xA4] = {InstructionType::ldy, AddressingMode::zero_page};
    instructions[0xA5] = {InstructionType::lda, AddressingMode::zero_page};
    instructions[0xA6] = {InstructionType::ldx, AddressingMode::zero_page};
    instructions[0xA8] = {InstructionType::tay, AddressingMode::implied};
    instructions[0xA9] = {InstructionType::lda, AddressingMode::immediate};
    instructions[0xAA] = {InstructionType::tax, AddressingMode::implied};
    instructions[0xAC] = {InstructionType::ldy, AddressingMode::absolute};
    instructions[0xAD] = {InstructionType::lda, AddressingMode::absolute};
    instructions[0xAE] = {InstructionType::ldx, AddressingMode::absolute};

    instructions[0xB0] = {InstructionType::bcs, AddressingMode::relative};
    instructions[0xB1] = {InstructionType::lda, AddressingMode::indirect_y};
    instructions[0xB4] = {InstructionType::ldy, AddressingMode::zero_page_x};
    instructions[0xB5] = {InstructionType::lda, AddressingMode::zero_page_x};
    instructions[0xB6] = {InstructionType::ldx, AddressingMode::zero_page_y};
    instructions[0xB8] = {InstructionType::clv, AddressingMode::implied};
    instructions[0xB9] = {InstructionType::lda, AddressingMode::absolute_y};
    instructions[0xBA] = {InstructionType::tsx, AddressingMode::implied};
    instructions[0xBC] = {InstructionType::ldy, AddressingMode::absolute_x};
    instructions[0xBD] = {InstructionType::lda, AddressingMode::absolute_x};
    instructions[0xBE] = {InstructionType::ldx, AddressingMode::absolute_y};

    /* $C0-$DF -------------------------------------------------------- */
    instructions[0xC0] = {InstructionType::cpy, AddressingMode::immediate};
    instructions[0xC1] = {InstructionType::cmp, AddressingMode::indirect_x};
    instructions[0xC4] = {InstructionType::cpy, AddressingMode::zero_page};
    instructions[0xC5] = {InstructionType::cmp, AddressingMode::zero_page};
    instructions[0xC6] = {InstructionType::dec, AddressingMode::zero_page};
    instructions[0xC8] = {InstructionType::iny, AddressingMode::implied};
    instructions[0xC9] = {InstructionType::cmp, AddressingMode::immediate};
    instructions[0xCA] = {InstructionType::dex, AddressingMode::implied};
    instructions[0xCC] = {InstructionType::cpy, AddressingMode::absolute};
    instructions[0xCD] = {InstructionType::cmp, AddressingMode::absolute};
    instructions[0xCE] = {InstructionType::dec, AddressingMode::absolute};
    instructions[0xD0] = {InstructionType::bne, AddressingMode::relative};
    instructions[0xD1] = {InstructionType::cmp, AddressingMode::indirect_y};
    instructions[0xD5] = {InstructionType::cmp, AddressingMode::zero_page_x};
    instructions[0xD6] = {InstructionType::dec, AddressingMode::zero_page_x};
    instructions[0xD8] = {InstructionType::cld, AddressingMode::implied};
    instructions[0xD9] = {InstructionType::cmp, AddressingMode::absolute_y};
    instructions[0xDD] = {InstructionType::cmp, AddressingMode::absolute_x};
    instructions[0xDE] = {InstructionType::dec, AddressingMode::absolute_x};

    /* $E0-$FF -------------------------------------------------------- */
    instructions[0xE0] = {InstructionType::cpx, AddressingMode::immediate};
    instructions[0xE1] = {InstructionType::sbc, AddressingMode::indirect_x};
    instructions[0xE4] = {InstructionType::cpx, AddressingMode::zero_page};
    instructions[0xE5] = {InstructionType::sbc, AddressingMode::zero_page};
    instructions[0xE6] = {InstructionType::inc, AddressingMode::zero_page};
    instructions[0xE8] = {InstructionType::inx, AddressingMode::implied};
    instructions[0xE9] = {InstructionType::sbc, AddressingMode::immediate};
    instructions[0xEA] = {InstructionType::nop, AddressingMode::implied};
    instructions[0xEC] = {InstructionType::cpx, AddressingMode::absolute};
    instructions[0xED] = {InstructionType::sbc, AddressingMode::absolute};
    instructions[0xEE] = {InstructionType::inc, AddressingMode::absolute};
    instructions[0xF0] = {InstructionType::beq, AddressingMode::relative};
    instructions[0xF1] = {InstructionType::sbc, AddressingMode::indirect_y};
    instructions[0xF5] = {InstructionType::sbc, AddressingMode::zero_page_x};
    instructions[0xF6] = {InstructionType::inc, AddressingMode::zero_page_x};
    instructions[0xF8] = {InstructionType::sed, AddressingMode::implied};
    instructions[0xF9] = {InstructionType::sbc, AddressingMode::absolute_y};
    instructions[0xFD] = {InstructionType::sbc, AddressingMode::absolute_x};
    instructions[0xFE] = {InstructionType::inc, AddressingMode::absolute_x};
}

inline auto addr_mode(CPU &cpu) -> AddrResult {
    switch (cpu.instr.mode) {
    case AddressingMode::NONE:
        assert(false);
        return {AddrResultType::complete};
    case AddressingMode::accum:
    case AddressingMode::implied:
        return {AddrResultType::complete};
    case AddressingMode::immediate:
        return {AddrResultType::complete_value, .value = fetch(cpu)};
    case AddressingMode::zero_page: {
        Address addr = static_cast<Address>(fetch(cpu));
        cpu.temporary_address_register = addr;
        return resolve_operand_from_address(cpu, addr);
    }
    case AddressingMode::zero_page_x: {
        Address addr = static_cast<Address>(zp_add(fetch(cpu), cpu.X));
        cpu.temporary_address_register = addr;
        return resolve_operand_from_address(cpu, addr);
    }
    case AddressingMode::zero_page_y: {
        Address addr = static_cast<Address>(zp_add(fetch(cpu), cpu.Y));
        cpu.temporary_address_register = addr;
        return resolve_operand_from_address(cpu, addr);
    }
    case AddressingMode::absolute: {
        Address low = static_cast<Address>(fetch(cpu));
        Address high = static_cast<Address>(fetch(cpu));
        Address addr = static_cast<Address>((high << 8) | low);
        cpu.temporary_address_register = addr;
        return resolve_operand_from_address(cpu, addr);
    }
    case AddressingMode::absolute_x: {
        Address low = static_cast<Address>(fetch(cpu));
        Address high = static_cast<Address>(fetch(cpu));
        Address base = static_cast<Address>((high << 8) | low);
        Address addr = static_cast<Address>(base + cpu.X);
        cpu.temporary_address_register = addr;
        return resolve_operand_from_address(cpu, addr);
    }
    case AddressingMode::absolute_y: {
        Address low = static_cast<Address>(fetch(cpu));
        Address high = static_cast<Address>(fetch(cpu));
        Address base = static_cast<Address>((high << 8) | low);
        Address addr = static_cast<Address>(base + cpu.Y);
        cpu.temporary_address_register = addr;
        return resolve_operand_from_address(cpu, addr);
    }
    case AddressingMode::indirect_x: {
        Byte zp_ptr = zp_add(fetch(cpu), cpu.X);
        Address low_addr = static_cast<Address>(zp_ptr);
        Address high_addr = static_cast<Address>(zp_add(zp_ptr, 1));
        Address addr = static_cast<Address>((static_cast<Address>(read(cpu, high_addr)) << 8) |
                                            static_cast<Address>(read(cpu, low_addr)));
        cpu.temporary_address_register = addr;
        return resolve_operand_from_address(cpu, addr);
    }
    case AddressingMode::indirect_y: {
        Byte zp_ptr = fetch(cpu);
        Address low_addr = static_cast<Address>(zp_ptr);
        Address high_addr = static_cast<Address>(zp_add(zp_ptr, 1));
        Address base = static_cast<Address>((static_cast<Address>(read(cpu, high_addr)) << 8) |
                                            static_cast<Address>(read(cpu, low_addr)));
        Address addr = static_cast<Address>(base + cpu.Y);
        cpu.temporary_address_register = addr;
        return resolve_operand_from_address(cpu, addr);
    }
    case AddressingMode::relative:
        assert(false); // handled separately
        return {AddrResultType::complete};
    case AddressingMode::indirect: {
        Address low = static_cast<Address>(fetch(cpu));
        Address high = static_cast<Address>(fetch(cpu));
        Address ptr = static_cast<Address>((high << 8) | low);
        Address high_ptr;
        bool page_crossed = (ptr & 0x00FFu) == 0x00FFu;
        if (page_crossed && cpu.config.preserve_indirect_jump_page_cross_bug) {
            high_ptr = ptr & 0xFF00u;
        } else {
            high_ptr = static_cast<Address>(ptr + 1u);
        }
        Address addr = static_cast<Address>((static_cast<Address>(read(cpu, high_ptr)) << 8) |
                                            static_cast<Address>(read(cpu, ptr)));
        cpu.temporary_address_register = addr;
        return {AddrResultType::complete_address, .addr = addr};
    }
}
}

inline auto finished_instruction(CPU &cpu) -> void {
    cpu.addr_result = {AddrResultType::load_instruction};
    cpu.instr_counter = 0;
}

inline auto handle_branching_instruction(CPU &cpu) -> void {
    switch (cpu.instr_counter) {
    case 1:
        fetch_to_tmp(cpu);
        if (!check_branching_condition(cpu)) {
            finished_instruction(cpu);
            return;
        }
        cpu.addr_result = {AddrResultType::in_progress};
        ++cpu.instr_counter;
        break;
    case 2: {
        cpu.temporary_address_register = static_cast<Address>(cpu.PC + static_cast<int8_t>(cpu.tmp));
        bool same_page = (cpu.temporary_address_register & 0xFF00) == (cpu.PC & 0xFF00);
        if (same_page) {
            cpu.PC = cpu.temporary_address_register;
            finished_instruction(cpu);
        } else {
            cpu.addr_result = {AddrResultType::in_progress};
            ++cpu.instr_counter;
        }
        break;
    }
    case 3:
        cpu.PC = cpu.temporary_address_register;
        finished_instruction(cpu);
        break;
    default:
        assert(false);
        break;
    }
}

inline auto tick(CPU &cpu) -> void {
    ++cpu.cycles;
    cpu.sync = false;
    cpu.addr_result.validate();

    if (!cpu.rdy) {
        return;
    }

    if (cpu.addr_result.type == AddrResultType::load_instruction) {
        assert(cpu.instr_counter == 0);

        if (cpu.nmi) {
            service_interrupt(cpu, static_cast<Address>(0xFFFAu));
            cpu.nmi = false;
            return;
        }
        if (cpu.irq && ((cpu.P & I_FLAG) == 0u)) {
            service_interrupt(cpu, static_cast<Address>(0xFFFEu));
            return;
        }

        // Fetch instruction
        cpu.sync = true;
        cpu.current_instruction_pc = cpu.PC;
        Byte opcode = fetch(cpu);
        cpu.instr = instructions[opcode];
        cpu.instr_counter = 1;
        cpu.addr_result = {AddrResultType::in_progress};
        return;
    }

    if (is_branching_instruction(cpu.instr.type)) {
        handle_branching_instruction(cpu);
        return;
    }

    cpu.addr_result = addr_mode(cpu);
    cpu.addr_result.validate();
    ++cpu.instr_counter;

    if (cpu.addr_result.is_complete()) {
        exec_func(cpu, cpu.addr_result.value, cpu.addr_result.addr);
        finished_instruction(cpu);
        return;
    }
}
} // namespace mos6502
