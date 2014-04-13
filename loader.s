global loader                           ; making entry point visible to linker
global magic                            ; we will use this in kmain
global mbi                              ; we will use this in kmain
global first_memblock
global multiboot_header
 
extern kmain                            ; kmain is defined in kernel.c
 
; setting up the Multiboot header - see GRUB docs for details
MODULEALIGN equ  1<<0                   ; align modules on page boundaries
MEMINFO     equ  1<<1                   ; provide memory map
FLAGS       equ  MODULEALIGN | MEMINFO  ; this is the Multiboot 'flag' field
MAGIC       equ  0x1BADB002             ; lets bootloader find the header
CHECKSUM    equ - (MAGIC + FLAGS)       ; checksum required
 
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
 
    call kmain                          ; call kernel proper
 
    cli
.hang:
    hlt                                 ; halt machine should kernel return
    jmp  .hang
 
section .bss
 
align 4
stack: resb STACKSIZE                   ; reserve 16k stack on a doubleword boundary
magic: resd 1
mbi:   resd 1

section .memblock

align 4
first_memblock:
    dd 0    ; prev_header
    dd 0    ; next_header
    dd 1    ; free

