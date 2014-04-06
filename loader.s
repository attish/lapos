global loader                           ; making entry point visible to linker
global magic                            ; we will use this in kmain
global mbi                              ; we will use this in kmain
 
extern kmain                            ; kmain is defined in kernel.c
 
; setting up the Multiboot header - see GRUB docs for details
MODULEALIGN equ  1<<0                   ; align loaded modules on page boundaries
MEMINFO     equ  1<<1                   ; provide memory map
FLAGS       equ  MODULEALIGN            ; this is the Multiboot 'flag' field
    ; The detailed memory map is not needed yet
    ; (all we need is the size of available memory)
MAGIC       equ  0x1BADB002             ; 'magic number' lets bootloader find the header
CHECKSUM    equ -(MAGIC + FLAGS)        ; checksum required
 
section .text
 
align 4
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
