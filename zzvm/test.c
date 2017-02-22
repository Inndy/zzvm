#include <stdio.h>
#include <stdint.h>
#include "zzvm.h"

#define MAKE_INS(INS, R1, R2, IMM) { INS, (R1 << 4) | R2, IMM }

int main()
{
    int i;
    char buffer[4096];
    ZZ_INSTRUCTION ins[] = {
        MAKE_INS( ZZOP_RAND, 0,     0,     0      ), // 4000: RAND
        MAKE_INS( ZZOP_MOVI, ZZ_R1, 0,     0x1234 ), // 4004: MOV   R1, 0x1234
        MAKE_INS( ZZOP_MOVI, ZZ_R2, 0,     0x2222 ), // 4008: MOV   R2, 0x2222
        MAKE_INS( ZZOP_ADDR, ZZ_R3, ZZ_R1, 0x2    ), // 400c: ADD   R3, R1, R2
        MAKE_INS( ZZOP_PUSI, 0,     0,     0x401c ), // 4010: PUSH  0x401c
        MAKE_INS( ZZOP_CALL, 0,     0,     0      ), // 4014: CALL  0x0000
        MAKE_INS( ZZOP_RET,  0,     0,     0      ), // 4018: RET
        MAKE_INS( ZZOP_NOP,  0,     0,     0      ), // 401c: NOP
        MAKE_INS( ZZOP_ADDI, ZZ_IP, ZZ_IP, 4      ), // 4020: JMP   0x4028
        MAKE_INS( ZZOP_NOP,  0,     0,     0      ), // 4024: NOP
        MAKE_INS( ZZOP_MOVI, ZZ_R1, 0,     12     ), // 4028: MOV   R1, 0x000c
        MAKE_INS( ZZOP_MOVI, ZZ_R4, 0,     0x2000 ), // 402c: MOV   R4, 0x2000
        MAKE_INS( ZZOP_MOVI, ZZ_RA, 0,     0      ), // 4030: MOV   RA, 0x0000
        MAKE_INS( ZZOP_ADDI, ZZ_R1, ZZ_R1, -1     ), // 4034: ADD   R1, 0xffff
        MAKE_INS( ZZOP_ST,   ZZ_R1, ZZ_R4, 0      ), // 4038: ST    R1, R4, 0x0000
        MAKE_INS( ZZOP_LD,   ZZ_R2, ZZ_R4, 0      ), // 403c: LD    R2, R4, 0x0000
        MAKE_INS( ZZOP_JNI,  ZZ_R2, ZZ_RA, -16    ), // 4040: JN    R2, RA, 0x4034
        MAKE_INS( ZZOP_ADDI, ZZ_IP, ZZ_IP, 4      ), // 4044: JMP   0x404c
        MAKE_INS( ZZOP_NOP,  0,     0,     0      ), // 4048: NOP
        MAKE_INS( ZZOP_MOVI, ZZ_RA, 0,     0xffff ), // 404c: MOV   RA, 0xffff
        MAKE_INS( ZZOP_SHRI, ZZ_R1, ZZ_RA, 1      ), // 4050: SHR   R1, RA, 0x0001
        MAKE_INS( ZZOP_SHRI, ZZ_R2, ZZ_R1, 8|1    ), // 4054: SHR   R2, R1, 0x0009
        MAKE_INS( ZZOP_MOVI, ZZ_R3, 0,     4      ), // 4058: MOV   R3, 0x0004
        MAKE_INS( ZZOP_SHRR, ZZ_RA, ZZ_RA, 3      ), // 405c: SHR   RA, RA, R3
        MAKE_INS( ZZOP_HLT,  0,     0,     0      ), // 4060: HLT
    };

    for(i = 0; i < sizeof(ins) / sizeof(ins[0]); i++) {
        ZZ_ADDRESS ip = 0x4000 + i * sizeof(ZZ_INSTRUCTION);
        zz_disasm(ip, &ins[i], buffer, sizeof(buffer));
        printf("%.4x: %s\n", ip, buffer);
    }

    ZZVM *vm;
    int reason;
    if(zz_create(&vm) != ZZ_SUCCESS) {
        printf("Failed to create vm\n");
        return 1;
    }

    zz_put_code(vm, 0x4000, ins, sizeof(ins) / sizeof(ins[0]));

    zz_execute(vm, 1, &reason);
    zz_dump_context(&vm->ctx, buffer, sizeof(buffer)); printf("%s", buffer);

    zz_execute(vm, 5, &reason);
    zz_dump_context(&vm->ctx, buffer, sizeof(buffer)); printf("%s", buffer);

    zz_execute(vm, -1, &reason);
    zz_dump_context(&vm->ctx, buffer, sizeof(buffer)); printf("%s", buffer);
}
