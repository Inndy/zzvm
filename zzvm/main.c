#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zzvm.h"

#define ZZ_IMAGE_MAGIC   0x7a5a /* 'Zz' */
#define ZZ_IMAGE_VERSION 0x0

typedef struct __attribute__((__packed__)) {
    ZZ_ADDRESS section_addr;
    ZZ_ADDRESS section_size;
} ZZ_SECTION_HEADER;

typedef struct __attribute__((__packed__)) {
    uint16_t          magic;
    uint16_t          file_ver;
    ZZ_ADDRESS        entry;
    uint16_t          section_count;
    ZZ_SECTION_HEADER sections[0];
} ZZ_IMAGE_HEADER;

int zz_decode_byte(const char *encoded, uint8_t *out)
{
    uint8_t value = 0;

    for(int i = 0; i < 8; i++)
    {
        value <<= 1;
        if(encoded[i] == 'Z') {
            value |= 1;
        } else if(encoded[i] != 'z') {
            return 0;
        }
    }

    *out = value;
    return 1;
}

int zz_decode_data(void *dst, const void *src, size_t unpacked_size)
{
    uint8_t *dst8 = (uint8_t *)dst;
    uint64_t i, val, packed_size;

    for(i = 0; i < unpacked_size; i++) {
        if(!zz_decode_byte(src + i * 8, &dst8[i])) {
            return 0;
        }
    }

    return 1;
}

int zz_read_image_header(FILE *fp, ZZ_IMAGE_HEADER *header)
{
    char buffer[sizeof(*header) * 8];

    if(fread(buffer, sizeof(buffer), 1, fp) != 1) {
        fprintf(stderr, "Unable to read file\n");
        return 0;
    }

    if(!zz_decode_data(header, buffer, sizeof(*header))) {
        fprintf(stderr, "Malformed file\n");
        return 0;
    }

    return 1;
}

int zz_verify_image_header(ZZ_IMAGE_HEADER *header)
{
    if(header->magic != ZZ_IMAGE_MAGIC) {
        fprintf(stderr, "Invalid file magic (%.4x)\n", header->magic);
        return 0;
    }

    if(header->file_ver != ZZ_IMAGE_VERSION) {
        fprintf(stderr, "Mismatch file version\n");
        return 0;
    }

    return 1;
}

int zz_read_image_section(FILE *fp, ZZ_SECTION_HEADER *section)
{
    char buffer[sizeof(*section) * 8];

    if(fread(buffer, sizeof(buffer), 1, fp) != 1) {
        fprintf(stderr, "Can not read file section\n");
        return 0;
    }

    if(!zz_decode_data(section, buffer, sizeof(*section))) {
        fprintf(stderr, "Malformed file\n");
        return 0;
    }

    return 1;
}

int zz_load_image_header(FILE *fp, ZZ_IMAGE_HEADER **out_header)
{
    ZZ_IMAGE_HEADER *header = (ZZ_IMAGE_HEADER *)malloc(sizeof(ZZ_IMAGE_HEADER));

    if(!zz_read_image_header(fp, header)) {
        return 0;
    }
    if(!zz_verify_image_header(header)) {
        return 0;
    }

    header = (ZZ_IMAGE_HEADER *)realloc(header, sizeof(ZZ_IMAGE_HEADER) +
            sizeof(ZZ_SECTION_HEADER) * header->section_count);

    for(int i = 0; i < header->section_count; i++) {
        if(!zz_read_image_section(fp, &header->sections[i])) {
            return 0;
        }
    }

    *out_header = header;
    return 1;
}

int zz_load_image_to_vm(const char *filename, ZZVM *vm)
{
    FILE *fp;
    ZZ_IMAGE_HEADER *header = NULL;

    if(strcmp(filename, "-") == 0 || filename == NULL) {
        fp = stdin;
    } else {
        fp = fopen(filename, "rb");
    }

    if(!fp) {
        fprintf(stderr, "Unable to open file\n");
        return 0;
    }

    if(!zz_load_image_header(fp, &header)) {
        return 0;
    }

    vm->ctx.regs.IP = header->entry;

    size_t buffer_size = 8192;
    char *buffer = malloc(buffer_size);

    for(int i = 0; i < header->section_count; i++) {
        ZZ_SECTION_HEADER *section_header = &header->sections[i];

        size_t size_bound = (size_t)section_header->section_addr +
                            (size_t)section_header->section_size;
        size_t encoded_size = section_header->section_size * 8;

        if(size_bound >= sizeof(vm->ctx.memory)) {
            fprintf(stderr, "Section#%d out of scope\n", i);
            goto fail;
        }

        if(buffer_size < encoded_size) {
            buffer_size = encoded_size;
            buffer = realloc(buffer, buffer_size);
        }

        if(fread(buffer, encoded_size, 1, fp) != 1) {
            fprintf(stderr, "Can not read section #%d\n", i);
            goto fail;
        }

        if(!zz_decode_data(&vm->ctx.memory[section_header->section_addr], buffer, section_header->section_size)) {
            fprintf(stderr, "Malformed file\n");
            goto fail;
        }
    }

    if(fp != stdin) fclose(fp);
    free(header);
    free(buffer);

    return 1;

fail:
    if(fp && fp != stdin) fclose(fp);
    if(header) free(header);
    if(buffer) free(buffer);
    return 0;
}

void dump_vm_context(ZZVM *vm)
{
    char buffer[1024];
    zz_dump_context(&vm->ctx, buffer, sizeof(buffer));
    puts(buffer);
}

int run_file(const char *filename, int trace)
{
    ZZVM *vm;
    if(zz_create(&vm) != ZZ_SUCCESS) {
        fprintf(stderr, "Can not create vm\n");
        return 0;
    }

    if(!zz_load_image_to_vm(filename, vm)) {
        return 0;
    }

    int stop_reason = ZZ_SUCCESS;
    while(stop_reason != ZZ_HALT) {
        int ret_val = zz_execute(vm, 1, &stop_reason);

        if(trace) {
            dump_vm_context(vm);
        }

        if(ret_val != ZZ_SUCCESS) {
            fprintf(stderr, "Failed to execute, stop_reason = %d\n", stop_reason);
            break;
        }
    }

    if(!trace) {
        dump_vm_context(vm);
    }

    zz_destroy(vm);
    return 1;
}

void usage(const char *prog)
{
    printf("Usage: %s <command> zz-image\n\n"
           "  available command:\n"
           "    run\n"
           "      run until HLT instruction\n"
           "    trace\n"
           "      run one step and dump context until HLT instruction\n"
           , prog);
}

int main(int argc, const char * const argv[])
{
    if(argc < 3) {
        usage(argv[0]);
    } else if(argc >= 3) {
        if(strcmp(argv[1], "trace") == 0) {
            run_file(argv[2], 1);
        } else if(strcmp(argv[1], "run") == 0) {
            run_file(argv[2], 0);
        } else {
            printf("Unknow command %s\n", argv[1]);
        }
    }
}
