#include <stdint.h>

#define NULL (void *) 0x0

typedef unsigned int int32;

typedef struct {
    int32 source_start;
    int32 source_end;
    int32 target_start;
} mem_mapping;

typedef int32 __attribute__((aligned(16))) page_table[1024];

int current_pagetable_set = 0;
page_table page_directory[2];
page_table page_tables[1024][2];

void build_pagetables(int32 mapping_count, mem_mapping *map,
                      int identity_mapping)
{
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
        kputs("Multiboot magic number mismatch! Freezing.");
        while (1);
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
