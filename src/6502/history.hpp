/* danielsinkin97@gmail.com */
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>

#include "6502.hpp"

namespace mos6502 {

inline constexpr std::size_t history_default_capacity = 240;
inline constexpr std::size_t history_block_size = 256; // 0.25 KB blocks

struct CPUCoreState {
    Address PC = 0x0000;
    Address current_instruction_pc = 0x0000;
    Byte A = 0x00;
    Byte X = 0x00;
    Byte Y = 0x00;
    Byte SP = 0x00;
    Byte P = 0x00;

    bool nmi = false;
    bool irq = false;

    bool sync = false;
    bool rdy = true;

    Address addr = 0x0000;
    Address temporary_address_register = 0x0000;
    Byte data_bus = 0x00;
    bool rw = true;

    Instruction instr{};
    int instr_counter = 0;

    Byte tmp = 0x00;
    uint64_t cycles = 0;

    AddrResult addr_result;
    Config config;
};

[[nodiscard]] inline auto capture_core_state(const CPU &cpu) -> CPUCoreState {
    return {
        .PC = cpu.PC,
        .current_instruction_pc = cpu.current_instruction_pc,
        .A = cpu.A,
        .X = cpu.X,
        .Y = cpu.Y,
        .SP = cpu.SP,
        .P = cpu.P,
        .nmi = cpu.nmi,
        .irq = cpu.irq,
        .sync = cpu.sync,
        .rdy = cpu.rdy,
        .addr = cpu.addr,
        .temporary_address_register = cpu.temporary_address_register,
        .data_bus = cpu.data_bus,
        .rw = cpu.rw,
        .instr = cpu.instr,
        .instr_counter = cpu.instr_counter,
        .tmp = cpu.tmp,
        .cycles = cpu.cycles,
        .addr_result = cpu.addr_result,
        .config = cpu.config,
    };
}

inline auto restore_core_state(CPU &cpu, const CPUCoreState &state) -> void {
    cpu.PC = state.PC;
    cpu.current_instruction_pc = state.current_instruction_pc;
    cpu.A = state.A;
    cpu.X = state.X;
    cpu.Y = state.Y;
    cpu.SP = state.SP;
    cpu.P = state.P;
    cpu.nmi = state.nmi;
    cpu.irq = state.irq;
    cpu.sync = state.sync;
    cpu.rdy = state.rdy;
    cpu.addr = state.addr;
    cpu.temporary_address_register = state.temporary_address_register;
    cpu.data_bus = state.data_bus;
    cpu.rw = state.rw;
    cpu.instr = state.instr;
    cpu.instr_counter = state.instr_counter;
    cpu.tmp = state.tmp;
    cpu.cycles = state.cycles;
    cpu.addr_result = state.addr_result;
    cpu.config = state.config;
}

struct ChangedMemoryBlock {
    Address base = 0x0000;
    std::array<Byte, history_block_size> before{};
    std::array<Byte, history_block_size> after{};
};

struct StepDeltaEntry {
    CPUCoreState before_state{};
    CPUCoreState after_state{};
    std::vector<ChangedMemoryBlock> blocks{};
};

class StepHistory {
public:
    explicit StepHistory(std::size_t capacity = history_default_capacity)
        : m_capacity(capacity) {}

    auto clear() -> void {
        m_entries.clear();
        m_cursor = 0;
        m_total_blocks = 0;
    }

    [[nodiscard]] auto size() const -> std::size_t { return m_entries.size(); }
    [[nodiscard]] auto cursor() const -> std::size_t { return m_cursor; }
    [[nodiscard]] auto capacity() const -> std::size_t { return m_capacity; }
    [[nodiscard]] auto can_step_back() const -> bool { return m_cursor > 0; }
    [[nodiscard]] auto can_step_forward() const -> bool { return m_cursor < m_entries.size(); }
    [[nodiscard]] auto total_changed_blocks() const -> std::size_t { return m_total_blocks; }

    [[nodiscard]] auto estimated_payload_bytes() const -> std::size_t {
        const auto block_payload = m_total_blocks * (sizeof(Address) + 2 * history_block_size);
        const auto state_payload = m_entries.size() * (2 * sizeof(CPUCoreState));
        return block_payload + state_payload;
    }

    auto record_step(
        const CPUCoreState &before_state,
        const std::array<Byte, 64 * 1024> &before_mem,
        const CPU &after_cpu) -> void {
        StepDeltaEntry entry{};
        entry.before_state = before_state;
        entry.after_state = capture_core_state(after_cpu);

        constexpr std::size_t mem_size = 64 * 1024;
        static_assert(mem_size % history_block_size == 0);
        for (std::size_t base = 0; base < mem_size; base += history_block_size) {
            const auto *before_ptr = before_mem.data() + base;
            const auto *after_ptr = after_cpu.mem.data() + base;
            if (std::memcmp(before_ptr, after_ptr, history_block_size) != 0) {
                ChangedMemoryBlock block{};
                block.base = static_cast<Address>(base);
                std::memcpy(block.before.data(), before_ptr, history_block_size);
                std::memcpy(block.after.data(), after_ptr, history_block_size);
                entry.blocks.push_back(block);
            }
        }

        push_entry(std::move(entry));
    }

    auto step_back(CPU &cpu) -> bool {
        if (!can_step_back()) return false;
        const StepDeltaEntry &entry = m_entries[m_cursor - 1];
        apply_blocks(cpu.mem, entry.blocks, false);
        restore_core_state(cpu, entry.before_state);
        --m_cursor;
        return true;
    }

    auto step_forward(CPU &cpu) -> bool {
        if (!can_step_forward()) return false;
        const StepDeltaEntry &entry = m_entries[m_cursor];
        apply_blocks(cpu.mem, entry.blocks, true);
        restore_core_state(cpu, entry.after_state);
        ++m_cursor;
        return true;
    }

private:
    std::size_t m_capacity = history_default_capacity;
    std::size_t m_cursor = 0;
    std::size_t m_total_blocks = 0;
    std::deque<StepDeltaEntry> m_entries{};

    static auto apply_blocks(
        std::array<Byte, 64 * 1024> &mem,
        const std::vector<ChangedMemoryBlock> &blocks,
        bool use_after) -> void {
        for (const auto &block : blocks) {
            const auto &source = use_after ? block.after : block.before;
            auto start = mem.begin() + static_cast<std::size_t>(block.base);
            std::copy(source.begin(), source.end(), start);
        }
    }

    auto push_entry(StepDeltaEntry &&entry) -> void {
        if (m_capacity == 0) return;

        while (m_entries.size() > m_cursor) {
            m_total_blocks -= m_entries.back().blocks.size();
            m_entries.pop_back();
        }

        if (m_entries.size() == m_capacity && !m_entries.empty()) {
            m_total_blocks -= m_entries.front().blocks.size();
            m_entries.pop_front();
            if (m_cursor > 0) {
                --m_cursor;
            }
        }

        m_total_blocks += entry.blocks.size();
        m_entries.push_back(std::move(entry));
        m_cursor = m_entries.size();
    }
};

} // namespace mos6502
