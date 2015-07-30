bits 32
global mod_entry
extern modmain

mod_entry:
    mov eax, 0xf0000004
    mov ebx, 0
    jmp modmain
