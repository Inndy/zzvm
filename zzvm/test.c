#include <stdio.h>
#include <stdint.h>
#include "zzvm.h"

int main()
{
    int i;
    char buffer[4096];
    ZZ_INSTRUCTION ins[] = {
        { ZZOP_RAND, 0,    0      }, // 4000: RAND
        { ZZOP_MOVI, 0x10, 0x1234 }, // 4004: MOV   R1, 0x1234
        { ZZOP_MOVI, 0x20, 0x2222 }, // 4008: MOV   R2, 0x2222
        { ZZOP_ADDR, 0x31, 0x2    }, // 400c: ADD   R3, R1, R2
        { ZZOP_PUSI, 0,    0x401c }, // 4010: PUSH  0x401c
        { ZZOP_CALL, 0,    0      }, // 4014: CALL  0x0000
        { ZZOP_RET,  0,    0      }, // 4018: RET
        { ZZOP_NOP,  0,    0      }, // 401c: NOP
        { ZZOP_ADDI, 0x77, 4      }, // 4020: JMP   0x4028
        { ZZOP_NOP,  0,    0      }, // 4024: NOP
        { ZZOP_MOVI, 0x10, 12     }, // 4028: MOV   R1, 0x000c
        { ZZOP_MOVI, 0x40, 0x2000 }, // 402c: MOV   R4, 0x2000
        { ZZOP_MOVI, 0x00, 0      }, // 4030: MOV   RA, 0x0000
        { ZZOP_ADDI, 0x11, -1     }, // 4034: ADD   R1, R1, 0xffff
        { ZZOP_ST,   0x14, 0      }, // 4038: ST    R1, R4, 0x0000
        { ZZOP_LD,   0x24, 0      }, // 403c: LD    R2, R4, 0x0000
        { ZZOP_JNI,  0x20, -16    }, // 4040: JN    R2, RA, 0x4034
        { ZZOP_ADDI, 0x77, 4      }, // 4044: JMP   0x404c
        { ZZOP_NOP,  0,    0      }, // 4048: NOP
        { ZZOP_MOVI, 0,    0xffff }, // 404c: MOV   RA, 0xffff
        { ZZOP_SHRI, 0x10, 1      }, // 4050: SHR   R1, RA, 0x0001
        { ZZOP_SHRI, 0x21, 8|1    }, // 4054: SHR   R2, R1, 0x0009
        { ZZOP_MOVI, 0x30, 4      }, // 4058: MOV   R3, 0x0004
        { ZZOP_SHRR, 0x00, 3      }, // 405c: SHR   RA, RA, R3
        { ZZOP_HLT,  0,    0      }, // 4060: HLT
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
