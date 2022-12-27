# Multiboot information

I started this document to store information that would be
too long for C code comments regarding the boot process.

Thank God for Multiboot, it makes hobby OS developers' lives much
easier, no need to tinker with a bootloader and use BIOS calls to
manipulate drives -- been there, did that, had some fun, but don't
regret not having to go back. GRUB is an OS in itself, able to
access drives, read filesystems, handle the keyboard and the
framebuffer. QEMU also supports Multiboot, which means that the VM is
started with the kernel right in place, and is also given some clues
about the situation.

I want to maintain bootability on real hardware forever. Which means
that I have to keep an eye on possible changes with both QEMU and GRUB
implementations of Multiboot to find possible breaks.

In 2022, I noticed that lapos no longer boots on the GRUB included
with Debian 11, but the reboot (probably a general protection fault)
only happens when no module is loaded. This turned out to be an issue
with the way GRUB handles this case -- it seems that in QEMU, the
modules block in the Multiboot structure is missing if there are no
modules present, while in GRUB, the structure is there, and it shows a
module count of 0. This means that it is an error to suppose that
flag bit 3 can be used to find out if there are any modules loaded
with the kernel. What is misleading about this is that this approach
works in QEMU, but must not be relied on.

(Actually while fixing this issue, I found another subtle bug that had
been lurking around. See commit 26cd57e for details.)

