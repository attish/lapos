NASM=nasm
NASMFLAGS=-f elf
CC=gcc
CFLAGS=-Wall -Wextra -Werror -nostdlib \
        -fno-builtin -nostartfiles -nodefaultlibs
LD=ld
LDFLAGS=-T linker.ld
LDFLAGS_MOD=-T module.ld
QEMU=qemu-system-i386
MODULES=module
OBJCOPY=objcopy
OC_FLAGS=-O binary

all: kernel

run: kernel module
	$(QEMU) -initrd $(MODULES) -kernel $<

module.o: module.s
	$(NASM) $(NASMFLAGS) -o $@ $<

modmain.o: modmain.c
	$(CC) $(CFLAGS) -o $@ -c $<

module.elf: module.o modmain.o
	$(LD) $(LDFLAGS_MOD) -o $@ $^

module: module.elf
	$(OBJCOPY) $(OC_FLAGS) $< $@

run-nomodules: kernel
	$(QEMU) -kernel $<

kernel: loader.o kernel.o
	$(LD) $(LDFLAGS) -o $@ $^

kernel.o: kernel.c
	$(CC) $(CFLAGS) -o $@ -c $<

loader.o: loader.s
	$(NASM) $(NASMFLAGS) -o $@ $<

clean: FRC
	rm -f kernel.o loader.o kernel module.o modmain.o module.elf module

FRC:
    
