// The built-in compiler was here — in the full version, 079 can build and run code on its own.

// I've decided to pull the compiler logic for now.
// It was supposed to be a core feature of this release,
// - but I'm currently hitting some technical roadblocks that make it unusable.
// Instead of shipping crap, I'm taking time to fix it properly. Expect a working version in a future update.

#include "compiler.h"

bool assemble_x86(const char*, CompileResult* result) {
    result->success = false;
    result->code_size = 0;
    return false;
}

bool compile_c(const char*, CompileResult* result) {
    result->success = false;
    result->code_size = 0;
    return false;
}

int execute_program(CompileResult*) {
    return -1;
}
