# 6502 Emulator

OpenGL + SDL2 + Dear ImGui MOS 6502 emulator/debugger.

## Current status
- Opcode table: official 151 opcodes mapped.
- Instruction families: implemented in `exec_func` (`src/6502/6502.hpp`).
- Addressing modes: implemented (`immediate`, `zero_page*`, `absolute*`, `indirect*`, `relative`, `accum`, `implied`).
- Debug UI: register view, memory windows, framebuffer view, disassembly with inline comments, and an `Instruction Explain` panel with 3-4 human-readable lines using live register/memory values.

## Known limitations
- Timing is not cycle-accurate yet.
- Interrupt handling is instruction-boundary accurate (`IRQ`/`NMI` vectors + stack/status push), but not cycle-accurate bus timing.

## Build requirements
- CMake 3.16+
- C++23 compiler
- Python 3 (used by `build.sh` to generate `assets/sound/beep.wav`)
- Internet access on first configure (CMake `FetchContent` pulls dependencies from GitHub)

## Build and run
```bash
./build.sh
./build/main
```

Precompiled headers are enabled by default. To disable:
```bash
cmake -S . -B build -DENABLE_PCH=OFF
```

Or use:
```bash
./cnr
```

## Tests
```bash
cmake -S . -B build
cmake --build build --target cpu_tests
ctest --test-dir build --output-on-failure
```

Third-party adapted tests and vectors in `tests/cpu_tests.cpp` are attributed to py65.
The BSD-3-Clause notice is included at `tests/third_party/py65.LICENSE`.

## Controls
- `SPACE`: toggle run/debug mode
- `N`: single-step one CPU tick
- `B`: step back
- `F`: step forward (redo)
- `I`: toggle IRQ line
- `M`: pulse NMI
- `ESC`: quit

Debug stepping keeps a fixed-capacity timeline (`240` steps by default) and stores RAM changes as block deltas (`256`-byte blocks) instead of full 64KB snapshots per step.

## Included runnable demo
`src/main.cpp` loads a framebuffer demo program at `$0600` that:
- Treats `$0200-$05FF` as a 32x32 pixel buffer
- Continuously fills it with an animated color pattern using only 6502 instructions
- Renders that memory as a color screen in the ImGui window `Framebuffer (0x0200-0x05FF)`

## References
- [Datasheet](https://web.archive.org/web/20221029042234if_/http://archive.6502.org/datasheets/mos_6500_mpu_preliminary_may_1976.pdf)
- [Instruction Set](https://www.masswerk.at/6502/6502_instruction_set.html)
- [Emulating a 6502 System in JavaScript • Matt Godbolt • GOTO 2016](https://www.youtube.com/watch?v=7WuRq-Wmw5o)
- [Basics - 6502 Assembly Crash Course 01](https://www.youtube.com/watch?v=yEiNs7pKNh8)
- [The first LowSpec Processor](https://www.youtube.com/watch?v=lP2ZBp9O0mk)
- [The 6502 CPU Powered a Whole Generation!](https://www.youtube.com/watch?v=acUH4lWe2NQ)
- [“Hello, world” from scratch on a 6502 — Part 1](https://www.youtube.com/watch?v=LnzuMJLZRdU)
- [Software Emulators vs FPGAs](https://www.youtube.com/watch?v=sMMiBEhnizE)
- [Advanced 6502 Assembly Programming for the Apple II](https://www.youtube.com/watch?v=WEliEAc3ZyA)
- [27c3: Reverse Engineering the MOS 6502 CPU (en)](https://www.youtube.com/watch?v=fWqBmmPQP40)

## Credits
- Font: Monaspace Krypton
- License: SIL Open Font License 1.1
- Source: https://monaspace.githubnext.com/
