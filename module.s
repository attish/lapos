bits 32
org 0x80000000

mod_entry:
    mov eax, 0xf0000004
    mov ebx, 0
display_loop:
    mov [eax], bl
    inc bl
    jmp display_loop
    hlt
