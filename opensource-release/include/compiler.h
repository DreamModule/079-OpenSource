#ifndef COMPILER_H
#define COMPILER_H

#include "types.h"

#define MAX_CODE_SIZE 2048
#define MAX_OUTPUT_SIZE 256
#define MAX_LABELS 64
#define MAX_FIXUPS 64

struct CompileResult {
    uint8_t code[MAX_CODE_SIZE];
    int code_size;
    char output[MAX_OUTPUT_SIZE];
    int output_size;
    bool success;
    char error[64];
    int error_line;
};


bool assemble_x86(const char* source, CompileResult* result);


bool compile_c(const char* source, CompileResult* result);


int execute_program(CompileResult* result);

#endif
