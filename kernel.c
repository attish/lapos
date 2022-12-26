// vi:cin:sw=4:ts=4:et:foldmethod=marker:foldmarker={{{,}}}

// Includes {{{


//#include <stdint.h>
#include "version.h"


// }}}

// Constants and macros {{{

#define NULL (void *) 0x0

/* Video */
#define VGA_COLS 80
#define VGA_ROWS 25
#define NL kputch('\n')

/* PIC address constants */
#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xa0
#define PIC2_DATA 0xa1
#define PIC1_TARGET_OFFSET 0x20
#define PIC2_TARGET_OFFSET 0x28

/* Debugging macros */
#define HALT while(1)
#define PRINT(X)    kputs((#X)); kputs(" == "); kputx((X)); NL; 
#define PRINTXX(X)  kputs((#X)); kputs(" == "); kputxx((X)); NL; 
#define PRINTD(X)   kputs((#X)); kputs(" == "); kputd((X)); NL; 
#define PRINTH(X)   kputs((#X)); kputs(" == "); kputh((X)); NL; 
#define PRINTPTR(X) kputs((#X)); kputs(" == "); kputx((int32)(X)); NL; 
#define IGNORE_UNUSED(X)   (X) = (X);
#define MIN(X,Y)    ((((X))<((Y)))?((X)):((Y)))
#define MAX(X,Y)    ((((X))>((Y)))?((X)):((Y)))
#define NEXT_PAGE(X)       (((X) & 0xfffff000) + 0x1000)
#define PREV_PAGE(X)       (((X) & 0xfffff000) - 0x1000)

/* Compile switches */
#define DEBUG_PAGING
#define DEBUG_IDT
//#define DEBUG_MODULES_ALLOC

// }}}

// Type definitions {{{

typedef unsigned char int8;
typedef unsigned short int int16;
typedef unsigned int int32;
typedef unsigned long long int64;
typedef unsigned int bool;
typedef unsigned int bit; // for bitfields

struct multiboot_mmap_entry {
    int32 size;
    int64 addr;
    int64 len;
#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2
    int32 type;
} __attribute__((packed));

typedef struct multiboot_mmap_entry multiboot_memory_map_t;

typedef struct {
    unsigned long flags;
    unsigned long mem_lower;
    unsigned long mem_upper;
    unsigned long boot_device;
    unsigned long cmdline;
    unsigned long mods_count;
    unsigned long mods_addr;
    //multiboot_elf_section_header_table_t elf_sec;
    int32 elf_sec[4];
    int32 mmap_length;
    multiboot_memory_map_t *mmap_table;
} multiboot_info_t;

typedef struct {
    // From multiboot.h, reformatted and modified

    // the memory used goes from bytes 'mod_start' to 'mod_end-1' inclusive
    int32 mod_start;
    int32 mod_end;
    
    // Module command line
    // int32 cmdline;
    char* cmdline;
     
    // padding to take it to 16 bytes (must be zero)
    int32 padding;
} multiboot_module_table_t;

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

typedef union {
    struct {
        int16 offset_low: 16;
        int16 css: 16;
        int8  :8;
        bit gate_type: 4;
        bit storage: 1;
        bit privilege_level: 2;
        bit present: 1;
        int16 offset_hi: 16;
    };
    int64 value;
} idt_entry_t;

/*typedef union {
    struct {
        bit present: 1;
        bit privilege_level: 2;
        bit storage: 1;
        bit gate_type: 4;
    };
    int8 value;
} idt_entry_type_attr_t;

typedef struct {
    int16 offset_low;
    int16 css;
    int8  unused;
    idt_entry_type_attr_t type_attrib;
    int16 offset_hi;
} idt_entry_t;*/

typedef struct {
    int16 size;
    int16 unused;
    int32 offset;
} idtr_t;

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
void  kputxx(unsigned long long);
void  kputb(unsigned int);
void  kputd(unsigned int);
void  kputh(unsigned int);
void *kmalloc(unsigned int);
int   kfree(void *);
void  walk_heap(memblock_header_t *);
int   kmem_available();
void  make_idt_entry(unsigned int, void *);

void  testisr_asm();
void  testisr_c();

// }}}

// Global variables {{{

// TODO Page tables should be dynamically allocated to match the size of
// available memory. Using 8M of memory is a waste, but for now, we can live
// with it.

page_directory_entry_t __attribute__((aligned(4096))) page_directory[1024];
page_table_entry_t *page_table;

idt_entry_t *idt = (idt_entry_t *)0x0;

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

int mmap_len;
int mmap_count;
multiboot_memory_map_t *mmap_start;
multiboot_memory_map_t *current_mmap;
multiboot_module_table_t *mods;
int module_num;
int32 module_table_start;
int32 module_table_end;
int32 modules_start;
int32 modules_end;
int32 heap_start;

// }}}

// Function definitions {{{

void build_pagetables(int32 mapping_count, mem_mapping *map, int identity) {
    // FIXME Mappings starting at address 0 do not work!

    // Debug counters
#ifdef DEBUG_PAGING
    int32 count_identity = 0;
    int32 count_null = 0;
    int32 count_mapped = 0;
#endif

    // create generic page directory
    // TODO factor out
    int curr_entry;
    for (curr_entry = 0; curr_entry < 1024; curr_entry++) {
        page_directory_entry_t pde;
        pde.value = 0;
        pde.present = 1;
        pde.writable = 1;
        pde.supervisor = 1;
        int address = (int)(page_table + curr_entry * 1024);
        pde.page_table = address >> 12;
        page_directory[curr_entry] = pde;
    }

#ifdef DEBUG_PAGING
    kputs("Page directory filled."); NL;
#endif
    
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
#ifdef DEBUG_PAGING
                count_identity++;
#endif
                page_table[pde * 1024 + pte] =
                    map_page_to_target(memory);
            } else {
#ifdef DEBUG_PAGING
                count_null++;
#endif
                page_table[pde * 1024 + pte] =
                    null_map_page();
            }
            if (memory == 0xfffff000) break;
            memory += 4096;
        }
        int32 target = curr_mapping->target_start;
        while (memory <= curr_mapping->source_end) {
            int32 pde = (memory & 0xffc00000) >> 22;
            int32 pte = (memory & 0x003ff000) >> 12;
#ifdef DEBUG_PAGING
            count_mapped++;
#endif
            page_table[pde * 1024 + pte] =
                map_page_to_target(target);
            if (memory == 0xfffff000) break;
            memory += 4096;
            target += 4096;
        }
        if (memory == 0xfffff000) break;
        curr_mapping++;
    }
#ifdef DEBUG_PAGING
    kputs("Page tables ready."); NL;
    kputs("Null mappings: "); kputx(count_null); NL;
    kputs("Identity mappings: "); kputx(count_identity); NL;
    kputs("Specified mappings: "); kputx(count_mapped); NL;
#endif
}

void enable_paging() {
#ifdef DEBUG_PAGING
    kputs("Loading CR3..."); NL;
#endif
    asm volatile("mov %0, %%cr3":: "b"(page_directory));
    unsigned int cr0;
    asm volatile("mov %%cr0, %0": "=b"(cr0));
    cr0 |= 0x80000000;
#ifdef DEBUG_PAGING
    kputs("Loading CR0..."); NL;
#endif
    asm volatile("mov %0, %%cr0":: "b"(cr0));
}

void reload_pagetable() {
#ifdef DEBUG_PAGING
    kputs("Loading CR3..."); NL;
#endif
    asm volatile("mov %0, %%cr3":: "b"(page_directory));
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

void kputx_inner_32(unsigned int d) {
    char *hexdigits = "0123456789abcdef";
    int current = d;
    int count;

    for (count = 0; count < 8; count++)
    {
        kputch(hexdigits[(current & 0xf0000000) >> 28]);
        current <<= 4;
    }
}

void kputx(unsigned int d) {
    kputs("0x");
    kputx_inner_32(d);
}

void kputxx(unsigned long long dd) {
    // 64 bit
    kputs("0x");

    int32 d = (int32)(dd >> 32);
    kputx_inner_32(d);
    d = (int32)dd;  // truncate to 32 bits
    kputx_inner_32(d);
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

#ifdef DEBUG_KMALLOC
    kputs("kmalloc() called to allocate: "); kputd(pages); NL;
#endif

    while (current_header) {
        if (current_header->free) {
            size = kmem_block_size(current_header);
#ifdef DEBUG_KMALLOC
            kputs("Found free block, size: "); kputd(size); NL;
#endif
            if (pages <= size)
                break;
        }
        current_header = current_header->next_header;
    }

    // Should normally reach the end of the loop when out of memory
    if (!current_header)
        return NULL;

#ifdef DEBUG_KMALLOC
    kputs("Matching block found. Header at ");
    kputx((int32)current_header); NL;
#endif

    // Is the block size exactly the same as the requested memory?
    // Or one page larger?

    if (size - pages <= 1) {
        // If yes, simply mark as used and return
#ifdef DEBUG_KMALLOC
        kputs("Exact match. Marking block as used and return.");
        kputx((int32)first_header); NL;
#endif
        current_header->free = 0;
        return (void*)((int32)current_header + 4096);
    } else {
        // Else the block needs to be bisected
        memblock_header_t *new_header;


        new_header = (memblock_header_t*)((int32)current_header +
                (pages + 1) * 4096);
#ifdef DEBUG_KMALLOC
        kputs("Bisecting block. ");
        PRINTPTR(new_header)
#endif

        new_header->next_header = current_header->next_header;
        current_header->next_header = new_header;
        new_header->prev_header = current_header;
        current_header->free = 0;
        new_header->free = 1;
    }

    return (void*)((int32)current_header + 4096);
}

int kfree(void *memory) {
    // TODO validate pointer, ie. is really used? magic num. later?
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

void mark_heap_start_used_until(int32 used_end) {
    memblock_header_t *free_space = \
        (memblock_header_t *)NEXT_PAGE(used_end);
    first_header->free = 0;
    first_header->next_header = free_space;
    free_space->free = 1;
    free_space->prev_header = first_header;
    free_space->next_header = NULL;

}

void mark_modules_as_used() {
    // Multiboot gives us two data blocks that we need to exclude from the
    // heap. The specification does not state that these would have to be
    // contiguous, so we have to arrange for the exclusion separately. Also,
    // it is not stated which of them would be the first (though I guess few
    // bootloaders would place the table after the modules themselves).

#ifdef DEBUG_MODULES_ALLOC
    kputs("Header of first block: "); kputx(heap_start); NL;
    kputs("Module table: "); kputx(module_table_start);
    kputs(".."); kputx(module_table_end); NL;
    kputs("Modules: "); kputx(modules_start);
    kputs(".."); kputx(modules_end); NL;
    NL;
#endif

    int32 used_start_1 = MIN(module_table_start, modules_start);
    int32 used_start_2 = MAX(module_table_start, modules_start);
    int32 used_end_1 = MIN(module_table_end, modules_end);
    int32 used_end_2 = MAX(module_table_end, modules_end);

#ifdef DEBUG_MODULES_ALLOC
    kputs("First block: "); kputx(used_start_1);
    kputs(".."); kputx(used_end_1); NL;
    kputs("Second block: "); kputx(used_start_2);
    kputs(".."); kputx(used_end_2); NL;
#endif

    bool ignore_block_1 = (used_end_1 < heap_start);
    bool ignore_block_2 = (used_end_2 < heap_start);
    bool free_mem_before_1 = ignore_block_1 ? 0 : \
        ((used_start_1 - heap_start) > 0x2000);
    // TODO this badly needs a comment
    bool free_mem_before_2 = ignore_block_2 ? 0 : \
        ((used_start_2 - (ignore_block_1?heap_start:used_end_1)) > 0x2000);
#ifdef DEBUG_MODULES_ALLOC
    kputs("Is free mem before first table? "); kputd(free_mem_before_1); NL;
    kputs("Is free mem between tables? "); kputd(free_mem_before_2); NL;
    kputs("Is first block before heap start? "); kputd(ignore_block_1); NL;
    kputs("Is second block before heap start? "); kputd(ignore_block_2); NL;
#endif

    // Cases:
    // 1. Both tables are below kernel and ignored.
    // 2. Only the first table is below kernel and ignored.
    //    2.a The second table comes right after the first memblock header,
    //        so it can be used to mark the table, and another one to mark
    //        the free space.
    //    2.b There is free space between the first memblock header and the
    //        second table. This will be marked by the first memblock, and the
    //        second will mark the table. A third will mark the free mem.
    // 3. Both tables are after the kernel.
    //    3.a The first table comes right after the first memblock header, and
    //        the second table comes right after the first one. The two blocks
    //        are marked together with the first memblock, and a new one is
    //        used for free mem.
    //    3.b The first table comes after the memblock header, but there is
    //        free memory between the two blocks. Use first header for first
    //        block, and add new ones for the free block, the second block and
    //        the free mem.
    //    3.c There is free memory before the first block, but not between the
    //        two blocks. We use the first block to mark the free memory, then
    //        create another one for the two consecutive blocks together, and
    //        one for the free mem.
    //    3.d There is free memory both before and after the blocks. This is
    //        the most complicated case.
    //
    // I have no better idea ATM than to code for these cases individually.
    // The real ugly part will be that long and convoluted 'if' structure.

    // TODO I am lazy. Therefore I only implement those situations which
    // actually occur via using either GRUB's or QEMU's Multiboot
    // functionality. If a loader is used that works differently, the symptom
    // will most probably will be the kernel failing to find the modules.
    // In all other cases, fall back to the safest option,
    // which is:
    // - in case 2.b: ignore the free space, and handle it as 2.a
    // - in case 3.[bcd]: simply mark everything up to used_end_2 as used in
    // the first header, and create another one to mark the free mem. This is
    // again the same as 2.a
    
    if (ignore_block_1 && ignore_block_2) {
        // Case 1: GRUB does this without modules
        // This is NOP -- all mem is ours.
    } else if (ignore_block_1) {
        // Case 2
        if (!free_mem_before_2) {
            // Case 2.a: GRUB does this with modules
#ifdef DEBUG_MODULES_ALLOC
            kputs("I think this is GRUB!"); NL;
#endif
            mark_heap_start_used_until(used_end_2);
        } else {
            // Case 2.b
            kputs("Warning: unexpected Multiboot layout, using fallback.");
            mark_heap_start_used_until(used_end_2);
        }
    } else {
        // Case 3
        // The code is the same as with GRUB!
        if (!free_mem_before_1 && !free_mem_before_2) {
            // Case 3.a: this is what QEMU does ATM
#ifdef DEBUG_MODULES_ALLOC
            kputs("I think this is QEMU!"); NL;
#endif
            memblock_header_t *free_space = \
                (memblock_header_t *)NEXT_PAGE(used_end_2);
            first_header->free = 0;
            first_header->next_header = free_space;
            free_space->free = 1;
            free_space->prev_header = first_header;
            free_space->next_header = NULL;
        } else if (!free_mem_before_1 && free_mem_before_2) {
            // Case 3.b
            kputs("Warning: unexpected Multiboot layout, using fallback.");
            mark_heap_start_used_until(used_end_2);
        } else if (free_mem_before_1 && !free_mem_before_2) {
            // Case 3.c
            kputs("Warning: unexpected Multiboot layout, using fallback.");
            mark_heap_start_used_until(used_end_2);
        } else if (free_mem_before_1 && free_mem_before_2) {
            // Case 3.d
            kputs("Warning: unexpected Multiboot layout, using fallback.");
            mark_heap_start_used_until(used_end_2);
        }
    }

#ifdef DEBUG_MODULES_ALLOC
    // Print the module table

    kputs("Number of modules: "); kputd(module_num); NL;
    int n_mod;
    for (n_mod = 0; n_mod < module_num; n_mod++) {
        kputs("Module #"); kputd(n_mod);
        kputs(" start: "); kputx(mods[n_mod].mod_start);
        kputs(" end: "); kputx(mods[n_mod].mod_end);
        kputs(" size: "); kputh(mods[n_mod].mod_end-mods[n_mod].mod_start);
        kputs(" text: "); kputs(mods[n_mod].cmdline); NL;
    }
#endif

}

void make_idt_entry(unsigned int index, void *isr) {
    idt_entry_t *entry = idt + index;

    entry->value = 0;
    int16 isr_low =  ((int32) isr) & 0xffff;
    int16 isr_high = ((int32) isr) >> 16;

    //PRINT((int32) isr);
    //PRINT(isr_low);
    //PRINT(isr_high);

    entry->offset_low = isr_low;
    entry->offset_hi = isr_high;
    entry->css = 0x8;
    entry->present = 1;
    entry->privilege_level = 0;
    entry->storage = 0;
    entry->gate_type = 0xe; // 32-bit interrupt gate
}

void initialize_pic() {
    outb(PIC1_CMD, 0x11); // initialization
    outb(PIC2_CMD, 0x11);
    outb(PIC1_DATA, PIC1_TARGET_OFFSET);
    outb(PIC2_DATA, PIC2_TARGET_OFFSET);
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);
    outb(PIC1_DATA, 1);
    outb(PIC2_DATA, 1);

    //outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	//io_wait();
	//outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	//io_wait();
	//outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	//io_wait();
	//outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	//io_wait();
	//outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	//io_wait();
	//outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	//io_wait();
 
	//outb(PIC1_DATA, ICW4_8086);
	//io_wait();
	//outb(PIC2_DATA, ICW4_8086);
	//io_wait();
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

void gpf_handler() {
    asm volatile("cli");
    kputs("General protection fault."); NL;
    for(;;);
}

void test_isr() {
    int8 *vmem = (int8 *)0xb8000;
    vmem[8]++;
    //kputs("Test ISR ok."); NL;
    outb(0x20, 0x20);
    //kputs("IRQ accepted, return."); NL;
    // TODO for some reason, IRET crashes, simple C return is OK
    // Something weird is going on with the stack
    //asm volatile("iret");
}

/// }}}

// Main entry point {{{

void kmain(void) {
    extern int32 magic;
    extern multiboot_info_t *mbi; // Multiboot information struct
    extern int32 first_memblock;
    extern int32 entry_eip; IGNORE_UNUSED(entry_eip);

    if (magic != 0x2BADB002)
    {
        kputs("Multiboot magic number mismatch! Freezing.");
        for (;;);
    }

    update_cursorpos_from_vga();

    kputs("LapOS ");
    kputs(VERSION_ID);
    kputs(", branch ");
    kputs(VERSION_BRANCH);
    kputs("\n\n");

    // kputx(entry_eip);
    // for(;;);

    kputs("Memory size: ");
    kputh((unsigned int) mbi->mem_upper * 1024); NL; NL;

/*    mem_mapping mapping[] = {
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
    enable_paging();
#ifdef DEBUG_PAGING
    kputs("Paging enabled.\n");

    char *testvideo = (char *) 0xc0000000;
    testvideo[0] = 1;
    testvideo = (char *) 0xd0000000;
    testvideo[2] = 2;
#endif*/

    first_header = (memblock_header_t *) &first_memblock;

    if (mbi->flags & (1<<6)) {
        mmap_len = mbi->mmap_length;
        kputs("Size of memory map table: "); kputd(mmap_len); NL;

        mmap_start = (multiboot_memory_map_t *)mbi->mmap_table;
        current_mmap = mmap_start;

        // Get end of memory from the memory map
        // (and not from the Multiboot header)
        for (mmap_count = 0;
             (int32)current_mmap < (int32)mmap_start + mmap_len;
             mmap_count++,

             current_mmap = (multiboot_memory_map_t *)\
                                ((int32)current_mmap
                                    + current_mmap->size
                                    + sizeof(current_mmap->size))) {
            // kputs("mmap #"); kputd(mmap_count);
            // kputs(" addr: "); kputxx(current_mmap->addr);
            // kputs(" len: "); kputxx(current_mmap->len);
            // kputs(" type: ");
            // kputs(current_mmap->type - 1 ? "reserved" : "available"); NL;

            int32 last_address = current_mmap->addr + current_mmap->len - 1;
            if (current_mmap->type == 1 && last_address > max_address)
                max_address = last_address;
        }
        NL;
    } else
        // Fall back if no memory map was given
        max_address = (mbi->mem_upper + 1024) * 1024 - 1;


#ifdef DEBUG_MODULES_ALLOC
    if (mbi->flags & (1<<3)) kputs("Received valid module table!"); NL;
#endif
	if (mbi->flags & (1<<3) && mbi->mods_count) {
		module_num = mbi->mods_count;
		module_table_start = mbi->mods_addr;
		mods = (multiboot_module_table_t *)module_table_start;
		module_table_end = (int32)(mods + module_num) - 1;

		modules_start = mods[0].mod_start;
		modules_end = mods[module_num - 1].mod_end;
	} else
		module_num = 0;
	
    heap_start = (int32)first_header;
    //int32 after_modules = (
    //    (mods[module_num - 1].mod_end & 0xfffff000) + 4096);

    // If there are modules, exclude the memory occupied by them from
    if (module_num != 0) mark_modules_as_used();

    NL; kputs("Kernel memory free: ");
    kputh(kmem_available() * 4096);
    kputs(" ("); kputd(kmem_available()); kputs(" pages)"); NL; NL;

    // NOTE: IDT is no longer allocated on kernel heap, it will reside at the
    // beginning of RAM like in real mode.
    // Allocate IDT
    // 1 block is enough, since IDT is 2k (256 entries, 8 bytes each)
    /*kputs("Setting up IDT..."); NL;
    idt = kmalloc(1); IGNORE_UNUSED(idt);*/

    // TODO - Add ISR (use module as ISR?)
    //      - Set up IDT entry
    //      - Initialize PIC
    //      - STI instruction

    make_idt_entry(0x0d, (void *)gpf_handler);
    //make_idt_entry(0x20, (void *)test_isr);
    make_idt_entry(0x20, (void *)testisr_c);
    make_idt_entry(0x21, (void *)testisr_asm);
    //make_idt_entry(0x21, (void *)test_isr);
    //for (int n=0;n<256;n++)
    //    make_idt_entry(n, (void *)test_isr);
    kputs("IDT entry set up."); NL;
#ifdef DEBUG_IDT
    PRINTXX(idt[0].value);
    //asm volatile("int $0");
    initialize_pic();
    outb(0x21, 0xfc);
    outb(0xa1, 0xff);
    asm volatile("sti");
    for(;;);
#endif

    // Reserve page table on the heap
    page_table = kmalloc(max_address / 4096 / 1024 + 1);

    // Map module and vmem to higher half
    mem_mapping module_mapping[] = {
        {
            0x00001000,
            0x10000000,
            0x00001000
        },
        {
            0x80000000,
            0x90000000,
            modules_start
        },
        {
            0xf0000000,
            0xf0001000,
            0x000b8000
        },
    };

    build_pagetables(3, module_mapping, 0);
    //for(;;);
    //reload_pagetable();
    enable_paging();
#ifdef DEBUG_PAGING
    char *test2 = (char *) 0xf0000000;
    test2[0] = 1;
#endif

    // Transfer control to module at new address
    if (module_num != 0) {
        void (*module_entry)() = (void(*)()) 0x80000000;
        module_entry();
    } else
        kputs("No module loaded, kernel halts.");
}

// }}}
