#include <stdint.h>

// Constants {

#define VERSION_NUMBER 0x0
#define NULL (void *) 0x0
#define VGA_COLS 80
#define VGA_ROWS 25
#define NL kputch('\n')
#define PRINT(X)    kputs((#X)); kputs(" == "); kputx((X)); NL; 

// }

// Type definitions {

typedef unsigned int int32;
typedef unsigned int bit; // for bitfields

typedef struct {
    int32 source_start;
    int32 source_end;
    int32 target_start;
} mem_mapping;

typedef union {
    struct {
        bit present: 1;
        bit writable: 1;
        bit supervisor: 1;
        bit write_through: 1;
        bit cache_disabled: 1;
        bit accessed: 1;
        bit :1;
        bit large_page: 1;
        bit ignored: 1;
        bit available: 3;
        bit page_table: 20;
    };
    int32 value;
} page_directory_entry_t;

typedef union {
    struct {
        bit present: 1;
        bit writable: 1;
        bit supervisor: 1;
        bit write_through: 1;
        bit cache_disabled: 1;
        bit global: 1;
        bit dirty:1;
        bit :1;
        bit ignored: 1;
        bit available: 3;
        bit page_address: 20;
    };
    int32 value;
} page_table_entry_t;

// }

// Function declarations {
void kputch(char);
void kputs(char *);
void kputx(int);
void kputb(int);
// }

// Global variables {
int current_pagetable_set = 1;
page_directory_entry_t __attribute__((aligned(4096))) page_directory[2][1024];
page_table_entry_t __attribute__((aligned(4096))) page_tables[2][1024][1024];

unsigned int cursor_x = 0;
unsigned int cursor_y = 0;

// }

page_table_entry_t null_map_page() {
    // TODO check page alignment
    page_table_entry_t pte;
    pte.value = 0;
    pte.present = 0;
    pte.writable = 1;
    pte.supervisor = 1;
    return pte;
}

page_table_entry_t map_page_to_target(int32 target) {
    // TODO check page alignment
    page_table_entry_t pte;
    pte.value = 0;
    pte.present = 1;
    pte.writable = 1;
    pte.supervisor = 1;
    pte.page_address = target >> 12;
    return pte;
}

void build_identity_mapping() {
    current_pagetable_set = !current_pagetable_set;

    // create generic page directory
    // TODO factor out
    int curr_entry;
    for (curr_entry = 0; curr_entry < 1024; curr_entry++) {
        page_directory_entry_t pde;
        pde.value = 0;
        pde.present = 1;
        pde.writable = 1;
        pde.supervisor = 1;
        int address = (int)&(page_tables[current_pagetable_set][curr_entry]);
        //kputx(address); NL;
        pde.page_table = address >> 12;
        //kputb(pde.value); NL;
        page_directory[current_pagetable_set][curr_entry] = pde;
    }

    kputs("Page directory filled."); NL;

    int page;
    for (page = 0; page < 1024 * 1024; page++) {
        int32 memory = page * 4096;
        int32 pde = (memory & 0xffc00000) >> 22;
        int32 pte = (memory & 0x003ff000) >> 12;
        page_tables[current_pagetable_set][pde][pte] =
            map_page_to_target(memory);
    }
    kputs("map 0x0 == ");
    kputx(map_page_to_target(0x0).value); NL;


    // 0x12345678
    // pde = 72, pte = 837
    PRINT(page_directory[current_pagetable_set][72].value)
    PRINT((int)page_tables[current_pagetable_set][72])
    PRINT(page_tables[current_pagetable_set][72][837].value)

    kputs("Page tables filled."); NL;
}

void build_pagetables(int32 mapping_count, mem_mapping *map,
                      int identity_mapping) {
    // TODO use kmalloc when it is done
    mem_mapping *curr_mapping;

    current_pagetable_set = !current_pagetable_set;

    // TODO zero out page_directory and page_tables
    // TODO setting up page_directory does not depend on the mapping, so it
    //      can be done here
    if (identity_mapping)
    {
        // if identity mapping is set, initialize tables for identity mapping,
        // then add the requested modifications here
    }

    for (curr_mapping = map; curr_mapping < (map + mapping_count); map++)
    {
        //unsigned int curr_source_start = curr_mapping->source_start;
        // TODO check start alignment
        // TODO find appropriate first page_table and the correct entry in it
    }
}

void enable_paging() {
    kputs("Loading CR3..."); NL;
    //moves page_directory (which is a pointer) into the cr3 register.
    asm volatile("mov %0, %%cr3":: "b"(page_directory[current_pagetable_set]));
    //reads cr0, switches the "paging enable" bit, and writes it back.
    unsigned int cr0;
    asm volatile("mov %%cr0, %0": "=b"(cr0));
    cr0 |= 0x80000000;
    kputs("Loading CR0..."); NL;
    asm volatile("mov %0, %%cr0":: "b"(cr0));
}

void outb(unsigned short int port, char data) {
   asm("outb %%al, %%dx;" : : "d"(port), "a"(data));
}

unsigned char inb(unsigned short int port) {
    unsigned char ret;
    asm ("inb %1, %0;" : "=a"(ret) : "Nd"(port));
    return ret;
}

void update_cursorpos_to_vga() {
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

void update_cursorpos_from_vga() {
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

void kputch(char ch) {
    volatile char *videoram = (char *)0xB8000;
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

    // End of line
    if (cursor_x > (80 - 1))
    {
        cursor_y += cursor_x / VGA_COLS;
        cursor_x = cursor_x % VGA_COLS;
    }

    // Screen is full -- scrolling
    if (cursor_y > 24)
    {
        int line, col;
        target = 0;
        for (line = 0; line < VGA_ROWS; line++)
            for (col = 0; col < VGA_COLS; col++) {
            videoram[target] = videoram[target + 160];
            target += 2;
            }
        cursor_y = VGA_ROWS - 1;
        cursor_x = 0;
    }

    update_cursorpos_to_vga();
}

void kputs(char *s) {
    for (; *s!=0; s++)
        kputch(*s);
}

//void kputd(int d) {
//}

void kputx(int d) {
    // always 32 bit for now
    char *hexdigits = "0123456789abcdef";
    int current = d;
    int count;

    kputs("0x");

    for (count = 0; count < 8; count++)
    {
        kputch(hexdigits[(current & 0xf0000000) >> 28]);
        current <<= 4;
    }
}

void kputb(int d) {
    // always 32 bit for now
    char *bindigits = "01";
    int current = d;
    int count;

    kputs("b");

    for (count = 0; count < 32; count++)
    {
        kputch(bindigits[(current & 0x80000000) >> 31]);
        current <<= 1;
    }
}

void kmain(void) {
    extern uint32_t magic;
 
    // The multiboot header
    //extern void *mbd;
    if ( magic != 0x2BADB002 )
    {
        /* Something went not according to specs. Print an error */
        /* message and halt, but do *not* rely on the multiboot */
        /* data structure. */
        kputs("Multiboot magic number mismatch! Freezing.");
        while (1);
    }
 
   /* You could either use multiboot.h */
   /* (http://www.gnu.org/software/grub/manual/multiboot/multiboot.html#multiboot_002eh) */
   /* or do your offsets yourself. The following is merely an example. */ 
    // char *boot_loader_name =(char*) ((long*)mbd)[16];

    update_cursorpos_from_vga();

    kputs("LapOS v");
    kputx(VERSION_NUMBER);
    kputs("\n\n");
    
//    build_pagetables(0, NULL, 0);
    build_identity_mapping();
    enable_paging();
    kputs("Paging enabled.\n");
}

