#include "pmm.h"
#include "util/functions.h"
#include "riscv.h"
#include "config.h"
#include "util/string.h"
#include "memlayout.h"
#include "spike_interface/spike_utils.h"

// _end is defined in kernel/kernel.lds, it marks the ending (virtual) address of PKE kernel
extern char _end[];
// g_mem_size is defined in spike_interface/spike_memory.c, it indicates the size of our
// (emulated) spike machine. g_mem_size's value is obtained when initializing HTIF. 
extern uint64 g_mem_size;

static uint64 free_mem_start_addr;  //beginning address of free memory
static uint64 free_mem_end_addr;    //end address of free memory (not included)

// A page reference count is needed when multiple user page tables share one
// physical heap page during COW.  The managed memory is capped by
// PKE_MAX_ALLOWABLE_RAM, so this table covers every page that alloc_page()
// can return without consuming another managed page for metadata.
#define MAX_MANAGED_PAGES (PKE_MAX_ALLOWABLE_RAM / PGSIZE)
static uint32 g_page_refcounts[MAX_MANAGED_PAGES];

typedef struct node {
  struct node *next;
} list_node;

// g_free_mem_list is the head of the list of free physical memory pages
static list_node g_free_mem_list;

//
// actually creates the freepage list. each page occupies 4KB (PGSIZE), i.e., small page.
// PGSIZE is defined in kernel/riscv.h, ROUNDUP is defined in util/functions.h.
//
static void create_freepage_list(uint64 start, uint64 end) {
  g_free_mem_list.next = 0;
  for (uint64 p = ROUNDUP(start, PGSIZE); p + PGSIZE < end; p += PGSIZE)
    free_page( (void *)p );
}

//
// place a physical page at *pa to the free list of g_free_mem_list (to reclaim the page)
//
void free_page(void *pa) {
  if (((uint64)pa % PGSIZE) != 0 || (uint64)pa < free_mem_start_addr || (uint64)pa >= free_mem_end_addr)
    panic("free_page 0x%lx \n", pa);

  g_page_refcounts[((uint64)pa - free_mem_start_addr) / PGSIZE] = 0;

  // insert a physical page to g_free_mem_list
  list_node *n = (list_node *)pa;
  n->next = g_free_mem_list.next;
  g_free_mem_list.next = n;
}

//
// takes the first free page from g_free_mem_list, and returns (allocates) it.
// Allocates only ONE page!
//
void *alloc_page(void) {
  list_node *n = g_free_mem_list.next;
  if (n) g_free_mem_list.next = n->next;

  if (n)
    g_page_refcounts[((uint64)n - free_mem_start_addr) / PGSIZE] = 1;

  return (void *)n;
}

// Add one page-table mapping reference to a managed physical page.
void page_retain(void *pa) {
  if (((uint64)pa % PGSIZE) != 0 || (uint64)pa < free_mem_start_addr ||
      (uint64)pa >= free_mem_end_addr)
    panic("page_retain 0x%lx\n", pa);

  uint32 *ref = &g_page_refcounts[((uint64)pa - free_mem_start_addr) / PGSIZE];
  if (*ref == 0)
    panic("page_retain on a free page 0x%lx\n", pa);
  (*ref)++;
}

// Drop one page-table mapping reference and return the page to the free list
// only after the last mapping disappears.
void page_release(void *pa) {
  if (((uint64)pa % PGSIZE) != 0 || (uint64)pa < free_mem_start_addr ||
      (uint64)pa >= free_mem_end_addr)
    panic("page_release 0x%lx\n", pa);

  uint32 *ref = &g_page_refcounts[((uint64)pa - free_mem_start_addr) / PGSIZE];
  if (*ref == 0)
    panic("page_release on a free page 0x%lx\n", pa);

  (*ref)--;
  if (*ref == 0)
    free_page(pa);
}

//
// pmm_init() establishes the list of free physical pages according to available
// physical memory space.
//
void pmm_init() {
  // start of kernel program segment
  uint64 g_kernel_start = KERN_BASE;
  uint64 g_kernel_end = (uint64)&_end;

  uint64 pke_kernel_size = g_kernel_end - g_kernel_start;
  sprint("PKE kernel start 0x%lx, PKE kernel end: 0x%lx, PKE kernel size: 0x%lx .\n",
    g_kernel_start, g_kernel_end, pke_kernel_size);

  // free memory starts from the end of PKE kernel and must be page-aligined
  free_mem_start_addr = ROUNDUP(g_kernel_end , PGSIZE);

  // recompute g_mem_size to limit the physical memory space that our riscv-pke kernel
  // needs to manage
  g_mem_size = MIN(PKE_MAX_ALLOWABLE_RAM, g_mem_size);
  if( g_mem_size < pke_kernel_size )
    panic( "Error when recomputing physical memory size (g_mem_size).\n" );

  free_mem_end_addr = g_mem_size + DRAM_BASE;
  sprint("free physical memory address: [0x%lx, 0x%lx] \n", free_mem_start_addr,
    free_mem_end_addr - 1);

  sprint("kernel memory manager is initializing ...\n");
  // create the list of free pages
  create_freepage_list(free_mem_start_addr, free_mem_end_addr);
}
