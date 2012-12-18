#include <stdint.h>

#define NULL (void *) 0x0
#define VGA_COLS (80);
#define VGA_ROWS (80);

typedef unsigned int int32;

unsigned int cursor_x = 0;
unsigned int cursor_y = 0;

void outb(unsigned short int port, char data)
{
   asm("outb %%al, %%dx;" : : "d"(port), "a"(data));
}

unsigned char inb(unsigned short int port)
{
    unsigned char ret;
    asm ("inb %1, %0;" : "=a"(ret) : "Nd"(port));
    return ret;
}

void update_cursorpos_to_vga()
{
    unsigned short int port = 0x3D4;
    unsigned char data = 0x0E;
    unsigned int position = cursor_y * 80 + cursor_x;
    outb(port, data);
    port = 0x3D5;
    data = (position >> 8) & 0xFF;
    outb(port, data);
    port = 0x3D4;
    data = 0x0F;
    outb(port, data);
    port = 0x3D5;
    data = position & 0xFF;
    outb(port, data);
}

void update_cursorpos_from_vga()
{
    unsigned short int offset;
    unsigned short int port_3D4 = 0x3D4;
    unsigned short int port_3D5 = 0x3D5;
    outb(port_3D4, 14);
    offset = inb(port_3D5) << 8;
    outb(port_3D4, 15);
    offset |= inb(port_3D5);

    cursor_x = offset % 80;
    cursor_y = offset / 80;
}    
     
void kputch(unsigned char ch)
{
    volatile unsigned char *videoram = (unsigned char *)0xB8000;
    unsigned int target = cursor_y * 80 + cursor_x;

    if (ch != '\n')
    {
        videoram[target << 1] = ch;
        videoram[(target << 1) + 1] = 0x07;
        cursor_x ++;
    } else {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_x > (80 - 1))
    {
        cursor_y += cursor_x / VGA_COLS;
        cursor_x = cursor_x % VGA_COLS;
    }

    if (cursor_y > 24)
    {
        // TODO scroll
        cursor_x = cursor_y = 0;
    }

    update_cursorpos_to_vga();
}

void kputs(char *s)
{
    for (; *s!=0; s++)
        kputch(*s);
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

    update_cursorpos_from_vga();

    kputs("Hello world!\n");
    int i;
    // TODO kputd
    char* numstr[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                  "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
                  "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
                  };
    for (i=0; i < 24; i++)
    {
        kputs(numstr[i]);
        kputch('\n');
    }
}
