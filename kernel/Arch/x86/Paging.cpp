#include "Paging.h"

PAGE_DIRECTORY Paging::page_direcotry __attribute__((aligned(PAGE_SIZE)));
PAGE_TABLE Paging::kernel_page_tables[FULL_KERNEL_PAGES] __attribute__((aligned(PAGE_SIZE)));
PAGE_TABLE Paging::boostrap_page_table[2] __attribute__((aligned(PAGE_SIZE)));

// Initialize page direcotry and page tables.
void Paging::setup(uint32_t num_kernel_pages)
{
	PAGE_DIRECTORY* ph_page_direcotry = (PAGE_DIRECTORY*)VIR_TO_PHY((uint32_t)&page_direcotry);
	initialize_page_directory(ph_page_direcotry);
	// Link to page tables
	setup_page_tables();
	// Map identity and higher half
	map_boot_pages(KERNEL_PHYSICAL_ADDRESS, KERNEL_PHYSICAL_ADDRESS, num_kernel_pages);
	map_boot_pages(KERNEL_VIRTUAL_ADDRESS, KERNEL_PHYSICAL_ADDRESS, num_kernel_pages);
	// Load page directory and enable paging
	load_page_directory((uint32_t)ph_page_direcotry);
	enable_PSE();
	enable_paging();
}

void Paging::setup_page_tables()
{
	PAGE_DIRECTORY* ph_page_direcotry = (PAGE_DIRECTORY*)VIR_TO_PHY((uint32_t)&page_direcotry);
	PAGE_TABLE* ph_kpage_tables = (PAGE_TABLE*)VIR_TO_PHY((uint32_t)kernel_page_tables);
	PAGE_TABLE* ph_bootpage_tables = (PAGE_TABLE*)VIR_TO_PHY((uint32_t)boostrap_page_table);
	uint32_t page_flags = PAGE_FLAGS_PRESENT | PAGE_FLAGS_WRITABLE;
	initialize_page_directory(ph_page_direcotry);
	// Kernel pages tables
	for (size_t i = 0; i < FULL_KERNEL_PAGES; i++) {
		initialize_page_table(&ph_kpage_tables[i]);
		fill_directory_entry(&ph_page_direcotry->entries[i + GET_PDE_INDEX(KERNEL_BASE)],
		                     GET_FRAME((uint32_t)&ph_kpage_tables[i]), page_flags);
	}
	// Identity page tables for 8mb of the kernel bootstrap
	for (size_t i = 0; i < 2; i++) {
		initialize_page_table(&ph_bootpage_tables[i]);
		fill_directory_entry(&ph_page_direcotry->entries[i], GET_FRAME((uint32_t)&ph_bootpage_tables[i]), page_flags);
	}
	// Set recursive entry
	fill_directory_entry(&ph_page_direcotry->entries[RECURSIVE_ENTRY], GET_FRAME((uint32_t)ph_page_direcotry),
	                     page_flags);
}
// Map virtual address range to physical address, can be used only at boot time.
void Paging::map_boot_pages(uint32_t virtual_address, uint32_t physical_address, uint32_t pages)
{
	PAGE_DIRECTORY* ph_page_directory = (PAGE_DIRECTORY*)VIR_TO_PHY((uint32_t)&page_direcotry);
	uint32_t current_v_page = virtual_address;
	uint32_t current_p_page = physical_address;
	uint32_t page_flags = PAGE_FLAGS_PRESENT | PAGE_FLAGS_WRITABLE | PAGE_FLAGS_GLOBAL;
	for (size_t i = 0; i < pages; i++) {
		uint32_t pde = GET_PDE_INDEX(current_v_page);
		uint32_t pte = GET_PTE_INDEX(current_v_page);
		PAGE_TABLE* ph_page_table = (PAGE_TABLE*)(ph_page_directory->entries[pde].frame * PAGE_SIZE);
		fill_page_table_entry(&ph_page_table->entries[pte], GET_FRAME(current_p_page), page_flags);
		current_v_page += PAGE_SIZE;
		current_p_page += PAGE_SIZE;
	}
}

// Map a virtual page to a physical page.
void Paging::map_page(uint32_t virtual_address, uint32_t physical_address, uint32_t flags)
{
	PAGE_TABLE* page_table = (PAGE_TABLE*)GET_PAGE_VIRTUAL_ADDRESS(RECURSIVE_ENTRY, GET_PDE_INDEX(virtual_address));
	fill_page_table_entry(&page_table->entries[GET_PTE_INDEX(virtual_address)], GET_FRAME(physical_address), flags);
	invalidate_page(virtual_address);
}

// Map contagious virtual pages to contagious physical pages.
void Paging::map_pages(uint32_t virtual_address, uint32_t physical_address, uint32_t pages, uint32_t flags)
{
	for (size_t i = 0; i < pages; i++) {
		map_page(virtual_address + PAGE_SIZE * i, physical_address + PAGE_SIZE * i, flags);
	}
}

// Map a virtual page to a physical page.
void Paging::unmap_page(uint32_t virtual_address)
{
	PAGE_TABLE* page_table = (PAGE_TABLE*)GET_PAGE_VIRTUAL_ADDRESS(RECURSIVE_ENTRY, GET_PDE_INDEX(virtual_address));
	page_table->entries[GET_PTE_INDEX(virtual_address)].present = 0;
	invalidate_page(virtual_address);
}

// Map contagious virtual pages to contagious physical pages.
void Paging::unmap_pages(uint32_t virtual_address, uint32_t pages)
{
	for (size_t i = 0; i < pages; i++) {
		unmap_page(virtual_address + PAGE_SIZE * i);
	}
}

void Paging::map_page_table(uint32_t virtual_address, uint32_t pt_virtual_address)
{
	uint32_t page_flags = PAGE_FLAGS_PRESENT | PAGE_FLAGS_WRITABLE | PAGE_FLAGS_USER;
	PAGE_DIRECTORY* page_dir = (PAGE_DIRECTORY*)GET_PAGE_VIRTUAL_ADDRESS(RECURSIVE_ENTRY, RECURSIVE_ENTRY);
	uint32_t pt_frame = get_physical_page(pt_virtual_address);
	fill_directory_entry(&page_dir->entries[GET_PDE_INDEX(virtual_address)], pt_frame, page_flags);
}

bool Paging::check_page_table_exists(uint32_t virtual_address)
{
	PAGE_DIRECTORY* page_dir = (PAGE_DIRECTORY*)GET_PAGE_VIRTUAL_ADDRESS(RECURSIVE_ENTRY, RECURSIVE_ENTRY);
	return page_dir->entries[GET_PDE_INDEX(virtual_address)].present;
}

bool Paging::check_page_exits_in_table(uint32_t virtual_address)
{
	PAGE_TABLE* page_table = (PAGE_TABLE*)GET_PAGE_VIRTUAL_ADDRESS(RECURSIVE_ENTRY, GET_PDE_INDEX(virtual_address));
	return page_table->entries[GET_PTE_INDEX(virtual_address)].present;
}

bool Paging::check_page_present(uint32_t virtual_address)
{
	if (check_page_table_exists(virtual_address) == false)
		return false;
	return check_page_exits_in_table(virtual_address);
}

uint32_t Paging::get_physical_page(uint32_t virtual_address)
{
	PAGE_TABLE* page_table = (PAGE_TABLE*)GET_PAGE_VIRTUAL_ADDRESS(RECURSIVE_ENTRY, GET_PDE_INDEX(virtual_address));
	return page_table->entries[GET_PTE_INDEX(virtual_address)].frame;
}

// Zeroing page directory entries.
void Paging::initialize_page_directory(PAGE_DIRECTORY* page_direcotry)
{
	for (size_t i = 0; i < NUMBER_OF_PAGE_DIRECOTRY_ENTRIES; i++) {
		*((uint32_t*)&page_direcotry->entries[i]) = 0;
	}
}

// Zeroing page table entries.
void Paging::initialize_page_table(PAGE_TABLE* page_direcotry)
{
	for (size_t i = 0; i < NUMBER_OF_PAGE_TABLE_ENTRIES; i++) {
		*((uint32_t*)&page_direcotry->entries[i]) = 0;
	}
}

void Paging::map_kernel_pd_entries(uint32_t pd)
{
	uint32_t page_flags = PAGE_FLAGS_PRESENT | PAGE_FLAGS_WRITABLE;
	PAGE_DIRECTORY* new_page_dir = (PAGE_DIRECTORY*)pd;
	initialize_page_directory(new_page_dir);
	const size_t start = GET_PDE_INDEX(KERNEL_VIRTUAL_ADDRESS);
	for (size_t i = start; i < RECURSIVE_ENTRY; i++) {
		memcpy((void*)&new_page_dir->entries[i], (void*)&page_direcotry.entries[i], sizeof(PAGE_DIRECTORY_ENTRY));
	}
	fill_directory_entry(&new_page_dir->entries[RECURSIVE_ENTRY], get_physical_page(pd), page_flags);
}

void Paging::fill_directory_entry(PAGE_DIRECTORY_ENTRY* page_direcotry_entry, uint16_t physical_frame, uint32_t flags)
{
	page_direcotry_entry->unused = 0;
	page_direcotry_entry->global = BOOL(flags & PAGE_FLAGS_GLOBAL);
	page_direcotry_entry->pwt = 0;
	page_direcotry_entry->pcd = 0;
	page_direcotry_entry->pse = 0;
	page_direcotry_entry->present = BOOL(flags & PAGE_FLAGS_PRESENT);
	page_direcotry_entry->rw = BOOL(flags & PAGE_FLAGS_WRITABLE);
	page_direcotry_entry->user = BOOL(flags & PAGE_FLAGS_USER);
	page_direcotry_entry->frame = physical_frame;
}

void Paging::fill_directory_PSE_entry(PAGE_DIRECTORY_ENTRY* page_direcotry_entry, uint16_t physical_frame,
                                      uint32_t flags)
{
	page_direcotry_entry->unused = 0;
	page_direcotry_entry->global = BOOL(flags & PAGE_FLAGS_GLOBAL);
	page_direcotry_entry->pwt = 0;
	page_direcotry_entry->pcd = 0;
	page_direcotry_entry->pse = 1;
	page_direcotry_entry->present = BOOL(flags & PAGE_FLAGS_PRESENT);
	page_direcotry_entry->rw = BOOL(flags & PAGE_FLAGS_WRITABLE);
	page_direcotry_entry->user = BOOL(flags & PAGE_FLAGS_USER);
	page_direcotry_entry->frame = physical_frame;
}

void Paging::fill_page_table_entry(PAGE_TABLE_ENTRY* page_table_entry, uint16_t physical_frame, uint32_t flags)
{
	// FIXME: accessing bits directly is so slow.
	page_table_entry->unused = 0;
	page_table_entry->global = BOOL(flags & PAGE_FLAGS_GLOBAL);
	page_table_entry->pwt = 0;
	page_table_entry->pcd = 0;
	page_table_entry->pse = 0;
	page_table_entry->present = BOOL(flags & PAGE_FLAGS_PRESENT);
	page_table_entry->rw = BOOL(flags & PAGE_FLAGS_WRITABLE);
	page_table_entry->user = BOOL(flags & PAGE_FLAGS_USER);
	page_table_entry->frame = physical_frame;
}

void Paging::load_page_directory(uint32_t page_direcotry)
{
	asm("MOV %%CR3,%0" : : "r"(page_direcotry) :);
}

void Paging::enable_paging()
{
	asm("MOV %%EAX,%%CR0;"
	    "OR  %%EAX,%0;"
	    "MOV %%CR0,%%EAX ;"
	    :
	    : "i"(CR0_WP | CR0_PAGING)
	    : "eax");
}

void Paging::enable_PSE()
{
	asm("MOV %%EAX,%%CR4;"
	    "OR %%EAX,%0;"
	    "MOV %%CR4,%%EAX;"
	    :
	    : "i"(CR4_PSE)
	    : "eax");
}

void Paging::invalidate_page(uint32_t addr)
{
	asm volatile("invlpg [%0]" ::"r"(addr) : "memory");
}