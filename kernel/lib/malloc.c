// all allocations are 16-byte aligned(required by some sse instructions, and a general good practice on x86).

#include "malloc.h"
#include "../lib/string.h"

static uint32_t heap_ptr = HEAP_START;

void mm_init(void) {
	heap_ptr = HEAP_START;
	// zero the heap so doom's zone allocator sees clean memory
	memset((void*)HEAP_START, 0, HEAP_SIZE);
}

void* kmalloc(size_t size) {
	if (size == 0) return (void*)0;

	// align to 16 bytes
	size = (size + 15) & ~15u;
	if (heap_ptr + size > HEAP_END) {
		// we just return null
		return (void*)0;
	}

	void* ptr = (void*)heap_ptr;
	heap_ptr += size;
	return ptr;
}

void k_free(void* ptr) {
	// free is intentionally a no-op btw
	// doom manages its own memory after getting the initial block. i already told ya
	(void)ptr;
}

void* krealloc(void* ptr, size_t new_size) {
	// naive: allocate new block, copy old data, abandon old block. thats it
	// it works because doom only reallocs small structs
	if (!ptr) return kmalloc(new_size);
	void* new_ptr = kmalloc(new_size);
	if (!new_ptr) return (void*)0;
	// we dk the old size, so copy new_size bytes.
	// thi is kinda safe bcz dooms realloc calls always grow.
	memcpy(new_ptr, ptr, new_size);
	return new_ptr;
}
size_t mm_used(void) {
	return heap_ptr - HEAP_START;
}
size_t mm_free(void) {
	return HEAP_END - heap_ptr;
}
