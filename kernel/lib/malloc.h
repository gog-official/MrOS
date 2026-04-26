// simple slab/bump allocator.
//
// Design(works as a revision): 
// 	we'll carve a heap region from 4MB to 12MB (8MB).
// 	doom need roughly 6-7 mb for its zone memory manager btw
// 	we give doom one large block via some func and let doom's internal
// 	zone allocator(Z_Init) manage it.
//
// 	Our allocator is a single bump allocator:
// 	- Allocation: advance heap pointer by size (aligned to 16 or so bytes)
// 	- free: no-op for now (ysk doom uses its own zone for internal allocs)
//
// 	So for you kind info memory map after this change we did:
// 	0x00000 - 0x00500 BIOS data + our mem size scratch
// 	0x07C00 - 0x07E00 bootloader
// 	0x10000 - 0x3FFFF kernel (256KB max - generous)
// 	0x90000 - 0x8FFFF kernel stack
// 	0x400000 - 0xBFFFFF heap (8MB) this is where our doom'll live
#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>
#include <stdint.h>

#define HEAP_START 0x400000 //4MB mark
#define HEAP_SIZE 0x800000
#define HEAP_END (HEAP_START + HEAP_SIZE)

void mm_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr); // no-op in bump allocator
void* krealloc(void* ptr, size_t new_size);
size_t mm_used(void);
size_t mm_free(void);

#endif // !MALLOC_H
