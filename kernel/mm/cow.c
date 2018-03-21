#include "mm.h"
#include "proc.h"
#include "percpu.h"
#include "libk.h"

void cow_copy_pte(pte_t *dest, pte_t *src)
{
	struct page *page = phys_to_page(*src & PTE_ADDR_MASK);
	__atomic_fetch_add(&page->refcount, 1, __ATOMIC_RELAXED);

	// Make page read-only and CoW if it isn't already
	if (!(*src & PAGE_COW)) {
		*src &= ~PAGE_WRITABLE;
		*src |= PAGE_COW;
		__atomic_fetch_add(&page->refcount, 1, __ATOMIC_RELAXED);
	}

	// Copy the PTE
	*dest = *src;
	kprintf("Handling a CoW copy\n");
}

// Returns true if the write is allowed and should be retried
bool cow_handle_write(pte_t *pte)
{
	if (!(*pte & PAGE_COW))
		return false;


	struct page *page = phys_to_page(*pte & PTE_ADDR_MASK);
	uint64_t next_count = __atomic_sub_fetch(&page->refcount, 1, __ATOMIC_RELAXED);

	if (next_count != 0) {
		// We need to allocate another page, and copy the memory into it
		physaddr_t dest = page_to_phys(pmm_alloc_order(0, GFP_NONE));
		physaddr_t src = *pte & PTE_ADDR_MASK;
		memcpy(phys_to_virt(dest), phys_to_virt(src), PAGE_SIZE);

		// Write the new address into the PTE
		*pte &= ~PTE_ADDR_MASK;
		*pte |= (dest & PTE_ADDR_MASK);
	}
	
	// Otherwise, this was the last reference and we are free to reuse this memory
	*pte &= ~PAGE_COW;
	*pte |= PAGE_WRITABLE;
	return true;
}
