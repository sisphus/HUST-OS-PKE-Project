#ifndef _PMM_H_
#define _PMM_H_

// Initialize phisical memeory manager
void pmm_init();
// Allocate a free phisical page
void* alloc_page();
// Free an allocated page
void free_page(void* pa);
// Add/remove one mapping reference to a physical page.
void page_retain(void *pa);
void page_release(void *pa);

#endif
