**LapOS** is a tiny C kernel for i386. Mainly just a personal experiment into ia32 protected mode and paging.

**Disclaimer:** this goals of this project fun and experimentation, please in no way regard anything done here as the "right" way to do it. **I am not a kernel developer.**

Another goal is to have as much of the code in C as possible, and to keep the amount of assembly code at a minimum.

# Getting Started

If you want to dive into kernelspace fun with LapOS, follow these steps on an x86 Ubuntu-based installation. Note: it is possible to develop on x64, but the setup is a bit more complicated.

You need `git`. Install `git`:

    laposdev@vanillamint ~ $ sudo apt install git-core

Then, clone the repo:

    laposdev@vanillamint ~ $ git clone https://github.com/attish/lapos.git
    laposdev@vanillamint ~ $ cd lapos

You need `nasm`. Install `nasm`.

    laposdev@vanillamint ~ $ sudo apt install nasm

Now you can compile.

    laposdev@vanillamint ~ $ make
    laposdev@vanillamint ~ $ ls kernel
    laposdev@vanillamint ~/lapos $ file ./kernel
    ./kernel: ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), statically linked, not stripped

You now have the kernel. It may be surprising that it is an ELF binary. This is only possible because of the Multiboot specification, which is supported by both GRUB and QEMU. Basically, Multiboot boots anything as long as it contains several magic signatures near the beginning.

To continue developing, you need an emulator. LapOS runs on physical hardware, but having to reboot to test the changes you made is tedious. Also you need not worry about harming the host system in any way. Install QEMU:

    laposdev@vanillamint ~/lapos $ sudo apt install qemu-system-x86

After this, a test run is as simple as:

    laposdev@vanillamint ~/lapos $ make run

Have fun, and use 

    laposdev@vanillamint ~/lapos $ make clean

to clean up your working directory when it becomes cluttered with .o files

