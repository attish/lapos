NASM=nasm
NASMFLAGS=-f elf
CC=gcc
CFLAGS=-Wall -Wextra -Werror -nostdlib \
        -fno-builtin -nostartfiles -nodefaultlibs
LD=ld
LDFLAGS=-T linker.ld
QEMU=qemu-system-i386
MODULES=module

all: kernel

run: kernel module
	$(QEMU) -initrd $(MODULES) -kernel $<

module: module.s
	$(NASM) -o $@ $<

run-nomodules: kernel
	$(QEMU) -kernel $<

kernel: loader.o kernel.o
	$(LD) $(LDFLAGS) -o $@ $^

kernel.o: kernel.c
	$(CC) $(CFLAGS) -o $@ -c $<

loader.o: loader.s
	$(NASM) $(NASMFLAGS) -o $@ $<

clean: FRC
	rm -f kernel.o loader.o kernel

FRC:
    
