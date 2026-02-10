



[BITS 16]
[ORG 0x7C00]

start:
    
    mov [boot_drive], dl

    
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    
    mov ax, 0x0003
    int 0x10

    
    mov si, msg_boot
    call print_string

    
    xor ah, ah
    mov dl, [boot_drive]
    int 0x13

    
    
.load_loop:
    cmp word [sectors_left], 0
    je .load_done

    
    mov ax, [sectors_left]
    cmp ax, 64
    jbe .cnt_ok
    mov ax, 64
.cnt_ok:
    mov [dap_count], ax

    
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jnc .chunk_ok

    
    mov ah, 0x42
    mov dl, 0x80
    mov si, dap
    int 0x13
    jnc .chunk_ok

    
    cmp word [dap_segment], 0x1000
    jne .error

    mov ah, 0x02
    mov al, 64
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, 0x1000
    mov es, bx
    xor bx, bx
    int 0x13
    jnc .chunk_ok

    
    mov ah, 0x02
    mov al, 64
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x80
    mov bx, 0x1000
    mov es, bx
    xor bx, bx
    int 0x13
    jnc .chunk_ok

.error:
    mov si, msg_error
    call print_string
    jmp $

.chunk_ok:
    mov ax, [dap_count]
    sub [sectors_left], ax

    
    shl ax, 5
    add [dap_segment], ax

    
    mov ax, [dap_count]
    add [dap_lba], ax

    jmp .load_loop

.load_done:
    mov si, msg_loading
    call print_string

    
    xor ax, ax
    mov ds, ax
    xor cx, cx
    xor dx, dx
    mov ax, 0xE801
    int 0x15
    jc .try_88h

    test cx, cx
    jnz .use_cx_dx
    mov cx, ax
    mov dx, bx
.use_cx_dx:
    mov [0x500], cx     
    mov [0x502], dx     
    jmp .mem_done

.try_88h:
    mov ah, 0x88
    int 0x15
    jc .no_mem
    mov [0x500], ax
    mov word [0x502], 0
    jmp .mem_done
.no_mem:
    mov word [0x500], 0
    mov word [0x502], 0
.mem_done:

    
    in al, 0x92
    or al, 2
    out 0x92, al

    
    cli
    lgdt [gdt_descriptor]

    
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    
    jmp 0x08:protected_mode


print_string:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    xor bh, bh
    int 0x10
    jmp .loop
.done:
    popa
    ret


dap:
    db 0x10         
    db 0            
dap_count:
    dw 64           
    dw 0x0000       
dap_segment:
    dw 0x1000       
dap_lba:
    dd 1            
    dd 0            


boot_drive:     db 0x80
sectors_left:   dw 200


gdt_start:
    dq 0            

gdt_code:
    dw 0xFFFF       
    dw 0x0000       
    db 0x00         
    db 10011010b    
    db 11001111b    
    db 0x00         

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32]
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x400000   

    
    jmp 0x10000


msg_boot:      db 'QUICKS OPERATING SYSTEM', 13, 10, 'INITIALIZING...', 13, 10, 0
msg_loading:   db 'Loading kernel...', 13, 10, 0
msg_error:     db 'Disk read error!', 13, 10, 0


times 510-($-$$) db 0
dw 0xAA55
