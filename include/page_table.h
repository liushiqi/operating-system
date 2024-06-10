/**
 * @file page_table.h
 * Defined several macros and functions to operate PTE.
 */

#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "stdbool.h"
#include "string.h"
#include <memory.h>
#include <sys/type.h>

#define SATP_ASID_SHIFT 44ul
#define SATP_MODE_SHIFT 60ul

#define NORMAL_PAGE_SHIFT 12ul
#define NORMAL_PAGE_SIZE (1ul << NORMAL_PAGE_SHIFT)
#define LARGE_PAGE_SHIFT 21ul
#define LARGE_PAGE_SIZE (1ul << LARGE_PAGE_SHIFT)
#define HUGE_PAGE_SHIFT 30ul
#define HUGE_PAGE_SIZE (1ul << HUGE_PAGE_SHIFT)

/**
 * @brief Flush entire local TLB.
 * 'sfence.vma' implicitly fences with the instruction cache as well, so a 'fence.i' is not necessary.
 */
static inline void flush_all_tlb(void) { asm volatile("sfence.vma" : : : "memory"); }

/** Flush one page from local TLB */
static inline void flush_one_tlb(unsigned long addr) { asm volatile("sfence.vma %0" : : "r"(addr) : "memory"); }

/** Flush instruction cache */
static inline void flush_instruction_cache(void) { asm volatile("fence.i" ::: "memory"); }

/** write into the satp csr register. */
static inline void set_satp(unsigned mode, unsigned asid, unsigned long ppn) {
  unsigned long __v = (unsigned long)(((unsigned long)mode << SATP_MODE_SHIFT) | ((unsigned long)asid << SATP_ASID_SHIFT) | ppn);
  asm volatile("csrw satp, %0" : : "rK"(__v) : "memory");
}

/*
 * page table format:
 * | 63      10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * |     PFN    | reserved for SW | D | A | G | U | X | W | R | V |
 */

#define PAGE_ACCESSED_OFFSET 6

#define PAGE_VALID (1ul << 0ul)    /**< Page is valid or not. */
#define PAGE_READ (1ul << 1ul)     /**< Readable */
#define PAGE_WRITE (1ul << 2ul)    /**< Writable */
#define PAGE_EXEC (1ul << 3ul)     /**< Executable */
#define PAGE_USER (1ul << 4ul)     /**< User */
#define PAGE_GLOBAL (1ul << 5ul)   /**< Global */
#define PAGE_ACCESSED (1ul << 6ul) /**< Set by hardware on any access */
#define PAGE_DIRTY (1ul << 7ul)    /**< Set by hardware on any write */
#define PAGE_SOFT (1ul << 8ul)     /**< Reserved for software */

#define PAGE_PPN_SHIFT 10ul
#define PAGE_PPN_SIZE (1ul << PAGE_PPN_SHIFT)

#define VA_MASK ((1ul << 39) - 1)

#define PPN_BITS 9ul
#define NUM_PTE_ENTRY (1 << PPN_BITS)

typedef uint64_t page_table_entry_t;

/**
 * Convert a virtual address of kernel to physical address.
 * @param kernel_address The virtual address.
 * @return The physical address.
 */
static inline uintptr_t ka_to_pa(uintptr_t kernel_address) {
#ifndef SECOND_STAGE
  return kernel_address & ~0xffffffc000000000;
#else
  return kernel_address;
#endif
}

/**
 * Convert a physical address to a virtual address in kernel ::ka_to_pa(uintptr_t kernel_address)
 * @param physical_address
 * @return The virtual address
 */
static inline uintptr_t pa_to_ka(uintptr_t physical_address) {
#ifndef SECOND_STAGE
  return physical_address | 0xffffffc000000000;
#else
  return physical_address;
#endif
}

/**
 * Get a physical address from a page table entry
 * @param entry The entry to page table entry
 * @return The physical addresss of the page table entry
 */
static inline uintptr_t get_physical_address(page_table_entry_t entry) { return (entry | 0x003ffffffffffc00ul) << 2ul; }

static inline uint64_t get_attribute(page_table_entry_t entry, uint64_t mask) { return entry & mask; }

static inline uint64_t set_attribute(page_table_entry_t *entry, uint64_t bits) { return *entry |= bits; }

static inline void clear_page_directory(uintptr_t page_directory_address) { memset((void *)page_directory_address, 0, PAGE_SIZE); }

static inline uintptr_t pa_to_pfn(uintptr_t physical_page) { return (physical_page >> NORMAL_PAGE_SHIFT) << PAGE_PPN_SHIFT; }

static inline uintptr_t pfn_to_pa(uintptr_t page_frame_number) { return (page_frame_number >> PAGE_PPN_SHIFT) << NORMAL_PAGE_SHIFT; }

static inline uintptr_t va_to_pa(uintptr_t virtual_address, const page_table_entry_t *root_page) {
  uintptr_t first_index = (virtual_address >> HUGE_PAGE_SHIFT) & 0x1fful;
  page_table_entry_t *second = (page_table_entry_t *)pa_to_ka(pfn_to_pa(root_page[first_index] & ~PAGE_VALID));
  uintptr_t second_index = (virtual_address >> LARGE_PAGE_SHIFT) & 0x1fful;
  page_table_entry_t *third = (page_table_entry_t *)pa_to_ka(pfn_to_pa(second[second_index] & ~PAGE_VALID));
  if (get_attribute((page_table_entry_t)second[second_index], PAGE_READ | PAGE_EXEC) != 0)
    return (uintptr_t)third + (virtual_address & (LARGE_PAGE_SIZE - 1));
  uintptr_t third_index = (virtual_address >> NORMAL_PAGE_SHIFT) & 0x1fful;
  return pfn_to_pa(third[third_index] & ~(PAGE_PPN_SIZE - 1)) + (virtual_address & (NORMAL_PAGE_SIZE - 1));
}

static inline int map_page(uintptr_t virtual_address, uintptr_t physical_address, page_table_entry_t *page_directory, uint64_t bits) {
  uintptr_t first_index = (virtual_address >> HUGE_PAGE_SHIFT) & 0x1fful;
  page_table_entry_t *second = (page_table_entry_t *)pa_to_ka(pfn_to_pa(page_directory[first_index] & ~PAGE_VALID));
  if (second == NULL || (unsigned long)second == 0xffffffc000000000) {
    second = (page_table_entry_t *)alloc_page(1);
    memset(second, 0, 0xfff);
    page_directory[first_index] = pa_to_pfn(ka_to_pa((uintptr_t)second)) | PAGE_VALID;
  }
  uintptr_t second_index = (virtual_address >> LARGE_PAGE_SHIFT) & 0x1fful;
  page_table_entry_t *third = (page_table_entry_t *)pa_to_ka(pfn_to_pa(second[second_index] & ~PAGE_VALID));
  if (third == NULL || (unsigned long)third == 0xffffffc000000000) {
    third = (page_table_entry_t *)alloc_page(1);
    memset(third, 0, 0xfff);
    second[second_index] = pa_to_pfn(ka_to_pa((uintptr_t)third)) | PAGE_VALID;
  }
  uintptr_t third_index = (virtual_address >> NORMAL_PAGE_SHIFT) & 0x1fful;
  if (third[third_index] != 0)
    return -1;
  third[third_index] = pa_to_pfn(physical_address & ~(NORMAL_PAGE_SIZE - 1ul)) | PAGE_DIRTY | PAGE_ACCESSED | bits | PAGE_VALID;
  return 0;
}

#endif // PAGE_TABLE_H
