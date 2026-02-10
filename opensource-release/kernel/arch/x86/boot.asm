
[BITS 32]

global _start
extern kernel_main
extern __bss_start
extern __bss_end

section .text
_start:
    
    
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    shr ecx, 2             
    xor eax, eax
    cld
    rep stosd

    
    mov esp, kernel_stack_top

    
    call kernel_main

    
.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
kernel_stack_bottom:
    resb 65536      
kernel_stack_top:
