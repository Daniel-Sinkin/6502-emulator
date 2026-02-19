// 6502 emulator tests.
//
// Attribution:
// - Several branch and status-flow cases in this file are adapted from py65's
//   6502 MPU tests:
//   https://git.applefritter.com/6502/py65/compare/0.21..9401f738078bf4d18b1c4fa27f616a350248650b?show-outdated=&style=split&whitespace=show-all
// - Several opcode vectors are adapted from py65 assembler tests:
//   https://git.applefritter.com/6502/py65/src/commit/27367da0f811a6c3869676545a586cb639ec633b/src/py65/tests/test_assembler.py
// - py65 repository/license:
//   https://github.com/mnaberez/py65 (BSD-3-Clause)
// - Local copy of the required third-party notice:
//   tests/third_party/py65.LICENSE

#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../src/6502/6502.hpp"
#include "../src/6502/history.hpp"
#include "../src/6502/program_writer.hpp"

using Address = TYPES::Word;
using Byte = TYPES::Byte;

namespace {

struct TestFailure : std::runtime_error {
    explicit TestFailure(const std::string &msg)
        : std::runtime_error(msg) {}
};

template <typename A, typename B>
auto expect_eq_impl(const A &actual, const B &expected, const char *a_expr, const char *e_expr, const char *file, int line) -> void {
    if (!(actual == expected)) {
        std::ostringstream oss;
        oss << file << ":" << line << " EXPECT_EQ failed (" << a_expr << " vs " << e_expr << ")";
        throw TestFailure(oss.str());
    }
}

#define EXPECT_EQ(actual, expected) expect_eq_impl((actual), (expected), #actual, #expected, __FILE__, __LINE__)
#define EXPECT_TRUE(condition)                                                                    \
    do {                                                                                          \
        if (!(condition)) {                                                                       \
            std::ostringstream oss;                                                               \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_TRUE failed: " << #condition;         \
            throw TestFailure(oss.str());                                                         \
        }                                                                                         \
    } while (false)

using TestFunc = void (*)();
struct TestCase {
    std::string name;
    TestFunc fn;
};

auto tests() -> std::vector<TestCase> & {
    static std::vector<TestCase> all;
    return all;
}

struct TestRegistrar {
    TestRegistrar(const char *name, TestFunc fn) {
        tests().push_back({name, fn});
    }
};

#define TEST(name)             \
    auto name() -> void;       \
    TestRegistrar reg_##name(#name, &name); \
    auto name() -> void

auto make_cpu(Address pc = static_cast<Address>(0x0200)) -> mos6502::CPU {
    auto cpu = mos6502::CPU{};
    cpu.PC = pc;
    cpu.addr_result = {mos6502::AddrResultType::load_instruction};
    cpu.instr_counter = 0;
    return cpu;
}

auto write_bytes(mos6502::CPU &cpu, Address addr, std::initializer_list<Byte> bytes) -> void {
    Address it = addr;
    for (Byte b : bytes) {
        cpu.mem[it++] = b;
    }
}

auto execute_instruction(mos6502::CPU &cpu, int max_ticks = 32) -> void {
    const auto start_cycles = cpu.cycles;
    for (int i = 0; i < max_ticks; ++i) {
        mos6502::tick(cpu);
        if (cpu.addr_result.type == mos6502::AddrResultType::load_instruction &&
            cpu.instr_counter == 0 &&
            cpu.cycles > start_cycles) {
            return;
        }
    }
    throw TestFailure("instruction did not finish within max ticks");
}

auto execute_n(mos6502::CPU &cpu, int instruction_count) -> void {
    for (int i = 0; i < instruction_count; ++i) {
        execute_instruction(cpu);
    }
}

auto expect_core_equal(const mos6502::CPU &lhs, const mos6502::CPU &rhs) -> void {
    const auto l = mos6502::capture_core_state(lhs);
    const auto r = mos6502::capture_core_state(rhs);
    EXPECT_EQ(l.PC, r.PC);
    EXPECT_EQ(l.current_instruction_pc, r.current_instruction_pc);
    EXPECT_EQ(l.A, r.A);
    EXPECT_EQ(l.X, r.X);
    EXPECT_EQ(l.Y, r.Y);
    EXPECT_EQ(l.SP, r.SP);
    EXPECT_EQ(l.P, r.P);
    EXPECT_EQ(l.nmi, r.nmi);
    EXPECT_EQ(l.irq, r.irq);
    EXPECT_EQ(l.sync, r.sync);
    EXPECT_EQ(l.rdy, r.rdy);
    EXPECT_EQ(l.addr, r.addr);
    EXPECT_EQ(l.temporary_address_register, r.temporary_address_register);
    EXPECT_EQ(l.data_bus, r.data_bus);
    EXPECT_EQ(l.rw, r.rw);
    EXPECT_EQ(l.instr.type, r.instr.type);
    EXPECT_EQ(l.instr.mode, r.instr.mode);
    EXPECT_EQ(l.instr_counter, r.instr_counter);
    EXPECT_EQ(l.tmp, r.tmp);
    EXPECT_EQ(l.cycles, r.cycles);
    EXPECT_EQ(l.addr_result.type, r.addr_result.type);
    EXPECT_EQ(l.addr_result.value.has_value(), r.addr_result.value.has_value());
    EXPECT_EQ(l.addr_result.addr.has_value(), r.addr_result.addr.has_value());
    if (l.addr_result.value.has_value()) {
        EXPECT_EQ(*l.addr_result.value, *r.addr_result.value);
    }
    if (l.addr_result.addr.has_value()) {
        EXPECT_EQ(*l.addr_result.addr, *r.addr_result.addr);
    }
    EXPECT_EQ(l.config.preserve_indirect_jump_page_cross_bug,
        r.config.preserve_indirect_jump_page_cross_bug);
}

TEST(instruction_table_has_expected_shape) {
    std::size_t mapped = 0;
    for (const auto &instr : mos6502::instructions) {
        if (instr.type != mos6502::InstructionType::NONE) {
            ++mapped;
        }
    }
    EXPECT_EQ(mapped, static_cast<std::size_t>(151));
    EXPECT_EQ(mos6502::instructions.size() - mapped, static_cast<std::size_t>(105));
}

TEST(mapped_opcodes_execute_without_asserting) {
    for (std::size_t opcode = 0; opcode < mos6502::instructions.size(); ++opcode) {
        if (mos6502::instructions[opcode].type == mos6502::InstructionType::NONE) {
            continue;
        }

        auto cpu = make_cpu(static_cast<Address>(0x4000));
        cpu.mem[0x4000] = static_cast<Byte>(opcode);
        cpu.mem[0x4001] = static_cast<Byte>(0x00);
        cpu.mem[0x4002] = static_cast<Byte>(0x20);

        if (opcode == 0x00) { // BRK
            cpu.mem[0xFFFE] = static_cast<Byte>(0x34);
            cpu.mem[0xFFFF] = static_cast<Byte>(0x12);
        }
        if (opcode == 0x20) { // JSR
            cpu.mem[0x4001] = static_cast<Byte>(0x00);
            cpu.mem[0x4002] = static_cast<Byte>(0x50);
            cpu.mem[0x5000] = static_cast<Byte>(0xEA);
        }
        if (opcode == 0x40) { // RTI
            cpu.SP = static_cast<Byte>(0xFA);
            cpu.mem[0x01FB] = static_cast<Byte>(mos6502::U_FLAG);
            cpu.mem[0x01FC] = static_cast<Byte>(0x78);
            cpu.mem[0x01FD] = static_cast<Byte>(0x56);
        }
        if (opcode == 0x60) { // RTS
            cpu.SP = static_cast<Byte>(0xFB);
            cpu.mem[0x01FC] = static_cast<Byte>(0x34);
            cpu.mem[0x01FD] = static_cast<Byte>(0x12);
        }

        execute_instruction(cpu, 64);
    }
}

TEST(program_writer_vectors_adapted_from_py65_assembler_tests) {
    struct VectorCase {
        std::string name;
        std::function<void(mos6502::ProgramWriter &)> emit;
        std::vector<Byte> expected;
    };

    const std::vector<VectorCase> vectors = {
        {"BRK", [](auto &pw) { pw.brk(); }, {0x00}},
        {"ORA ($44,X)", [](auto &pw) { pw.ora_indirect_x(); pw(static_cast<Byte>(0x44)); }, {0x01, 0x44}},
        {"ORA $44", [](auto &pw) { pw.ora_zero_page(); pw(static_cast<Byte>(0x44)); }, {0x05, 0x44}},
        {"ASL $44", [](auto &pw) { pw.asl_zero_page(); pw(static_cast<Byte>(0x44)); }, {0x06, 0x44}},
        {"PHP", [](auto &pw) { pw.php(); }, {0x08}},
        {"ORA #$44", [](auto &pw) { pw.ora_immediate(); pw(static_cast<Byte>(0x44)); }, {0x09, 0x44}},
        {"ASL A", [](auto &pw) { pw.asl_accumulator(); }, {0x0A}},
        {"BIT $4400", [](auto &pw) { pw.bit_absolute(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0x2C, 0x00, 0x44}},
        {"AND $4400", [](auto &pw) { pw.and_absolute(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0x2D, 0x00, 0x44}},
        {"ROL $4400", [](auto &pw) { pw.rol_absolute(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0x2E, 0x00, 0x44}},
        {"BMI rel", [](auto &pw) { pw.bmi(); pw(static_cast<Byte>(0x44)); }, {0x30, 0x44}},
        {"AND ($44),Y", [](auto &pw) { pw.and_indirect_y(); pw(static_cast<Byte>(0x44)); }, {0x31, 0x44}},
        {"AND $44,X", [](auto &pw) { pw.and_zeropage_x(); pw(static_cast<Byte>(0x44)); }, {0x35, 0x44}},
        {"ROL $44,X", [](auto &pw) { pw.rol_zero_page_x(); pw(static_cast<Byte>(0x44)); }, {0x36, 0x44}},
        {"SEC", [](auto &pw) { pw.sec(); }, {0x38}},
        {"AND $4400,Y", [](auto &pw) { pw.and_absolute_y(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0x39, 0x00, 0x44}},
        {"AND $4400,X", [](auto &pw) { pw.and_absolute_x(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0x3D, 0x00, 0x44}},
        {"ROL $4400,X", [](auto &pw) { pw.rol_absolute_x(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0x3E, 0x00, 0x44}},
        {"RTI", [](auto &pw) { pw.rti(); }, {0x40}},
        {"EOR ($44,X)", [](auto &pw) { pw.eor_indirect_x(); pw(static_cast<Byte>(0x44)); }, {0x41, 0x44}},
        {"EOR $44", [](auto &pw) { pw.eor_zero_page(); pw(static_cast<Byte>(0x44)); }, {0x45, 0x44}},
        {"LSR $44", [](auto &pw) { pw.lsr_zero_page(); pw(static_cast<Byte>(0x44)); }, {0x46, 0x44}},
        {"PHA", [](auto &pw) { pw.pha(); }, {0x48}},
        {"EOR #$44", [](auto &pw) { pw.eor_immediate(); pw(static_cast<Byte>(0x44)); }, {0x49, 0x44}},
        {"LSR A", [](auto &pw) { pw.lsr_accumulator(); }, {0x4A}},
        {"JMP $5597", [](auto &pw) { pw.jmp_absolute(); pw(static_cast<Byte>(0x97)); pw(static_cast<Byte>(0x55)); }, {0x4C, 0x97, 0x55}},
        {"LSR $4400,X", [](auto &pw) { pw.lsr_absolute_x(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0x5E, 0x00, 0x44}},
        {"RTS", [](auto &pw) { pw.rts(); }, {0x60}},
        {"ADC ($44,X)", [](auto &pw) { pw.adc_indirect_x(); pw(static_cast<Byte>(0x44)); }, {0x61, 0x44}},
        {"ADC $44", [](auto &pw) { pw.adc_zero_page(); pw(static_cast<Byte>(0x44)); }, {0x65, 0x44}},
        {"ROR $44", [](auto &pw) { pw.ror_zero_page(); pw(static_cast<Byte>(0x44)); }, {0x66, 0x44}},
        {"PLA", [](auto &pw) { pw.pla(); }, {0x68}},
        {"ADC #$44", [](auto &pw) { pw.adc_immediate(); pw(static_cast<Byte>(0x44)); }, {0x69, 0x44}},
        {"ROR A", [](auto &pw) { pw.ror_accumulator(); }, {0x6A}},
        {"JMP ($5597)", [](auto &pw) { pw.jmp_indirect(); pw(static_cast<Byte>(0x97)); pw(static_cast<Byte>(0x55)); }, {0x6C, 0x97, 0x55}},
        {"ADC $4400", [](auto &pw) { pw.adc_absolute(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0x6D, 0x00, 0x44}},
        {"STA ($44,X)", [](auto &pw) { pw.sta_indirect_x(); pw(static_cast<Byte>(0x44)); }, {0x81, 0x44}},
        {"STY $44", [](auto &pw) { pw.sty_zero_page(); pw(static_cast<Byte>(0x44)); }, {0x84, 0x44}},
        {"STA $44", [](auto &pw) { pw.sta_zero_page(); pw(static_cast<Byte>(0x44)); }, {0x85, 0x44}},
        {"STX $44", [](auto &pw) { pw.stx_zero_page(); pw(static_cast<Byte>(0x44)); }, {0x86, 0x44}},
        {"DEY", [](auto &pw) { pw.dey(); }, {0x88}},
        {"TXA", [](auto &pw) { pw.txa(); }, {0x8A}},
        {"STY $4400", [](auto &pw) { pw.sty_absolute(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0x8C, 0x00, 0x44}},
        {"STA $4400", [](auto &pw) { pw.sta_absolute(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0x8D, 0x00, 0x44}},
        {"STX $4400", [](auto &pw) { pw.stx_absolute(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0x8E, 0x00, 0x44}},
        {"BCC rel", [](auto &pw) { pw.bcc(); pw(static_cast<Byte>(0x44)); }, {0x90, 0x44}},
        {"CMP ($44),Y", [](auto &pw) { pw.cmp_indirect_y(); pw(static_cast<Byte>(0x44)); }, {0xD1, 0x44}},
        {"CMP $44,X", [](auto &pw) { pw.cmp_zero_page_x(); pw(static_cast<Byte>(0x44)); }, {0xD5, 0x44}},
        {"DEC $44,X", [](auto &pw) { pw.dec_zero_page_x(); pw(static_cast<Byte>(0x44)); }, {0xD6, 0x44}},
        {"CLD", [](auto &pw) { pw.cld(); }, {0xD8}},
        {"CMP $4400,X", [](auto &pw) { pw.cmp_absolute_x(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0xDD, 0x00, 0x44}},
        {"DEC $4400,X", [](auto &pw) { pw.dec_absolute_x(); pw(static_cast<Byte>(0x00)); pw(static_cast<Byte>(0x44)); }, {0xDE, 0x00, 0x44}},
        {"CPX #$44", [](auto &pw) { pw.cpx_immediate(); pw(static_cast<Byte>(0x44)); }, {0xE0, 0x44}},
        {"SBC ($44,X)", [](auto &pw) { pw.sbc_indirect_x(); pw(static_cast<Byte>(0x44)); }, {0xE1, 0x44}},
        {"CPX $44", [](auto &pw) { pw.cpx_zero_page(); pw(static_cast<Byte>(0x44)); }, {0xE4, 0x44}},
    };

    for (const auto &tc : vectors) {
        auto cpu = make_cpu();
        auto pw = mos6502::ProgramWriter(cpu, static_cast<Address>(0x0200));
        tc.emit(pw);
        for (std::size_t i = 0; i < tc.expected.size(); ++i) {
            EXPECT_EQ(cpu.mem[static_cast<Address>(0x0200 + i)], tc.expected[i]);
        }
    }
}

TEST(load_store_transfer_flow) {
    auto cpu = make_cpu(static_cast<Address>(0x0600));
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xA9), static_cast<Byte>(0x2A), // LDA #$2A
        static_cast<Byte>(0xAA),                         // TAX
        static_cast<Byte>(0xA8),                         // TAY
        static_cast<Byte>(0x8D), static_cast<Byte>(0x00), static_cast<Byte>(0x02), // STA $0200
        static_cast<Byte>(0x8E), static_cast<Byte>(0x01), static_cast<Byte>(0x02), // STX $0201
        static_cast<Byte>(0x8C), static_cast<Byte>(0x02), static_cast<Byte>(0x02)  // STY $0202
    });

    execute_n(cpu, 6);

    EXPECT_EQ(cpu.A, static_cast<Byte>(0x2A));
    EXPECT_EQ(cpu.X, static_cast<Byte>(0x2A));
    EXPECT_EQ(cpu.Y, static_cast<Byte>(0x2A));
    EXPECT_EQ(cpu.mem[0x0200], static_cast<Byte>(0x2A));
    EXPECT_EQ(cpu.mem[0x0201], static_cast<Byte>(0x2A));
    EXPECT_EQ(cpu.mem[0x0202], static_cast<Byte>(0x2A));
}

TEST(adc_overflow_sets_v_and_n) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x50);
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x69), static_cast<Byte>(0x50) // ADC #$50
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0xA0));
    EXPECT_TRUE((cpu.P & mos6502::V_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::N_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) == 0u);
}

TEST(adc_sets_carry_and_zero) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0xFF);
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x69), static_cast<Byte>(0x01) // ADC #$01
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x00));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::Z_FLAG) != 0u);
}

TEST(sbc_sets_borrow_and_negative) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x00);
    cpu.P |= mos6502::C_FLAG;
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xE9), static_cast<Byte>(0x01) // SBC #$01
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0xFF));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) == 0u);
    EXPECT_TRUE((cpu.P & mos6502::N_FLAG) != 0u);
}

TEST(adc_decimal_mode_adds_bcd_digits) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x15);
    cpu.P = mos6502::D_FLAG;
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x69), static_cast<Byte>(0x27) // ADC #$27
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x42));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) == 0u);
    EXPECT_TRUE((cpu.P & mos6502::Z_FLAG) == 0u);
}

TEST(adc_decimal_mode_carry_and_overflow_behavior) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x50);
    cpu.P = mos6502::D_FLAG;
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x69), static_cast<Byte>(0x50) // ADC #$50
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x00));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::Z_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::V_FLAG) != 0u);
}

TEST(adc_decimal_mode_uses_input_carry) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x09);
    cpu.P = static_cast<Byte>(mos6502::D_FLAG | mos6502::C_FLAG);
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x69), static_cast<Byte>(0x00) // ADC #$00
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x10));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) == 0u);
}

TEST(adc_decimal_mode_vectors) {
    struct Case {
        Byte a;
        Byte operand;
        bool carry_in;
        Byte expected_a;
        bool expected_carry;
    };

    const std::vector<Case> cases = {
        {static_cast<Byte>(0x00), static_cast<Byte>(0x00), false, static_cast<Byte>(0x00), false},
        {static_cast<Byte>(0x09), static_cast<Byte>(0x01), false, static_cast<Byte>(0x10), false},
        {static_cast<Byte>(0x45), static_cast<Byte>(0x55), false, static_cast<Byte>(0x00), true},
        {static_cast<Byte>(0x99), static_cast<Byte>(0x00), true, static_cast<Byte>(0x00), true},
        {static_cast<Byte>(0x12), static_cast<Byte>(0x34), false, static_cast<Byte>(0x46), false},
        {static_cast<Byte>(0x38), static_cast<Byte>(0x62), false, static_cast<Byte>(0x00), true},
    };

    for (const auto &tc : cases) {
        auto cpu = make_cpu();
        cpu.A = tc.a;
        cpu.P = mos6502::D_FLAG;
        if (tc.carry_in) {
            cpu.P |= mos6502::C_FLAG;
        }
        write_bytes(cpu, cpu.PC, {
            static_cast<Byte>(0x69), tc.operand // ADC #imm
        });
        execute_instruction(cpu);

        EXPECT_EQ(cpu.A, tc.expected_a);
        EXPECT_EQ((cpu.P & mos6502::C_FLAG) != 0u, tc.expected_carry);
    }
}

TEST(sbc_decimal_mode_subtracts_bcd_digits) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x50);
    cpu.P = static_cast<Byte>(mos6502::D_FLAG | mos6502::C_FLAG);
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xE9), static_cast<Byte>(0x01) // SBC #$01
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x49));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) != 0u);
}

TEST(sbc_decimal_mode_borrow_sets_carry_clear) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x00);
    cpu.P = static_cast<Byte>(mos6502::D_FLAG | mos6502::C_FLAG);
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xE9), static_cast<Byte>(0x01) // SBC #$01
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x99));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) == 0u);
}

TEST(sbc_decimal_mode_uses_input_borrow) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x10);
    cpu.P = mos6502::D_FLAG; // carry clear => borrow in
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xE9), static_cast<Byte>(0x01) // SBC #$01
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x08));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) != 0u);
}

TEST(sbc_decimal_mode_overflow_uses_binary_rule) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x80);
    cpu.P = static_cast<Byte>(mos6502::D_FLAG | mos6502::C_FLAG);
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xE9), static_cast<Byte>(0x01) // SBC #$01
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x79));
    EXPECT_TRUE((cpu.P & mos6502::V_FLAG) != 0u);
}

TEST(sbc_decimal_mode_vectors) {
    struct Case {
        Byte a;
        Byte operand;
        bool carry_in;
        Byte expected_a;
        bool expected_carry;
    };

    const std::vector<Case> cases = {
        {static_cast<Byte>(0x50), static_cast<Byte>(0x49), true, static_cast<Byte>(0x01), true},
        {static_cast<Byte>(0x10), static_cast<Byte>(0x01), true, static_cast<Byte>(0x09), true},
        {static_cast<Byte>(0x00), static_cast<Byte>(0x01), true, static_cast<Byte>(0x99), false},
        {static_cast<Byte>(0x00), static_cast<Byte>(0x00), false, static_cast<Byte>(0x99), false},
        {static_cast<Byte>(0x42), static_cast<Byte>(0x42), true, static_cast<Byte>(0x00), true},
    };

    for (const auto &tc : cases) {
        auto cpu = make_cpu();
        cpu.A = tc.a;
        cpu.P = mos6502::D_FLAG;
        if (tc.carry_in) {
            cpu.P |= mos6502::C_FLAG;
        }
        write_bytes(cpu, cpu.PC, {
            static_cast<Byte>(0xE9), tc.operand // SBC #imm
        });
        execute_instruction(cpu);

        EXPECT_EQ(cpu.A, tc.expected_a);
        EXPECT_EQ((cpu.P & mos6502::C_FLAG) != 0u, tc.expected_carry);
    }
}

TEST(cld_disables_decimal_math_for_adc) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x50);
    cpu.P = 0;
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xF8),                         // SED
        static_cast<Byte>(0x18),                         // CLC
        static_cast<Byte>(0x69), static_cast<Byte>(0x50), // ADC #$50 => decimal result $00, carry set
        static_cast<Byte>(0xA9), static_cast<Byte>(0x50), // LDA #$50
        static_cast<Byte>(0xD8),                         // CLD
        static_cast<Byte>(0x18),                         // CLC
        static_cast<Byte>(0x69), static_cast<Byte>(0x50)  // ADC #$50 => binary result $A0
    });

    execute_n(cpu, 7);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0xA0));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) == 0u);
}

TEST(irq_taken_when_enabled_pushes_state_and_uses_vector) {
    auto cpu = make_cpu(static_cast<Address>(0x3456));
    cpu.P = 0;
    cpu.irq = true;
    cpu.mem[0xFFFE] = static_cast<Byte>(0xCD);
    cpu.mem[0xFFFF] = static_cast<Byte>(0xAB);

    mos6502::tick(cpu);

    EXPECT_EQ(cpu.PC, static_cast<Address>(0xABCD));
    EXPECT_EQ(cpu.SP, static_cast<Byte>(0xFA));
    EXPECT_EQ(cpu.mem[0x01FD], static_cast<Byte>(0x34));
    EXPECT_EQ(cpu.mem[0x01FC], static_cast<Byte>(0x56));
    EXPECT_TRUE((cpu.mem[0x01FB] & mos6502::B_FLAG) == 0u);
    EXPECT_TRUE((cpu.mem[0x01FB] & mos6502::U_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::I_FLAG) != 0u);
    EXPECT_EQ(cpu.addr_result.type, mos6502::AddrResultType::load_instruction);
    EXPECT_EQ(cpu.instr_counter, 0);
}

TEST(irq_is_ignored_when_interrupt_disable_flag_set) {
    auto cpu = make_cpu(static_cast<Address>(0x2200));
    cpu.P = mos6502::I_FLAG;
    cpu.irq = true;
    cpu.mem[cpu.PC] = static_cast<Byte>(0xEA); // NOP

    mos6502::tick(cpu);

    EXPECT_EQ(cpu.PC, static_cast<Address>(0x2201));
    EXPECT_EQ(cpu.instr.type, mos6502::InstructionType::nop);
    EXPECT_EQ(cpu.addr_result.type, mos6502::AddrResultType::in_progress);
}

TEST(nmi_is_taken_even_when_interrupt_disable_flag_set) {
    auto cpu = make_cpu(static_cast<Address>(0x3210));
    cpu.P = mos6502::I_FLAG;
    cpu.nmi = true;
    cpu.mem[0xFFFA] = static_cast<Byte>(0xAA);
    cpu.mem[0xFFFB] = static_cast<Byte>(0x55);

    mos6502::tick(cpu);

    EXPECT_EQ(cpu.PC, static_cast<Address>(0x55AA));
    EXPECT_EQ(cpu.SP, static_cast<Byte>(0xFA));
    EXPECT_TRUE((cpu.P & mos6502::I_FLAG) != 0u);
}

TEST(nmi_has_priority_over_irq_when_both_pending) {
    auto cpu = make_cpu(static_cast<Address>(0x4000));
    cpu.nmi = true;
    cpu.irq = true;
    cpu.mem[0xFFFA] = static_cast<Byte>(0x11);
    cpu.mem[0xFFFB] = static_cast<Byte>(0x22);
    cpu.mem[0xFFFE] = static_cast<Byte>(0x33);
    cpu.mem[0xFFFF] = static_cast<Byte>(0x44);

    mos6502::tick(cpu);

    EXPECT_EQ(cpu.PC, static_cast<Address>(0x2211));
    EXPECT_TRUE(!cpu.nmi);
    EXPECT_TRUE(cpu.irq);
}

TEST(nmi_line_is_cleared_after_servicing) {
    auto cpu = make_cpu();
    cpu.nmi = true;
    cpu.mem[0xFFFA] = static_cast<Byte>(0x00);
    cpu.mem[0xFFFB] = static_cast<Byte>(0x20);

    mos6502::tick(cpu);

    EXPECT_TRUE(!cpu.nmi);
}

TEST(interrupt_preempts_next_opcode_fetch_at_instruction_boundary) {
    auto cpu = make_cpu(static_cast<Address>(0x4000));
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xEA), // NOP
        static_cast<Byte>(0xEA)  // NOP (should not fetch yet when IRQ arrives)
    });
    cpu.mem[0xFFFE] = static_cast<Byte>(0x00);
    cpu.mem[0xFFFF] = static_cast<Byte>(0x30);

    execute_instruction(cpu); // execute first NOP -> PC now points at second NOP
    EXPECT_EQ(cpu.PC, static_cast<Address>(0x4001));

    cpu.irq = true;
    mos6502::tick(cpu);

    EXPECT_EQ(cpu.PC, static_cast<Address>(0x3000));
    EXPECT_EQ(cpu.mem[0x01FD], static_cast<Byte>(0x40));
    EXPECT_EQ(cpu.mem[0x01FC], static_cast<Byte>(0x01));
}

TEST(rti_round_trip_after_irq_restores_pc_and_status) {
    auto cpu = make_cpu(static_cast<Address>(0x2000));
    cpu.P = static_cast<Byte>(mos6502::C_FLAG | mos6502::D_FLAG);
    cpu.irq = true;
    cpu.mem[0xFFFE] = static_cast<Byte>(0x00);
    cpu.mem[0xFFFF] = static_cast<Byte>(0x30);
    cpu.mem[0x3000] = static_cast<Byte>(0x40); // RTI

    mos6502::tick(cpu); // service IRQ
    EXPECT_EQ(cpu.PC, static_cast<Address>(0x3000));

    cpu.irq = false;
    execute_instruction(cpu); // RTI

    EXPECT_EQ(cpu.PC, static_cast<Address>(0x2000));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::D_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::I_FLAG) == 0u);
    EXPECT_TRUE((cpu.P & mos6502::B_FLAG) == 0u);
    EXPECT_TRUE((cpu.P & mos6502::U_FLAG) != 0u);
}

TEST(rdy_low_stalls_cpu_fetch_and_execution) {
    auto cpu = make_cpu(static_cast<Address>(0x1234));
    cpu.mem[cpu.PC] = static_cast<Byte>(0xEA); // NOP
    cpu.rdy = false;

    mos6502::tick(cpu);

    EXPECT_EQ(cpu.PC, static_cast<Address>(0x1234));
    EXPECT_EQ(cpu.addr_result.type, mos6502::AddrResultType::load_instruction);
    EXPECT_EQ(cpu.instr_counter, 0);
    EXPECT_TRUE(!cpu.sync);
}

TEST(sync_is_true_on_opcode_fetch_and_false_otherwise) {
    auto cpu = make_cpu();
    cpu.mem[cpu.PC] = static_cast<Byte>(0xEA); // NOP

    mos6502::tick(cpu); // fetch
    EXPECT_TRUE(cpu.sync);

    mos6502::tick(cpu); // execute
    EXPECT_TRUE(!cpu.sync);
    EXPECT_EQ(cpu.addr_result.type, mos6502::AddrResultType::load_instruction);
}

TEST(current_instruction_pc_tracks_opcode_start) {
    auto cpu = make_cpu(static_cast<Address>(0x5000));
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x9D), static_cast<Byte>(0x00), static_cast<Byte>(0x02), // STA $0200,X
        static_cast<Byte>(0xEA) // NOP
    });

    mos6502::tick(cpu); // fetch STA
    EXPECT_EQ(cpu.current_instruction_pc, static_cast<Address>(0x5000));
    EXPECT_EQ(cpu.PC, static_cast<Address>(0x5001));

    mos6502::tick(cpu); // finish STA
    EXPECT_EQ(cpu.current_instruction_pc, static_cast<Address>(0x5000));

    mos6502::tick(cpu); // fetch NOP
    EXPECT_EQ(cpu.current_instruction_pc, static_cast<Address>(0x5003));
}

TEST(irq_level_line_can_retrigger_after_cli) {
    auto cpu = make_cpu(static_cast<Address>(0x1000));
    cpu.irq = true;
    cpu.mem[0xFFFE] = static_cast<Byte>(0x00);
    cpu.mem[0xFFFF] = static_cast<Byte>(0x20);
    write_bytes(cpu, static_cast<Address>(0x2000), {
        static_cast<Byte>(0x58), // CLI
        static_cast<Byte>(0xEA)  // NOP
    });

    mos6502::tick(cpu); // first IRQ service
    EXPECT_EQ(cpu.PC, static_cast<Address>(0x2000));
    EXPECT_TRUE((cpu.P & mos6502::I_FLAG) != 0u);

    execute_instruction(cpu); // CLI -> clear I
    EXPECT_TRUE((cpu.P & mos6502::I_FLAG) == 0u);

    mos6502::tick(cpu); // IRQ still asserted, should service again
    EXPECT_EQ(cpu.PC, static_cast<Address>(0x2000));
    EXPECT_TRUE((cpu.P & mos6502::I_FLAG) != 0u);
}

TEST(bit_updates_z_n_v_flags) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x40);
    cpu.mem[0x0040] = static_cast<Byte>(0xC0);
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x24), static_cast<Byte>(0x40) // BIT $40
    });
    execute_instruction(cpu);
    EXPECT_TRUE((cpu.P & mos6502::Z_FLAG) == 0u);
    EXPECT_TRUE((cpu.P & mos6502::N_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::V_FLAG) != 0u);
}

TEST(bne_zero_set_does_not_branch_adapted_from_py65) {
    auto cpu = make_cpu(static_cast<Address>(0x0050));
    cpu.P |= mos6502::Z_FLAG;
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xD0), static_cast<Byte>(0x06) // BNE +6
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.PC, static_cast<Address>(0x0052));
}

TEST(bne_zero_clear_branches_forward_adapted_from_py65) {
    auto cpu = make_cpu(static_cast<Address>(0x0050));
    cpu.P &= static_cast<Byte>(~mos6502::Z_FLAG);
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xD0), static_cast<Byte>(0x06) // BNE +6
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.PC, static_cast<Address>(0x0058));
}

TEST(bcs_carry_set_branches_backward_adapted_from_py65) {
    auto cpu = make_cpu(static_cast<Address>(0x0050));
    cpu.P |= mos6502::C_FLAG;
    Byte rel = static_cast<Byte>((0x06u ^ 0xFFu) + 1u);
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xB0), rel // BCS -6
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.PC, static_cast<Address>(0x0052 - 0x0006));
}

TEST(bpl_negative_set_does_not_branch_adapted_from_py65) {
    auto cpu = make_cpu(static_cast<Address>(0x0050));
    cpu.P |= mos6502::N_FLAG;
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x10), static_cast<Byte>(0xFA) // BPL -6
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.PC, static_cast<Address>(0x0052));
}

TEST(jsr_and_rts_round_trip) {
    auto cpu = make_cpu(static_cast<Address>(0x0300));
    write_bytes(cpu, static_cast<Address>(0x0300), {
        static_cast<Byte>(0x20), static_cast<Byte>(0x10), static_cast<Byte>(0x03), // JSR $0310
        static_cast<Byte>(0xEA)                                                      // NOP
    });
    write_bytes(cpu, static_cast<Address>(0x0310), {
        static_cast<Byte>(0xA9), static_cast<Byte>(0x77), // LDA #$77
        static_cast<Byte>(0x60)                           // RTS
    });

    execute_n(cpu, 3);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x77));
    EXPECT_EQ(cpu.PC, static_cast<Address>(0x0303));
}

TEST(pha_pla_round_trip_preserves_a) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x44);
    const auto starting_sp = cpu.SP;
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x48),                         // PHA
        static_cast<Byte>(0xA9), static_cast<Byte>(0x00), // LDA #$00
        static_cast<Byte>(0x68)                          // PLA
    });
    execute_n(cpu, 3);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x44));
    EXPECT_EQ(cpu.SP, starting_sp);
}

TEST(php_clc_plp_restores_carry) {
    auto cpu = make_cpu();
    cpu.P |= mos6502::C_FLAG;
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x08), // PHP
        static_cast<Byte>(0x18), // CLC
        static_cast<Byte>(0x28)  // PLP
    });
    execute_n(cpu, 3);
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::B_FLAG) == 0u);
    EXPECT_TRUE((cpu.P & mos6502::U_FLAG) != 0u);
}

TEST(brk_pushes_state_and_sets_vector) {
    auto cpu = make_cpu(static_cast<Address>(0x4000));
    cpu.P = 0;
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x00) // BRK
    });
    cpu.mem[0xFFFE] = static_cast<Byte>(0x34);
    cpu.mem[0xFFFF] = static_cast<Byte>(0x12);

    execute_instruction(cpu);

    EXPECT_EQ(cpu.PC, static_cast<Address>(0x1234));
    EXPECT_EQ(cpu.SP, static_cast<Byte>(0xFA));
    EXPECT_EQ(cpu.mem[0x01FD], static_cast<Byte>(0x40));
    EXPECT_EQ(cpu.mem[0x01FC], static_cast<Byte>(0x02));
    EXPECT_EQ(cpu.mem[0x01FB], static_cast<Byte>(mos6502::B_FLAG | mos6502::U_FLAG));
    EXPECT_TRUE((cpu.P & mos6502::I_FLAG) != 0u);
}

TEST(rti_restores_pc_and_status) {
    auto cpu = make_cpu();
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x40) // RTI
    });

    cpu.SP = static_cast<Byte>(0xFA);
    cpu.mem[0x01FB] = static_cast<Byte>(mos6502::C_FLAG | mos6502::N_FLAG | mos6502::B_FLAG);
    cpu.mem[0x01FC] = static_cast<Byte>(0x78);
    cpu.mem[0x01FD] = static_cast<Byte>(0x56);

    execute_instruction(cpu);

    EXPECT_EQ(cpu.PC, static_cast<Address>(0x5678));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::N_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::B_FLAG) == 0u);
    EXPECT_TRUE((cpu.P & mos6502::U_FLAG) != 0u);
}

TEST(jmp_indirect_preserves_page_wrap_bug_by_default) {
    auto cpu = make_cpu(static_cast<Address>(0x2000));
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x6C), static_cast<Byte>(0xFF), static_cast<Byte>(0x12) // JMP ($12FF)
    });
    cpu.mem[0x12FF] = static_cast<Byte>(0x78);
    cpu.mem[0x1200] = static_cast<Byte>(0x56);
    cpu.mem[0x1300] = static_cast<Byte>(0x9A);
    execute_instruction(cpu);
    EXPECT_EQ(cpu.PC, static_cast<Address>(0x5678));
}

TEST(jmp_indirect_can_disable_page_wrap_bug) {
    auto cpu = make_cpu(static_cast<Address>(0x2000));
    cpu.config.preserve_indirect_jump_page_cross_bug = false;
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x6C), static_cast<Byte>(0xFF), static_cast<Byte>(0x12) // JMP ($12FF)
    });
    cpu.mem[0x12FF] = static_cast<Byte>(0x78);
    cpu.mem[0x1300] = static_cast<Byte>(0x9A);
    execute_instruction(cpu);
    EXPECT_EQ(cpu.PC, static_cast<Address>(0x9A78));
}

TEST(zero_page_x_wraps_address) {
    auto cpu = make_cpu();
    cpu.X = static_cast<Byte>(0x10);
    cpu.mem[0x0005] = static_cast<Byte>(0xAB);
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xB5), static_cast<Byte>(0xF5) // LDA $F5,X -> $05
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0xAB));
}

TEST(indirect_y_reads_from_base_plus_y) {
    auto cpu = make_cpu();
    cpu.Y = static_cast<Byte>(0x03);
    cpu.mem[0x0020] = static_cast<Byte>(0x00);
    cpu.mem[0x0021] = static_cast<Byte>(0x20);
    cpu.mem[0x2003] = static_cast<Byte>(0x99);
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xB1), static_cast<Byte>(0x20) // LDA ($20),Y
    });
    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x99));
}

TEST(cmp_sets_carry_zero_and_negative) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x10);
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xC9), static_cast<Byte>(0x10), // CMP #$10
        static_cast<Byte>(0xC9), static_cast<Byte>(0x20)  // CMP #$20
    });

    execute_instruction(cpu);
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) != 0u);
    EXPECT_TRUE((cpu.P & mos6502::Z_FLAG) != 0u);

    execute_instruction(cpu);
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) == 0u);
    EXPECT_TRUE((cpu.P & mos6502::N_FLAG) != 0u);
}

TEST(step_history_undo_redo_restores_core_and_memory) {
    auto cpu = make_cpu(static_cast<Address>(0x0800));
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0xA9), static_cast<Byte>(0x11), // LDA #$11
        static_cast<Byte>(0x8D), static_cast<Byte>(0x00), static_cast<Byte>(0x02), // STA $0200
        static_cast<Byte>(0xE8), // INX
        static_cast<Byte>(0xE8)  // INX
    });

    mos6502::StepHistory history(240);
    const auto initial_cpu = cpu;

    constexpr int n_steps = 12;
    for (int i = 0; i < n_steps; ++i) {
        const auto before_state = mos6502::capture_core_state(cpu);
        const auto before_mem = cpu.mem;
        mos6502::tick(cpu);
        history.record_step(before_state, before_mem, cpu);
    }
    const auto after_cpu = cpu;
    EXPECT_EQ(history.size(), static_cast<std::size_t>(n_steps));
    EXPECT_EQ(history.cursor(), static_cast<std::size_t>(n_steps));

    for (int i = 0; i < n_steps; ++i) {
        EXPECT_TRUE(history.step_back(cpu));
    }
    EXPECT_TRUE(!history.can_step_back());
    expect_core_equal(cpu, initial_cpu);
    EXPECT_TRUE(std::equal(cpu.mem.begin(), cpu.mem.end(), initial_cpu.mem.begin()));

    for (int i = 0; i < n_steps; ++i) {
        EXPECT_TRUE(history.step_forward(cpu));
    }
    EXPECT_TRUE(!history.can_step_forward());
    expect_core_equal(cpu, after_cpu);
    EXPECT_TRUE(std::equal(cpu.mem.begin(), cpu.mem.end(), after_cpu.mem.begin()));
}

TEST(step_history_block_delta_tracks_block_granularity) {
    auto cpu = make_cpu(static_cast<Address>(0x0100));
    mos6502::StepHistory history(8);

    const auto before_state = mos6502::capture_core_state(cpu);
    const auto before_mem = cpu.mem;

    cpu.mem[0x0100] = static_cast<Byte>(0xAA);
    cpu.mem[0x01F0] = static_cast<Byte>(0xBB);
    cpu.PC = static_cast<Address>(cpu.PC + 1);

    history.record_step(before_state, before_mem, cpu);

    EXPECT_EQ(history.size(), static_cast<std::size_t>(1));
    EXPECT_EQ(history.total_changed_blocks(), static_cast<std::size_t>(1));
}

TEST(rol_and_ror_use_carry_in_out) {
    auto cpu = make_cpu();
    cpu.A = static_cast<Byte>(0x80);
    cpu.P |= mos6502::C_FLAG;
    write_bytes(cpu, cpu.PC, {
        static_cast<Byte>(0x2A), // ROL A
        static_cast<Byte>(0x6A)  // ROR A
    });

    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x01));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) != 0u);

    execute_instruction(cpu);
    EXPECT_EQ(cpu.A, static_cast<Byte>(0x80));
    EXPECT_TRUE((cpu.P & mos6502::C_FLAG) != 0u);
}

} // namespace

auto main() -> int {
    mos6502::initialize_instructions();

    int failures = 0;
    for (const auto &tc : tests()) {
        try {
            tc.fn();
            std::cout << "[PASS] " << tc.name << "\n";
        } catch (const std::exception &ex) {
            ++failures;
            std::cout << "[FAIL] " << tc.name << " -> " << ex.what() << "\n";
        } catch (...) {
            ++failures;
            std::cout << "[FAIL] " << tc.name << " -> unknown exception\n";
        }
    }

    std::cout << "\nExecuted " << tests().size() << " tests, failures: " << failures << "\n";
    return (failures == 0) ? 0 : 1;
}
