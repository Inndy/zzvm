#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "zzvm.h"

FILE *zz_msg_pipe = NULL;
int zz_msg_level = ZZ_MSGL_MSG;

const char * const ZZ_REGISTER_NAME[] = {
    "RA", "R1", "R2", "R3", "R4", "R5", "SP", "IP"
};

const char * const ZZ_OP_NAME[] = {
    /* 0x00 */ "NOP",
    /* 0x01 */ "NEG",
    /* 0x02 */ "ADD",
    /* 0x03 */ "ADD",
    /* 0x04 */ "MUL",
    /* 0x05 */ "MUL",
    /* 0x06 */ "AND",
    /* 0x07 */ "AND",
    /* 0x08 */ "OR",
    /* 0x09 */ "OR",
    /* 0x0a */ "XOR",
    /* 0x0b */ "XOR",
    /* 0x0c */ "SHR",
    /* 0x0d */ "SHR",
    /* 0x0e */ "NOT",
    /* 0x0f */ "LD",
    /* 0x10 */ "ST",
    /* 0x11 */ "HLT",
    /* 0x12 */ "MOV",
    /* 0x13 */ "MOV",
    /* 0x14 */ "JE",
    /* 0x15 */ "JN",
    /* 0x16 */ "JG",
    /* 0x17 */ "JZ",
    /* 0x18 */ "CALL",
    /* 0x19 */ "RET",
    /* 0x1a */ "POP",
    /* 0x1b */ "PUSH",
    /* 0x1c */ "PUSH",
    /* 0x1d */ "SYS",
    /* 0x1e */ "RAND",
    /* 0x1f */ "XXX",
};

void zz_output_message(int level, char *msg, ...)
{
    if(level < zz_msg_level) {
        return;
    }
    if(zz_msg_pipe == NULL) {
        return;
    }

    va_list args;
    va_start(args, msg);
    vfprintf(zz_msg_pipe, msg, args);
    va_end(args);
}

uint16_t _zz_default_syscall_handler(ZZVM_CTX *ctx)
{
    char c;

    switch (ctx->regs.RA) {
        case 0: // read
            if(read(0, &c, 1) == 1) {
                return c;
            } else {
                return 0xffff;
            }
        case 1: // write
            c = ctx->regs.R1;
            if(write(1, &c, 1) == 1) {
                return 0;
            } else {
                return 0xffff;
            }
    }
    return 0;
}

int zz_create(ZZVM **p_vm)
{
    *p_vm = NULL;

    ZZVM *vm = malloc(sizeof(ZZVM));
    if(vm == NULL) {
        return ZZ_FAILED;
    }

    memset(&vm->ctx, 0, sizeof(ZZVM_CTX));
    vm->ctx.random_seed = time(NULL) ^ (uint64_t)&vm ^ (uint64_t)p_vm ^ (uint64_t)vm ^ random();
#ifdef ZZ_UNIX_ENV
    uint64_t seed;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        if(read(fd, &seed, sizeof seed) == sizeof seed) {
            vm->ctx.random_seed ^= seed;
        }
        close(fd);
    }
#endif
    zz_reg_syscall_handler(vm, _zz_default_syscall_handler);
    vm->ctx.regs.SP = 0xFFF0;
    vm->state = ZZ_ST_SLEEP;
    *p_vm = vm;
    return ZZ_SUCCESS;
}

int zz_destroy(ZZVM *vm)
{
    if(vm->state == ZZ_ST_SLEEP) {
        vm->state = ZZ_ST_FREED;
        free(vm);
        return ZZ_SUCCESS;
    } else if(vm->state == ZZ_ST_FREED) {
        zz_fatal("[FATAL] double free detected\n");
        return ZZ_FAILED;
    } else {
        return ZZ_FAILED;
    }
}

int zz_write_mem(ZZVM *vm, ZZ_ADDRESS addr, void *data, size_t len)
{
    if(vm->state != ZZ_ST_SLEEP) {
        return ZZ_FAILED;
    }
    if(addr + len >= ZZ_MEM_LIMIT) {
        return ZZ_OUT_BOUND;
    }
    memcpy(vm->ctx.memory + addr, data, len);
    return ZZ_SUCCESS;
}

int zz_read_mem(ZZVM *vm, ZZ_ADDRESS addr, void *buffer, size_t len)
{
    if(vm->state != ZZ_ST_SLEEP) {
        return ZZ_FAILED;
    }
    if(addr + len >= ZZ_MEM_LIMIT) {
        return ZZ_OUT_BOUND;
    }
    memcpy(buffer, vm->ctx.memory + addr, len);
    return ZZ_SUCCESS;
}

int zz_put_code(ZZVM *vm, ZZ_ADDRESS addr, ZZ_INSTRUCTION *ins, size_t count)
{
    if(vm->state != ZZ_ST_SLEEP) {
        return ZZ_FAILED;
    }

    ZZVM_CTX *ctx = &vm->ctx;
    int r = zz_write_mem(vm, addr, ins, count * sizeof(ZZ_INSTRUCTION));
    if(r != ZZ_SUCCESS) {
        return r;
    }
    ctx->regs.IP = addr;
    return ZZ_SUCCESS;
}

uint64_t zz_rand(ZZVM_CTX *ctx)
{
	ctx->random_seed ^= ctx->random_seed >> 12; // a
	ctx->random_seed ^= ctx->random_seed << 25; // b
	ctx->random_seed ^= ctx->random_seed >> 27; // c
	return ctx->random_seed * UINT64_C(2685821657736338717);
}

int zz_dump_context(ZZVM_CTX *ctx, char *buffer, size_t buffer_size)
{
    int i, r;

    r = snprintf(buffer, buffer_size,
            "--- Registers ---\n"
            "RA: 0x%.4x\n" "R1: 0x%.4x\n" "R2: 0x%.4x\n" "R3: 0x%.4x\n"
            "R4: 0x%.4x\n" "R5: 0x%.4x\n" "SP: 0x%.4x\n" "IP: 0x%.4x\n"
            "--- Stack ---\n",
            ctx->regs.RA, ctx->regs.R1, ctx->regs.R2, ctx->regs.R3,
            ctx->regs.R4, ctx->regs.R5, ctx->regs.SP, ctx->regs.IP
            );

    for(i = 0; i < 8; i++) {
        if(r >= buffer_size) {
            break;
        }
        ZZ_ADDRESS addr = ctx->regs.SP + i * sizeof(ctx->regs.RA);
        r += snprintf(buffer + r, buffer_size - r, "0x%.4x: 0x%.4x\n", addr,
                *(uint16_t*)&ctx->memory[addr]);
    }
    return r;
}

#define ZZ_MEM(CTX, TYPE, ADDR) ((TYPE*)&((CTX)->memory[(ZZ_ADDRESS)(ADDR)]))

ZZ_INSTRUCTION * zz_fetch(ZZVM_CTX *ctx)
{
    return (ZZ_INSTRUCTION*)&ctx->memory[ctx->regs.IP];
}

uint16_t zz_pop(ZZVM_CTX *ctx)
{
    uint16_t data = *ZZ_MEM(ctx, uint16_t, ctx->regs.SP);
    ctx->regs.SP += 4;
    return data;
}

#define ZZ_SHIFT(VALUE, OFFSET) ((OFFSET) & 8) ? (VALUE << (OFFSET & 7)) : \
                                                 (VALUE >> (OFFSET & 7))

int zz_execute(ZZVM *vm, int count, int *stop_reason)
{
    if(vm->state != ZZ_ST_SLEEP) {
        return ZZ_FAILED;
    }

    ZZVM_CTX *ctx = &vm->ctx;
    ZZ_REGISTERS *regs = &ctx->regs;
    uint16_t *rega = ctx->registers;

    while(1) {
        if(count > 0) {
            count--;
        } else if(count == 0) {
            break;
        }

        if(regs->IP > (ZZ_MEM_LIMIT - sizeof(ZZ_INSTRUCTION))) {
            zz_error("[ERROR] IP out of bound\n");
            *stop_reason = ZZ_OUT_BOUND;
            return ZZ_FAILED;
        }

        ZZ_INSTRUCTION *ins = zz_fetch(ctx);

        int trace = 1;
        if(trace)
        {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%.4x: ", regs->IP);
            zz_disasm(regs->IP, ins, buffer + 6, sizeof(buffer) - 6);
            zz_msg_f("[TRACE] %s\n", buffer);
        }

        uint8_t r1 = ins->reg >> 4;
        uint8_t r2 = ins->reg & 0xf;
        uint8_t r3 = ins->imm & 7;

        if((r1 & 8) || (r2 & 8)) {
            zz_error("[ERROR] invalid register\n");
            *stop_reason = ZZ_INVALID_REGISTER;
            return ZZ_FAILED;
        }

        switch(ins->op) {
            case ZZOP_NOP:  break;
            case ZZOP_NEG:  rega[r1] = -rega[r2]; break;
            case ZZOP_ADDR: rega[r1] = rega[r2] + rega[r3]; break;
            case ZZOP_ADDI: rega[r1] = rega[r2] + ins->imm; break;
            case ZZOP_MULR: rega[r1] = rega[r2] * rega[r3]; break;
            case ZZOP_MULI: rega[r1] = rega[r2] * ins->imm; break;
            case ZZOP_ANDR: rega[r1] = rega[r2] & rega[r3]; break;
            case ZZOP_ANDI: rega[r1] = rega[r2] & ins->imm; break;
            case ZZOP_ORR:  rega[r1] = rega[r2] | rega[r3]; break;
            case ZZOP_ORI:  rega[r1] = rega[r2] | ins->imm; break;
            case ZZOP_XORR: rega[r1] = rega[r2] ^ rega[r3]; break;
            case ZZOP_XORI: rega[r1] = rega[r2] ^ ins->imm; break;
            case ZZOP_SHRR: rega[r1] = ZZ_SHIFT(rega[r2], rega[r3]); break;
            case ZZOP_SHRI: rega[r1] = ZZ_SHIFT(rega[r2], ins->imm); break;
            case ZZOP_NOT:  rega[r1] = ~rega[r2]; break;
            case ZZOP_LD:   rega[r1] = *ZZ_MEM(ctx, uint16_t, rega[r2] + ins->imm); break;
            case ZZOP_ST:   *ZZ_MEM(ctx, uint16_t, rega[r2] + ins->imm) = rega[r1]; break;

            case ZZOP_HLT:
                zz_msg("[MESSAGE] vm halt\n");
                *stop_reason = ZZ_HALT;
                return ZZ_SUCCESS;

            case ZZOP_MOVR: rega[r1] = rega[r2]; break;
            case ZZOP_MOVI: rega[r1] = ins->imm; break;

            case ZZOP_JEI:
                if(rega[r1] == rega[r2]) {
                    regs->IP += ins->imm;
                }
                break;

            case ZZOP_JNI:
                if(rega[r1] != rega[r2]) {
                    regs->IP += ins->imm;
                }
                break;

            case ZZOP_JGI:
                if(rega[r1] > rega[r2]) {
                    regs->IP += ins->imm;
                }
                break;

            case ZZOP_JZI:
                if(rega[r1] == 0) {
                    regs->IP += ins->imm;
                }
                break;

            case ZZOP_CALL:
                regs->SP -= sizeof(regs->RA);
                *(uint16_t*)&ctx->memory[(uint64_t)regs->SP] = regs->IP + sizeof(ZZ_INSTRUCTION);
                regs->IP += ins->imm;
                break;

            case ZZOP_RET:
                regs->IP = *(uint16_t*)&ctx->memory[(uint64_t)regs->SP];
                regs->SP += sizeof(regs->RA);
                continue; // skip IP increment

            case ZZOP_POP:
                rega[r1] = *(uint16_t*)&ctx->memory[(uint64_t)regs->SP];
                regs->SP += sizeof(regs->RA);
                break;

            case ZZOP_PUSH:
                regs->SP -= sizeof(regs->RA);
                *(uint16_t*)&ctx->memory[(uint64_t)regs->SP] = rega[r1];
                break;

            case ZZOP_PUSI:
                regs->SP -= sizeof(regs->RA);
                *(uint16_t*)&ctx->memory[(uint64_t)regs->SP] = ins->imm;
                break;

            case ZZOP_SYS:
                regs->RA = vm->syscall_handler(ctx);
                break;

            case ZZOP_RAND:
                regs->RA = zz_rand(ctx);
                break;

            default:
                *stop_reason = ZZ_INVALID_INSTRUCTION;
                return ZZ_FAILED;
        }
        regs->IP += sizeof(ZZ_INSTRUCTION);
    }

    *stop_reason = ZZ_SUCCESS;
    return ZZ_SUCCESS;
}

int _zz_disasm_0(char *buffer, size_t  limit, ZZ_ADDRESS ip, ZZ_INSTRUCTION *ins)
{
    snprintf(buffer, limit, "%-5s", ZZ_OP_NAME[ins->op]);
    return ZZ_SUCCESS;
}

int _zz_disasm_1r(char *buffer, size_t  limit, ZZ_ADDRESS ip, ZZ_INSTRUCTION *ins)
{
    snprintf(buffer, limit, "%-5s %s", ZZ_OP_NAME[ins->op], ZZ_REGISTER_NAME[ins->reg >> 4]);
    return ZZ_SUCCESS;
}

int _zz_disasm_2r(char *buffer, size_t  limit, ZZ_ADDRESS ip, ZZ_INSTRUCTION *ins)
{
    snprintf(buffer, limit, "%-5s %s, %s", ZZ_OP_NAME[ins->op], ZZ_REGISTER_NAME[ins->reg >> 4],
             ZZ_REGISTER_NAME[ins->reg & 7]);
    return ZZ_SUCCESS;
}

int _zz_disasm_3r(char *buffer, size_t  limit, ZZ_ADDRESS ip, ZZ_INSTRUCTION *ins)
{
    snprintf(buffer, limit, "%-5s %s, %s, %s", ZZ_OP_NAME[ins->op],
             ZZ_REGISTER_NAME[ins->reg >> 4], ZZ_REGISTER_NAME[ins->reg & 7],
             ZZ_REGISTER_NAME[ins->imm & 7]);
    return ZZ_SUCCESS;
}

int _zz_disasm_1i(char *buffer, size_t  limit, ZZ_ADDRESS ip, ZZ_INSTRUCTION *ins)
{
    snprintf(buffer, limit, "%-5s 0x%.4x", ZZ_OP_NAME[ins->op], ins->imm);
    return ZZ_SUCCESS;
}

int _zz_disasm_1j(char *buffer, size_t limit, ZZ_ADDRESS ip, ZZ_INSTRUCTION *ins)
{
    const char *op_name = ZZ_OP_NAME[ins->op];
    if(ins->op == ZZOP_ADDI) {
        op_name = "JMP";
    }
    snprintf(buffer, limit, "%-5s 0x%.4x", op_name,
             (ZZ_ADDRESS)(ins->imm + sizeof(ZZ_INSTRUCTION) + ip));
    return ZZ_SUCCESS;
}

int _zz_disasm_2i(char *buffer, size_t  limit, ZZ_ADDRESS ip, ZZ_INSTRUCTION *ins)
{
    snprintf(buffer, limit, "%-5s %s, 0x%.4x", ZZ_OP_NAME[ins->op],
             ZZ_REGISTER_NAME[ins->reg >> 4], ins->imm);
    return ZZ_SUCCESS;
}

int _zz_disasm_2j(char *buffer, size_t limit, ZZ_ADDRESS ip, ZZ_INSTRUCTION *ins)
{
    snprintf(buffer, limit, "%-5s %s, 0x%.4x", ZZ_OP_NAME[ins->op],
             ZZ_REGISTER_NAME[ins->reg >> 4], (ZZ_ADDRESS)(ins->imm + sizeof(ZZ_INSTRUCTION) + ip));
    return ZZ_SUCCESS;
}

int _zz_disasm_3i(char *buffer, size_t  limit, ZZ_ADDRESS ip, ZZ_INSTRUCTION *ins)
{
    snprintf(buffer, limit, "%-5s %s, %s, 0x%.4x", ZZ_OP_NAME[ins->op],
             ZZ_REGISTER_NAME[ins->reg >> 4], ZZ_REGISTER_NAME[ins->reg & 7],
             ins->imm);
    return ZZ_SUCCESS;
}

int _zz_disasm_3j(char *buffer, size_t limit, ZZ_ADDRESS ip, ZZ_INSTRUCTION *ins)
{
    snprintf(buffer, limit, "%-5s %s, %s, 0x%.4x", ZZ_OP_NAME[ins->op],
             ZZ_REGISTER_NAME[ins->reg >> 4], ZZ_REGISTER_NAME[ins->reg & 7],
             (ZZ_ADDRESS)(ins->imm + sizeof(ZZ_INSTRUCTION) + ip));
    return ZZ_SUCCESS;
}

int zz_disasm(ZZ_ADDRESS ip, ZZ_INSTRUCTION *ins, char *buffer, size_t limit)
{
    uint8_t r1 = ins->reg >> 4;
    uint8_t r2 = ins->reg & 0xf;
    uint8_t r3 = ins->imm & 7;

    switch(ins->op) {
        case ZZOP_NOP:
        case ZZOP_HLT:
        case ZZOP_RET:
        case ZZOP_SYS:
        case ZZOP_RAND:
            return _zz_disasm_0(buffer, limit, ip, ins);

        case ZZOP_NEG:
        case ZZOP_NOT:
        case ZZOP_MOVR:
            return _zz_disasm_2r(buffer, limit, ip, ins);

        case ZZOP_ADDR:
        case ZZOP_MULR:
        case ZZOP_ANDR:
        case ZZOP_ORR:
        case ZZOP_XORR:
        case ZZOP_SHRR:
            return _zz_disasm_3r(buffer, limit, ip, ins);

        case ZZOP_ADDI:
            if(r1 == r2 && r1 == ZZ_IP) {
                return _zz_disasm_1j(buffer, limit, ip, ins);
            }
        case ZZOP_MULI:
        case ZZOP_ANDI:
        case ZZOP_ORI:
        case ZZOP_XORI:
        case ZZOP_SHRI:
            return _zz_disasm_3i(buffer, limit, ip, ins);

        case ZZOP_JEI:
        case ZZOP_JNI:
        case ZZOP_JGI:
        case ZZOP_JZI:
            return _zz_disasm_3j(buffer, limit, ip, ins);

        case ZZOP_LD:
        case ZZOP_ST:
            return _zz_disasm_3i(buffer, limit, ip, ins);
            break;

        case ZZOP_MOVI:
            return _zz_disasm_2i(buffer, limit, ip, ins);

        case ZZOP_CALL:
        case ZZOP_PUSI:
            return _zz_disasm_1i(buffer, limit, ip, ins);

        case ZZOP_POP:
        case ZZOP_PUSH:
            return _zz_disasm_1r(buffer, limit, ip, ins);

        default:
            snprintf(buffer, limit, "WTF?!");
            return ZZ_FAILED;
    }
    return ZZ_FAILED;
}

int zz_reg_syscall_handler(ZZVM *vm, ZZ_SYSCALL_HANDLER handler)
{
    if(vm && handler) {
        vm->syscall_handler = handler;
        return 0;
    } else {
        return 1;
    }
}
