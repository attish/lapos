bits 32

    mov eax, 0xc0000004
    mov [eax], byte 3
    hlt
