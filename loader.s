global loader                           ; making entry point visible to linker
global magic                            ; we will use this in kmain
global mbi                              ; we will use this in kmain
global first_memblock
global multiboot_header
global entry_eip
global testisr_asm
global testisr_c
 
extern kmain                            ; kmain is defined in kernel.c
extern test_isr
 
; setting up the Multiboot header - see GRUB docs for details
MODULEALIGN equ  1<<0                   ; align modules on page boundaries
MEMINFO     equ  1<<1                   ; provide memory map
FLAGS       equ  MODULEALIGN | MEMINFO  ; this is the Multiboot 'flag' field
MAGIC       equ  0x1BADB002             ; lets bootloader find the header
CHECKSUM    equ - (MAGIC + FLAGS)       ; checksum required
GDT_BASE    equ  0x400                 ; GDT is placed right above real mode int table
GDTR_BASE   equ  0x500
IDT_BASE    equ  0x0
 
section .text
 
align 4
multiboot_header:
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; reserve initial kernel stack space
STACKSIZE equ 0x4000                    ; that's 16k.
 
loader:
    mov  esp, stack + STACKSIZE         ; set up the stack
    mov  [magic], eax                   ; Multiboot magic number
    mov  [mbi], ebx                     ; Multiboot info structure

    call continue
continue:
    pop eax
    mov [entry_eip], eax

    cld
    mov edi, GDT_BASE
    mov eax, 0x0                        ; null descriptor
    stosd
    stosd
    mov eax, 0x0000ffff                  ; code descriptor
    stosd
    mov eax, 0x00df9b00
    stosd
    mov eax, 0x0000ffff                  ; data descriptor
    stosd
    mov eax, 0x00df9300
    stosd

    mov edi, GDTR_BASE
    mov eax, 24
    stosw
    mov eax, GDT_BASE
    stosd

    lgdt [GDTR_BASE]
    jmp 0x8:new_gdt
new_gdt:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov al, 3
    mov [0xb8000], al

    mov edi, GDTR_BASE                  ; re-use GDTR as IDTR
    mov eax, 256*8                      ; 256*8
    stosw
    mov eax, IDT_BASE
    stosd

    lidt [GDTR_BASE]

    call kmain                          ; call kernel proper
 
    cli
.hang:
    hlt                                 ; halt machine should kernel return
    jmp  .hang

testisr_asm:
    pushad
    mov ah, [0xb8010]
    inc ah
    mov [0xb8010], ah
    in al, 0x60
    mov [0xb8012], al
    mov al, 0x20
    out 0x20, al
    popad
    iret

testisr_c:
    pushad
    call test_isr
    popad
    iret

section .bss
 
align 4
stack: resb STACKSIZE                   ; reserve 16k stack on a doubleword boundary
magic: resd 1
mbi:   resd 1
entry_eip: resd 1

section .memblock

align 4
first_memblock:
    dd 0    ; prev_header
    dd 0    ; next_header
    dd 1    ; free

