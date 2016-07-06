/*
 * # MUST HAVE
 * nop, hlt
 *
 * # ALU
 * add, mul, neg
 * addi, muli
 * and, or, xor, not
 * andi, ori, xori
 *
 * # MEM
 * ld, st
 *
 * # DATA
 * mov, movi
 *
 * # CONTROL FLOW
 * je, jn, jg, jz
 * jei, jni, jgi
 *
 * # FUNCTION
 * call, ret
 *
 * # STACK
 * push, pushi, pop
 *
 * # SPECIAL
 * sys, rand
 */

enum ZZOP {
    ZZOP_NOP    = 0x00,
    ZZOP_NEG    = 0x01,
    ZZOP_ADDR   = 0x02,
    ZZOP_ADDI   = 0x03,
    ZZOP_MULR   = 0x04,
    ZZOP_MULI   = 0x05,
    ZZOP_ANDR   = 0x06,
    ZZOP_ANDI   = 0x07,
    ZZOP_ORR    = 0x08,
    ZZOP_ORI    = 0x09,
    ZZOP_XORR   = 0x0a,
    ZZOP_XORI   = 0x0b,
    ZZOP_NOT    = 0x0c,
    ZZOP_LD     = 0x0d,
    ZZOP_ST     = 0x0e,
    ZZOP_HLT    = 0x0f,
    ZZOP_MOVR   = 0x10,
    ZZOP_MOVI   = 0x11,
    ZZOP_JEI    = 0x12,
    ZZOP_JNI    = 0x13,
    ZZOP_JGI    = 0x14,
    ZZOP_JZI    = 0x15,
    ZZOP_CALL   = 0x16,
    ZZOP_RET    = 0x17,
    ZZOP_POP    = 0x18,
    ZZOP_PUSH   = 0x19,
    ZZOP_PUSI   = 0x1a,
    ZZOP_SYS    = 0x1b,
    ZZOP_RAND   = 0x1c,
};
