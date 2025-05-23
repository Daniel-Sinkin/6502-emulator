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
    Address addr = 0x0000;
    Address temporary_address_register = 0x0000; // TAR
    Byte data_bus = 0x00;
    bool rw;

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
    }
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
    if (cpu.instr.mode == AddressingMode::accum) {
        assert(!value.has_value() && !addr.has_value());
    }

    switch (cpu.instr.type) {
    case InstructionType::adc:
        assert(false);
    case InstructionType::and_: {
        cpu.A &= *value;
        set_flags_ZN(cpu, cpu.A);
        break;
    }
    case InstructionType::asl: {
        Byte read_value;
        if (cpu.instr.mode == AddressingMode::accum) {
            read_value = cpu.A;
        } else {
            read_value = read(cpu, *addr);
        }
        set_flag_C(cpu, read_value & 0x80);
        read_value <<= 1;
        set_flags_ZN(cpu, read_value);
        if (cpu.instr.mode == AddressingMode::accum) {
            cpu.A = read_value;
        } else {
            write(cpu, *addr, read_value);
        }
        break;
    }
    case InstructionType::lda:
        set_flags_ZN(cpu, *value);
        cpu.A = *value;
        break;
    case InstructionType::jmp:
        cpu.PC = *addr;
        break;
    case InstructionType::nop:
        break;
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
        cpu.mem[*addr] = cpu.A;
        break;
    case InstructionType::stx:
        cpu.mem[*addr] = cpu.X;
        break;
    case InstructionType::sty:
        cpu.mem[*addr] = cpu.Y;
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
    case InstructionType::txs:
        cpu.SP = cpu.X;
        set_flags_ZN(cpu, cpu.SP);
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
    if (cpu.instr_counter < 1) assert(false);
    switch (cpu.instr.mode) {
    case AddressingMode::implied:
        assert(cpu.instr_counter == 1);
        return {AddrResultType::complete};
    case AddressingMode::zero_page: // operand is zeropage address (hi-byte is zero, address = $00LL)
        assert(cpu.instr_counter == 1);
        return {AddrResultType::complete_value, .value = fetch(cpu)};
    case AddressingMode::immediate:
        if (cpu.instr_counter != 1) assert(false);
        return {AddrResultType::complete_value, .value = fetch(cpu)};
        break;
    case AddressingMode::absolute:
        if (cpu.instr_counter == 1) {
            fetch_to_tmp(cpu);
            return {AddrResultType::in_progress};
        } else if (cpu.instr_counter == 2) {
            Address addr = static_cast<Address>(fetch(cpu) << 8) | static_cast<Address>(cpu.tmp);
            return {AddrResultType::complete_address, .addr = addr};
        } else {
            assert(false);
        }
        break;
    case AddressingMode::relative:
        assert(false); // Should be handeled seperately
    case AddressingMode::indirect:
        switch (cpu.instr_counter) {
        case 1:
            cpu.tmp = fetch(cpu);
            return {AddrResultType::in_progress};
            break;
        case 2:
            fetch_to_tar(cpu);
            return {AddrResultType::in_progress};
            break;
        case 3:
            read_tar(cpu);
            return {AddrResultType::in_progress};
            break;
        case 4: {
            Address high_addr;
            bool page_crossed = (cpu.temporary_address_register & 0x00FF) == 0x00FF;
            if (page_crossed && cpu.config.preserve_indirect_jump_page_cross_bug) {
                high_addr = cpu.temporary_address_register & 0xFF00;
            } else {
                high_addr = cpu.temporary_address_register + 1;
            }
            Address high_ = static_cast<Address>(read(cpu, high_addr) << 8);
            Address addr = high_ | static_cast<Address>(cpu.tmp);
            return {AddrResultType::complete_address, .addr = addr};
            break;
        }
        default:
            assert(false);
        }
        break;
    default:
        assert(false);
    }
}
inline auto addr_mode_rmw(CPU &cpu) -> AddrResult {
    switch (cpu.instr.mode) {
    case AddressingMode::absolute:
        switch (cpu.instr_counter) {
        case 1:
            cpu.addr = cpu.PC;
            fetch_to_tmp(cpu);
            return {AddrResultType::in_progress};
            break;
        case 2:
            fetch_to_tar(cpu);
            return {AddrResultType::in_progress};
            break;
        case 3:
            read_tar(cpu);
            return {AddrResultType::in_progress};
            break;
        case 4:
            write(cpu, cpu.addr, cpu.tmp); // Dummy Write
            return {AddrResultType::in_progress};
            break;
        case 5:
            return {AddrResultType::complete_address, .addr = cpu.addr};
            break;
        default:
            assert(false);
        }
        break;
    case AddressingMode::zero_page:
        assert(false);
        break;
    case AddressingMode::zero_page_x:
        assert(false);
        break;
    case AddressingMode::absolute_x:
        assert(false);
        break;
    case AddressingMode::accum:
        assert(false);
        break;
    default:
        assert(false);
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
    cpu.addr_result.validate();
    if (cpu.addr_result.type == AddrResultType::load_instruction) {
        assert(cpu.instr_counter == 0);
        // Fetch instruction
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

    if (is_rmw_instruction(cpu.instr.type)) {
        cpu.addr_result = addr_mode_rmw(cpu);
    } else {
        cpu.addr_result = addr_mode(cpu);
    }
    cpu.addr_result.validate();
    ++cpu.instr_counter;

    if (cpu.addr_result.is_complete()) {
        exec_func(cpu, cpu.addr_result.value, cpu.addr_result.addr);
        finished_instruction(cpu);
        return;
    }
}
} // namespace mos6502
