#include <stdint.h>

void kputch(unsigned char ch)
{
    volatile unsigned char *videoram = (unsigned char *)0xB8000;
    
    videoram[0] = ch;
    videoram[1] = 0x07;
}
 
void kmain(void)
{
    extern uint32_t magic;
 
    // The multiboot header
    //extern void *mbd;
 
    if ( magic != 0x2BADB002 )
    {
        /* Something went not according to specs. Print an error */
        /* message and halt, but do *not* rely on the multiboot */
        /* data structure. */
    }
 
   /* You could either use multiboot.h */
   /* (http://www.gnu.org/software/grub/manual/multiboot/multiboot.html#multiboot_002eh) */
   /* or do your offsets yourself. The following is merely an example. */ 
    // char *boot_loader_name =(char*) ((long*)mbd)[16];

    unsigned char x;
    while (1)
    {
        kputch(x);
        x++;
    }
}
