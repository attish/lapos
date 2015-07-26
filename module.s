bits 32

    mov eax, 0xf0000004
    mov [eax], byte 3
    hlt
