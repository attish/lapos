// vi:cin:sw=4 ts=4:foldmethod=marker:foldmarker={{{,}}}

// Includes {{{

#include <stdint.h>

// }}}

// Constants and macros {{{

#define VERSION_NUMBER 0x0
#define NULL (void *) 0x0
#define VGA_COLS 80
#define VGA_ROWS 25
#define NL kputch('\n')
#define PRINT(X)    kputs((#X)); kputs(" == "); kputx((X)); NL; 
#define PRINTD(X)   kputs((#X)); kputs(" == "); kputd((X)); NL; 
#define PRINTPTR(X) kputs((#X)); kputs(" == "); kputx((int32)(X)); NL; 
#define IGNORE_UNUSED(X)   (X) = (X);

// }}}

// Type definitions {{{

typedef unsigned int int32;
typedef unsigned int bool;
typedef unsigned int bit; // for bitfields

typedef struct multiboot_info {
    unsigned long flags;
    unsigned long mem_lower;
    unsigned long mem_upper;
    unsigned long boot_device;
    unsigned long cmdline;
    unsigned long mods_count;
    unsigned long mods_addr;
} multiboot_info_t;

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

typedef struct memblock_header_s memblock_header_t;

struct memblock_header_s {
    memblock_header_t *prev_header;
    memblock_header_t *next_header;
    bool free;
};

// }}}

// Function declarations {{{

void  kputch(char);
void  kputs(char *);
void  kputx(unsigned int);
void  kputb(unsigned int);
void  kputd(unsigned int);
void  kputh(unsigned int);
void *kmalloc(unsigned int);
int   kfree(void *);
void  walk_heap(memblock_header_t *);
int   kmem_available();

// }}}

// Global variables {{{

int current_pagetable_set = 1;
page_directory_entry_t __attribute__((aligned(4096))) page_directory[2][1024];
page_table_entry_t __attribute__((aligned(4096))) page_tables[2][1024][1024];

memblock_header_t *first_header;
unsigned int max_address = 0;   // This is the address of the last
                                // valid byte of memory

unsigned int cursor_x = 0;
unsigned int cursor_y = 0;

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

// }}}

// Function definitions {{{

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
        pde.page_table = address >> 12;
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

    kputs("Page tables filled."); NL;
}

void build_pagetables(int32 mapping_count, mem_mapping *map, int identity) {
    // TODO use kmalloc when it is done
    current_pagetable_set = !current_pagetable_set;

    // Debug
    int32 stat_identity = 0;
    int32 stat_null = 0;
    int32 stat_mapped = 0;

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
        pde.page_table = address >> 12;
        page_directory[current_pagetable_set][curr_entry] = pde;
    }

    kputs("Page directory filled."); NL;
    
    int32 memory = 0;
    int32 end;  // The last page to map in this iteration
    mem_mapping *curr_mapping = map;
    while (1) {
        if (curr_mapping < map + mapping_count)
            end = curr_mapping->source_start - 1;
        else
            end = 0xfffff000; // hope I got it right
        while (memory < end) {
            int32 pde = (memory & 0xffc00000) >> 22;
            int32 pte = (memory & 0x003ff000) >> 12;
            if (identity) {
                stat_identity++;
                page_tables[current_pagetable_set][pde][pte] =
                    map_page_to_target(memory);
            } else {
                stat_null++;
                page_tables[current_pagetable_set][pde][pte] =
                    null_map_page();
            }
            if (memory == 0xfffff000) break;
            memory += 4096;
        }
        int32 target = curr_mapping->target_start;
        while (memory <= curr_mapping->source_end) {
            int32 pde = (memory & 0xffc00000) >> 22;
            int32 pte = (memory & 0x003ff000) >> 12;
            stat_mapped++;
            page_tables[current_pagetable_set][pde][pte] =
                map_page_to_target(target);
            if (memory == 0xfffff000) break;
            memory += 4096;
            target += 4096;
        }
        if (memory == 0xfffff000) break;
        curr_mapping++;
    }
    kputs("Page tables ready."); NL;
    kputs("Null mappings: "); kputx(stat_null); NL;
    kputs("Identity mappings: "); kputx(stat_identity); NL;
    kputs("Specified mappings: "); kputx(stat_mapped); NL;
}

void enable_paging() {
    kputs("Loading CR3..."); NL;
    asm volatile("mov %0, %%cr3":: "b"(page_directory[current_pagetable_set]));
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

void kputd(unsigned int d) {
    // The only thing this function aims to do is the simplest
    // possible way of displaying an unsigned decimal integer, without
    // any leading zeros, alignment etc.

    char *decdigits = "0123456789";
    int d_place = 1000000000;
    int leading_zero = 1;

    while (d_place) {
        int digit = d / d_place;
        if (digit || !leading_zero || (d_place == 1))
            kputch(decdigits[d / d_place]);
        if (digit) leading_zero = 0;
        d %= d_place;
        d_place /= 10;
    }
}

void kputh(unsigned int d) {
    // "Human" printing. This is what some Unix commands do on the
    // '-h' switch. Eg. 16400 -> "16k"

    char postfix[] = "GMk "; // 0: G, 1: M, 2: k, 3: none
    int magnitude = 0;

    for (magnitude = 0; magnitude < 4; magnitude++) {
        if (d >= 1U<<(30 - 10 * magnitude))
            break;
    }

    kputd(d / (1U<<(30 - 10 * magnitude)));
    kputch(postfix[magnitude]);
}

void kputx(unsigned int d) {
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

void kputb(unsigned int d) {
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

int32 kmem_block_size(memblock_header_t *header) {
    if (!header->next_header)
        // There is a slight trick here: this should be
        // (max_address + 1 - header) / 4096 - 1; however adding that 1 always
        // gives the address of the next page, hence it actually increases the
        // page count by 1, which is cancelled out by the decrement, which is
        // a correction for the first page occupied by the header
        return (max_address - (int32)header) / 4096;
    return ((int32)header->next_header - (int32)header) / 4096 - 1;
}

void *kmalloc(unsigned int pages) {
    // First, find a free block that is large enough
    memblock_header_t *current_header = first_header;
    int32 size;

    kputs("kmalloc() called to allocate: "); kputd(pages); NL;

    while (current_header) {
        if (current_header->free) {
            size = kmem_block_size(current_header);
            kputs("Found free block, size: "); kputd(size); NL;
            if (pages <= size)
                break;
        }
        current_header = current_header->next_header;
    }

    // Should normally reach the end of the loop when out of memory
    if (!current_header)
        return NULL;

    kputs("Matching block found. Header at ");
    kputx((int32)current_header); NL;

    // Is the block size exactly the same as the requested memory?
    // Or one page larger?

    if (size - pages <= 1) {
        // If yes, simply mark as used and return
        kputs("Exact match. Marking block as used and return."); kputx((int32)first_header); NL;
        current_header->free = 0;
        return (void*)((int32)current_header + 4096);
    } else {
        // Else the block needs to be bisected
        memblock_header_t *new_header;


        new_header = (memblock_header_t*)((int32)current_header +
                (pages + 1) * 4096);
        kputs("Bisecting block. ");
        PRINTPTR(new_header)

        new_header->next_header = current_header->next_header;
        current_header->next_header = new_header;
        new_header->prev_header = current_header;
        current_header->free = 0;
        new_header->free = 1;
        //kputs("free_header after creating new block: ");
        //kputx((int32)first_header); NL;
    }

    return (void*)((int32)current_header + 4096);
}

int kfree(void *memory) {
    // First, we need a pointer to the header
    memblock_header_t *header = memory - 4096;

    // Freeing a block means marking it as free...
    header->free = 1;

    // ...and joining it with any neighboring free blocks
    if (header->prev_header && header->prev_header->free) {
        kputs("Previous block is free, joining.");
        header->prev_header->next_header = header->next_header;
        header->next_header->prev_header = header->prev_header;
        header = header->prev_header;
    }

    if (header->next_header && header->next_header->free) {
        header->next_header = header->next_header->next_header;
        header->next_header->prev_header = header;
    }

    // Basically, this function cannot fail -- it can only do nonsense
    return 1;
}


void walk_heap(memblock_header_t *start) {
    memblock_header_t *current_header = start;
    
    while (current_header) {
        kputs("prev: "); kputx((int32)current_header->prev_header);
        kputs(" block: "); kputx((int32)current_header);
        kputs(" next: "); kputx((int32)current_header->next_header);
        kputs(" size: "); kputd(kmem_block_size(current_header)); 
        kputs(" ("); kputh(kmem_block_size(current_header) * 4096); 
        kputs(") free: "); kputd((int32)current_header->free);
        NL;

        current_header = current_header->next_header;
    }
}

int kmem_available() {
    // Return amount of memory left to the kernel in blocks
    // (not the largest continuous block!)
    int count = 0;
    memblock_header_t *current_header = first_header;

    while (current_header) {
        if (current_header->free)
            count += kmem_block_size(current_header);
        current_header = current_header->next_header;
    }

    return count;
}

// }}}

// Tests {{{

void test_kmalloc_1() {
    kputs("Heap before allocation"); NL; walk_heap(first_header);
    PRINT((int32)first_header)

    kputs("Allocating blocks"); NL;
    int *m1 = kmalloc(15); IGNORE_UNUSED(m1);
    int *m2 = kmalloc(15); IGNORE_UNUSED(m2);
    int *m3 = kmalloc(15); IGNORE_UNUSED(m3);
    kputs("Heap after allocation"); NL; walk_heap(first_header);
    kfree(m2);
    kputs("Heap after freeing 2"); NL; walk_heap(first_header);
    kfree(m1);
    kputs("Heap after freeing 1"); NL; walk_heap(first_header);
    kfree(m3);
    kputs("Heap after freeing 3"); NL; walk_heap(first_header);
}

void test_kmalloc_2() {
    kputs("Heap before allocation"); NL; walk_heap(first_header);
    PRINT((int32)first_header)

    kputs("Allocating blocks"); NL;
    int *m0 = kmalloc(1); IGNORE_UNUSED(m0);
    int *m1 = kmalloc(15); IGNORE_UNUSED(m1);
    int *m2 = kmalloc(15); IGNORE_UNUSED(m2);
    int *m3 = kmalloc(15); IGNORE_UNUSED(m3);
    int *m4 = kmalloc(15); IGNORE_UNUSED(m4);
    int *m5 = kmalloc(15); IGNORE_UNUSED(m5);
    int *m6 = kmalloc(15); IGNORE_UNUSED(m6);
    kputs("Heap after initial allocation"); NL; walk_heap(first_header);
    kfree(m2);
    kputs("Heap after freeing 2"); NL; walk_heap(first_header);
    kfree(m4);
    kfree(m5);
    kputs("Heap after freeing 4&5"); NL; walk_heap(first_header);
    int *m7 = kmalloc(19); IGNORE_UNUSED(m7);
    kputs("Heap after allocating 19 blocks"); NL; walk_heap(first_header);
    int *m8 = kmalloc(8); IGNORE_UNUSED(m8);
    int *m9 = kmalloc(8); IGNORE_UNUSED(m9);
    kputs("Heap after allocating 2*8 blocks"); NL; walk_heap(first_header);
    int *m10 = kmalloc(16); IGNORE_UNUSED(m10);
    kputs("Heap after allocating 16 blocks"); NL; walk_heap(first_header);
}

void test_kmalloc_3() {
    kputs("Heap before allocation"); NL; walk_heap(first_header);
    PRINT((int32)first_header)

    kputs("Allocating blocks"); NL;
    int *m0 = kmalloc(32505); IGNORE_UNUSED(m0);
    if (!m0) {
        kputs("Out of memory"); NL;
    }
    kputs("Heap after allocating 32505 blocks"); NL; walk_heap(first_header);
}

/// }}}

// Main entry point {{{

void kmain(void) {
    extern uint32_t magic;
    extern multiboot_info_t *mbi; // The multiboot header
    extern int32 first_memblock;

    if (magic != 0x2BADB002)
    {
        kputs("Multiboot magic number mismatch! Freezing.");
        for (;;);
    }

    update_cursorpos_from_vga();

    kputs("LapOS v");
    kputd(VERSION_NUMBER);
    kputs("\n\n");

    kputs("Memory size: ");
    kputh((unsigned int) mbi->mem_upper * 1024); NL; NL;
    max_address = (mbi->mem_upper + 1024) * 1024 - 1;

    mem_mapping mapping[] = {
        {
            0xc0000000,
            0xc0001000,
            0x000b8000
        },
        {
            0xd0000000,
            0xd0001000,
            0x000b8000
        }
    };

    build_pagetables(2, mapping, 1);
    //build_identity_mapping();
    enable_paging();
    kputs("Paging enabled.\n");
    char *testvideo = (char *) 0xc0000000;
    testvideo[0] = 1;
    testvideo = (char *) 0xd0000000;
    testvideo[2] = 2;

    first_header = (memblock_header_t *) &first_memblock;
    max_address = (mbi->mem_upper + 1024) * 1024 - 1;

    kputs("Kernel memory free: ");
    int freemem = kmem_available();
    kputh(freemem * 4096);
    kputs(" ("); kputd(freemem); kputs(" pages)"); NL;
}

// }}}
