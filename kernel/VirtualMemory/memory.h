#include "types.h"

enum MEMORY {
	KERNEL = 1,
	WRITABLE = 2,
	COPY_ON_WRITE = 4,
};
void setup_virtual_memory();
uintptr_t memory_alloc(uint32_t size, uint32_t flags);
uintptr_t memory_free(uintptr_t address, uint32_t size, uint32_t flags);
uintptr_t memory_map(uint32_t virtual_address, uint32_t physical_address, uint32_t size, uint32_t flags);
uint32_t memory_unmap(uint32_t virtual_address, uint32_t physical_address, uint32_t size, uint32_t flags);
uint32_t virtual_memory_size();
uint32_t physical_memory_size();

static uint32_t get_kernel_pages();
