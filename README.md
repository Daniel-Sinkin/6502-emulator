# 6502 Emulator

OpenGL + SDL2 + Dear ImGui MOS 6502 emulator/debugger.

## Current status
- Opcode table: official 151 opcodes mapped.
- Instruction families: implemented in `exec_func` (`src/6502/6502.hpp`).
- Addressing modes: implemented (`immediate`, `zero_page*`, `absolute*`, `indirect*`, `relative`, `accum`, `implied`).
- Debug UI: register view, memory windows, framebuffer view, disassembly with inline comments, and an `Instruction Explain` panel with 3-4 human-readable lines using live register/memory values.
- Portfolio dashboard: in-app KPI bars + support matrix with `% support` and hardware-closeness scoring.
- Interactive showcase modes: pure-6502 framebuffer pattern + keyboard-driven snake using memory-mapped I/O bytes.

## Known limitations
- Timing is not cycle-accurate yet.
- Interrupt handling is instruction-boundary accurate (`IRQ`/`NMI` vectors + stack/status push), but not cycle-accurate bus timing.
- Unofficial/undocumented opcodes are intentionally not implemented.

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
- `Arrow keys` / `WASD`: snake direction (when snake demo is active)
- `R`: reset active demo (snake restart or pattern reset)
- `ESC`: quit

Debug stepping keeps a fixed-capacity timeline (`240` steps by default) and stores RAM changes as block deltas (`256`-byte blocks) instead of full 64KB snapshots per step.

## Included runnable demos
Use the ImGui window `Portfolio Dashboard` to switch between demos:

1. `Pattern Program (pure 6502)`:
- Loads a generated 6502 program at `$0600`
- Treats `$0200-$05FF` as a 32x32 pixel buffer
- Continuously fills it with an animated pattern using only 6502 instructions

2. `Snake (framebuffer + host I/O)`:
- Renders to the same `$0200-$05FF` framebuffer window
- Accepts direction input from keyboard (`Arrow keys` / `WASD`)
- Exposes input/game telemetry bytes for memory-map storytelling:
  - `$00F0`: direction
  - `$00F1`: snake tick counter
  - `$00F2`: score
  - `$00F3`: game-over flag

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
