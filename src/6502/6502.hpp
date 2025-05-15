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

enum class AddressingMode {
    NONE,
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
        assert(false);
    }
}

enum class AddrResultType {
    load_instruction,
    in_progress,
    complete,
    complete_value,
    complete_address,
};

struct AddrResult {
    AddrResultType type = AddrResultType::load_instruction;
    optional<Byte> value = std::nullopt;
    optional<Address> addr = std::nullopt;

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
    Byte opcode;
    AddressingMode mode;
    bool is_read;
    bool has_page_penalty;
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

class ProgramWriter {
public:
    Address addr;

    ProgramWriter(CPU &cpu, Address addr = 0x0000)
        : addr(addr), cpu(cpu) {}

    void operator()(Byte value) {
        cpu.mem[addr++] = value;
    }

    void lda_immediate() { (*this)(0xA9); }
    void jmp_absolute() { (*this)(0x4C); }
    void jmp_indirect() { (*this)(0x6C); }

private:
    CPU &cpu;
};

inline auto exec_func(CPU &cpu, optional<Byte> value, optional<Address> addr) -> void {
    if (cpu.instr.mode == AddressingMode::accum) {
        assert(!value.has_value() && !addr.has_value());
    }
    switch (cpu.instr.type) {
    case InstructionType::lda:
        set_flags_ZN(cpu, *value);
        cpu.A = *value;
        break;
    case InstructionType::jmp:
        cpu.PC = *addr;
        break;
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
    default:
        assert(false);
    }
}

std::array<Instruction, 256> instructions{};
void initialize_instructions() {
    instructions[0xA9] = {InstructionType::lda, 0xA9, AddressingMode::immediate, true, false};
    instructions[0x4C] = {InstructionType::jmp, 0x4C, AddressingMode::absolute, true, false};
    instructions[0x6C] = {InstructionType::jmp, 0x6C, AddressingMode::indirect, true, false};
    instructions[0x6C] = {InstructionType::asl, 0x0E, AddressingMode::absolute, true, false};
}

inline auto addr_mode(CPU &cpu) -> AddrResult {
    if (cpu.instr_counter < 1) assert(false);
    switch (cpu.instr.mode) {
    case AddressingMode::immediate:
        if (cpu.instr_counter != 1) assert(false);
        if (cpu.instr.is_read) {
            return {AddrResultType::complete_value, .value = fetch(cpu)};
        } else {
            assert(false);
        }
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
    assert(cpu.instr.type == InstructionType::asl);
    switch (cpu.instr.mode) {
    case AddressingMode::accum:
        assert(false);
        break;
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
    default:
        assert(false);
    }
}

inline auto tick(CPU &cpu) -> void {
    cpu.addr_result.validate();
    if (cpu.addr_result.type == AddrResultType::load_instruction) {
        // Fetch instruction
        Byte opcode = fetch(cpu);
        cpu.instr = instructions[opcode];
        cpu.instr_counter = 1;
        cpu.addr_result = {AddrResultType::in_progress};
        ++cpu.cycles;
        return;
    }

    if (is_rmw_instruction(cpu.instr.type)) {
        cpu.addr_result = addr_mode_rmw(cpu);
    } else {
        cpu.addr_result = addr_mode(cpu);
    }
    cpu.addr_result.validate();
    ++cpu.instr_counter;
    if (cpu.addr_result.type != AddrResultType::in_progress) {
        exec_func(cpu, cpu.addr_result.value, cpu.addr_result.addr);
        cpu.addr_result = {AddrResultType::load_instruction};
        cpu.instr_counter = 0;
    }
    ++cpu.cycles;
}
} // namespace mos6502
