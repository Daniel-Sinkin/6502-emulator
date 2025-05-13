/*danielsinkin97@gmail.com*/
#pragma once

#include <array>

#include "../types.hpp"

using TYPES::Byte;
using TYPES::Word;

namespace mos6502 {

struct cpu {
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

    uint64_t cycles = 0;
};

} // namespace mos6502
