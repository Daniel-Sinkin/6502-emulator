/* danielsinkin97@gmail.com */
#pragma once

#include "6502.hpp"

namespace mos6502 {
class ProgramWriter {
public:
    Address addr;

    explicit ProgramWriter(CPU &cpu, Address addr = 0x0000)
        : addr(addr), cpu(cpu) {}

    void operator()(Byte value) { cpu.mem[addr++] = value; }

    /* † BRK / interrupts & status */
    void brk() { (*this)(0x00); }
    void php() { (*this)(0x08); }
    void plp() { (*this)(0x28); }
    void pha() { (*this)(0x48); }
    void pla() { (*this)(0x68); }
    void rti() { (*this)(0x40); }
    void rts() { (*this)(0x60); }

    /* † Branch instructions (all relative) */
    void bpl() { (*this)(0x10); }
    void bmi() { (*this)(0x30); }
    void bvc() { (*this)(0x50); }
    void bvs() { (*this)(0x70); }
    void bcc() { (*this)(0x90); }
    void bcs() { (*this)(0xB0); }
    void bne() { (*this)(0xD0); }
    void beq() { (*this)(0xF0); }

    /* † Flag manipulation (implied) */
    void clc() { (*this)(0x18); }
    void sec() { (*this)(0x38); }
    void cli() { (*this)(0x58); }
    void sei() { (*this)(0x78); }
    void clv() { (*this)(0xB8); }
    void cld() { (*this)(0xD8); }
    void sed() { (*this)(0xF8); }
    void nop() { (*this)(0xEA); }

    /* † Jumps & subroutines */
    void jmp_absolute() { (*this)(0x4C); } //  absolute   $HHLL
    void jmp_indirect() { (*this)(0x6C); } // (absolute)  ($HHLL)
    void jsr_absolute() { (*this)(0x20); }

    /* † Arithmetic / logic helpers — ADC, SBC, CMP–CPX–CPY, BIT, etc. */
    /* --- ORA -------------------------------------------------------- */
    void ora_indirect_x() { (*this)(0x01); }
    void ora_zero_page() { (*this)(0x05); }
    void ora_immediate() { (*this)(0x09); }
    void ora_absolute() { (*this)(0x0D); }
    void ora_indirect_y() { (*this)(0x11); }
    void ora_zero_page_x() { (*this)(0x15); }
    void ora_absolute_y() { (*this)(0x19); }
    void ora_absolute_x() { (*this)(0x1D); }

    /* --- AND (already partially present) --------------------------- */
    void and_zeropage() { (*this)(0x25); }
    void and_immediate() { (*this)(0x29); }
    void and_zeropage_x() { (*this)(0x35); }
    void and_absolute() { (*this)(0x2D); }
    void and_absolute_x() { (*this)(0x3D); }
    void and_absolute_y() { (*this)(0x39); }
    void and_indirect_x() { (*this)(0x21); }
    void and_indirect_y() { (*this)(0x31); }

    /* --- EOR -------------------------------------------------------- */
    void eor_indirect_x() { (*this)(0x41); }
    void eor_zero_page() { (*this)(0x45); }
    void eor_immediate() { (*this)(0x49); }
    void eor_absolute() { (*this)(0x4D); }
    void eor_indirect_y() { (*this)(0x51); }
    void eor_zero_page_x() { (*this)(0x55); }
    void eor_absolute_y() { (*this)(0x59); }
    void eor_absolute_x() { (*this)(0x5D); }

    /* --- ADC -------------------------------------------------------- */
    void adc_indirect_x() { (*this)(0x61); }
    void adc_zero_page() { (*this)(0x65); }
    void adc_immediate() { (*this)(0x69); }
    void adc_absolute() { (*this)(0x6D); }
    void adc_indirect_y() { (*this)(0x71); }
    void adc_zero_page_x() { (*this)(0x75); }
    void adc_absolute_y() { (*this)(0x79); }
    void adc_absolute_x() { (*this)(0x7D); }

    /* --- SBC -------------------------------------------------------- */
    void sbc_indirect_x() { (*this)(0xE1); }
    void sbc_zero_page() { (*this)(0xE5); }
    void sbc_immediate() { (*this)(0xE9); }
    void sbc_absolute() { (*this)(0xED); }
    void sbc_indirect_y() { (*this)(0xF1); }
    void sbc_zero_page_x() { (*this)(0xF5); }
    void sbc_absolute_y() { (*this)(0xF9); }
    void sbc_absolute_x() { (*this)(0xFD); }

    /* --- CMP -------------------------------------------------------- */
    void cmp_indirect_x() { (*this)(0xC1); }
    void cmp_zero_page() { (*this)(0xC5); }
    void cmp_immediate() { (*this)(0xC9); }
    void cmp_absolute() { (*this)(0xCD); }
    void cmp_indirect_y() { (*this)(0xD1); }
    void cmp_zero_page_x() { (*this)(0xD5); }
    void cmp_absolute_y() { (*this)(0xD9); }
    void cmp_absolute_x() { (*this)(0xDD); }

    /* --- CPX -------------------------------------------------------- */
    void cpx_immediate() { (*this)(0xE0); }
    void cpx_zero_page() { (*this)(0xE4); }
    void cpx_absolute() { (*this)(0xEC); }

    /* --- CPY -------------------------------------------------------- */
    void cpy_immediate() { (*this)(0xC0); }
    void cpy_zero_page() { (*this)(0xC4); }
    void cpy_absolute() { (*this)(0xCC); }

    /* --- BIT -------------------------------------------------------- */
    void bit_zero_page() { (*this)(0x24); }
    void bit_absolute() { (*this)(0x2C); }

    /* † Shifts & rotates -------------------------------------------- */
    /* ASL */
    void asl_zero_page() { (*this)(0x06); }
    void asl_accumulator() { (*this)(0x0A); }
    void asl_absolute() { (*this)(0x0E); }
    void asl_zero_page_x() { (*this)(0x16); }
    void asl_absolute_x() { (*this)(0x1E); }
    /* LSR */
    void lsr_zero_page() { (*this)(0x46); }
    void lsr_accumulator() { (*this)(0x4A); }
    void lsr_absolute() { (*this)(0x4E); }
    void lsr_zero_page_x() { (*this)(0x56); }
    void lsr_absolute_x() { (*this)(0x5E); }
    /* ROL */
    void rol_zero_page() { (*this)(0x26); }
    void rol_accumulator() { (*this)(0x2A); }
    void rol_absolute() { (*this)(0x2E); }
    void rol_zero_page_x() { (*this)(0x36); }
    void rol_absolute_x() { (*this)(0x3E); }
    /* ROR */
    void ror_zero_page() { (*this)(0x66); }
    void ror_accumulator() { (*this)(0x6A); }
    void ror_absolute() { (*this)(0x6E); }
    void ror_zero_page_x() { (*this)(0x76); }
    void ror_absolute_x() { (*this)(0x7E); }

    /* † Load / store ------------------------------------------------- */
    /* LDA */
    void lda_immediate() { (*this)(0xA9); }
    void lda_indirect_x() { (*this)(0xA1); }
    void lda_zero_page() { (*this)(0xA5); }
    void lda_absolute() { (*this)(0xAD); }
    void lda_indirect_y() { (*this)(0xB1); }
    void lda_zero_page_x() { (*this)(0xB5); }
    void lda_absolute_y() { (*this)(0xB9); }
    void lda_absolute_x() { (*this)(0xBD); }

    /* LDX */
    void ldx_immediate() { (*this)(0xA2); }
    void ldx_zero_page() { (*this)(0xA6); }
    void ldx_absolute() { (*this)(0xAE); }
    void ldx_zero_page_y() { (*this)(0xB6); }
    void ldx_absolute_y() { (*this)(0xBE); }

    /* LDY */
    void ldy_immediate() { (*this)(0xA0); }
    void ldy_zero_page() { (*this)(0xA4); }
    void ldy_absolute() { (*this)(0xAC); }
    void ldy_zero_page_x() { (*this)(0xB4); }
    void ldy_absolute_x() { (*this)(0xBC); }

    /* STA */
    void sta_indirect_x() { (*this)(0x81); }
    void sta_zero_page() { (*this)(0x85); }
    void sta_absolute() { (*this)(0x8D); }
    void sta_indirect_y() { (*this)(0x91); }
    void sta_zero_page_x() { (*this)(0x95); }
    void sta_absolute_y() { (*this)(0x99); }
    void sta_absolute_x() { (*this)(0x9D); }

    /* STX */
    void stx_zero_page() { (*this)(0x86); }
    void stx_absolute() { (*this)(0x8E); }
    void stx_zero_page_y() { (*this)(0x96); }

    /* STY */
    void sty_zero_page() { (*this)(0x84); }
    void sty_absolute() { (*this)(0x8C); }
    void sty_zero_page_x() { (*this)(0x94); }

    /* INC / DEC */
    void inc_zero_page() { (*this)(0xE6); }
    void inc_absolute() { (*this)(0xEE); }
    void inc_zero_page_x() { (*this)(0xF6); }
    void inc_absolute_x() { (*this)(0xFE); }

    void dec_zero_page() { (*this)(0xC6); }
    void dec_absolute() { (*this)(0xCE); }
    void dec_zero_page_x() { (*this)(0xD6); }
    void dec_absolute_x() { (*this)(0xDE); }

    /* † Registers & stack transfers (implied) */
    void tax() { (*this)(0xAA); }
    void txa() { (*this)(0x8A); }
    void dex() { (*this)(0xCA); }
    void inx() { (*this)(0xE8); }

    void tay() { (*this)(0xA8); }
    void tya() { (*this)(0x98); }
    void dey() { (*this)(0x88); }
    void iny() { (*this)(0xC8); }

    void tsx() { (*this)(0xBA); }
    void txs() { (*this)(0x9A); }

private:
    CPU &cpu;
};
} // namespace mos6502