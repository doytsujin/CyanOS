#include "Heap.h"
#include "Tasking/ScopedLock.h"

volatile PageFrameBlock* Heap::malloc_mem;
StaticSpinlock Heap::lock;

void Heap::setup()
{
	lock.init();
	malloc_mem = nullptr;
	malloc_mem = create_new_page();
}

// Returns address of zeroed block of memory.
void* Heap::kmalloc(unsigned size, unsigned flags)
{
	UNUSED(flags);
	ScopedLock local_lock(lock);
	// FIXME: allocate normal memory if the size is greater than MAX_SIZE (there is a bug when less) and handle
	// freeing too
	if (!size)
		return nullptr;

	if (size > MAX_SIZE) {
		return nullptr;
	}

	BlockHeader* free_block;
	// Check if there is a free block in the list
	// TODO: if the current page is not enough, try expanding it by allocating neighbor pages.
	// TODO: classify the sizes into power of 2 sizes.
	free_block = find_free_block(size + sizeof(BlockHeader));
	if (free_block) {
		link_block(free_block->previous, free_block);
		free_block->size = size;
		return (void*)HEADER_TO_ADDR(free_block);
	}
	// Create new memory page
	PageFrameBlock* new_page = create_new_page();
	free_block = initiate_first_block(new_page);
	free_block->size = size;
	uintptr_t alloc_address = HEADER_TO_ADDR(free_block);
	memset((void*)alloc_address, 0, size);
	return (void*)alloc_address;
}

// Frees block of memory allocated by kmalloc.
void Heap::kfree(void* addr)
{
	if (!addr)
		return;

	ScopedLock local_lock(lock);
	BlockHeader* current_block = (BlockHeader*)ADDR_TO_HEADER(addr);
	unlink_block(current_block);
}

PageFrameBlock* Heap::create_new_page()
{
	// FIXME: create page according to the allocation size.
	// FIXME: do some asserts to nullpointers.
	PageFrameBlock* new_page =
	    (PageFrameBlock*)Memory::alloc(MALLOC_PAGE_SIZE, MEMORY_TYPE::KERNEL | MEMORY_TYPE::WRITABLE);
	new_page->size = MALLOC_PAGE_SIZE;

	PageFrameBlock* last_page = get_last_page();
	if (last_page) {
		last_page->next = new_page;
	}
	return new_page;
}

BlockHeader* Heap::initiate_first_block(PageFrameBlock* new_page)
{
	BlockHeader* empty_block = (BlockHeader*)(new_page + 1);
	BlockHeader* free_block = (empty_block + 1);
	empty_block->size = 0;
	empty_block->previous = 0;
	empty_block->next = free_block;
	free_block->previous = empty_block;
	return free_block;
}

void Heap::link_block(BlockHeader* current_block, BlockHeader* new_block)
{
	BlockHeader* prev_block = current_block;
	BlockHeader* next_block = current_block->next;
	prev_block->next = new_block;
	if (next_block)
		next_block->previous = new_block;
}

void Heap::unlink_block(BlockHeader* current_block)
{
	BlockHeader* prev_block = current_block->previous;
	BlockHeader* next_block = current_block->next;
	prev_block->next = next_block;
	if (next_block) {
		next_block->previous = prev_block;
	}
}

BlockHeader* Heap::find_free_block(unsigned size)
{
	volatile PageFrameBlock* current_page = malloc_mem;
	BlockHeader *current_block, *free_block;
	unsigned current_free_space;
	while (current_page) {
		current_block = (BlockHeader*)(current_page + 1);
		while (current_block) {
			if (current_block->next) {
				current_free_space = (unsigned)current_block->next - NEXT_NEIGHBOR_BLOCK(current_block);
			} else { // last block in the current page
				current_free_space = (unsigned)current_page + current_page->size - NEXT_NEIGHBOR_BLOCK(current_block);
			}
			if (current_free_space >= size) {
				free_block = (BlockHeader*)NEXT_NEIGHBOR_BLOCK(current_block);
				free_block->previous = current_block;
				free_block->next = current_block->next;
				return free_block;
			}

			current_block = current_block->next;
		}
		current_page = current_page->next;
	}
	return nullptr;
}

PageFrameBlock* Heap::get_last_page()
{
	if (!malloc_mem) {
		return nullptr;
	}
	volatile PageFrameBlock* current = malloc_mem;
	while (current->next) {
		current = current->next;
	}
	return (PageFrameBlock*)current;
}