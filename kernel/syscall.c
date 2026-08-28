/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"
#include "pmm.h"
#include "vmm.h"
#include "sched.h"

#include "spike_interface/spike_utils.h"

#define MAX_SEMAPHORES 16

typedef struct semaphore_t {
  int used;
  int count;
  process *wait_queue_head;
  process *wait_queue_tail;
} semaphore;

static semaphore semaphores[MAX_SEMAPHORES];

static int valid_semaphore(uint64 sem_id) {
  return sem_id < MAX_SEMAPHORES && semaphores[sem_id].used;
}

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  // buf is now an address in user space of the given app's user stack,
  // so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert( current );
  char* pa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)buf);
  sprint(pa);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // reclaim the current process, and reschedule. added @lab3_1
  free_process( current );
  schedule();
  return 0;
}

//
// maybe, the simplest implementation of malloc in the world ... added @lab2_2
//
uint64 sys_user_allocate_page() {
  void* pa = alloc_page();
  uint64 va;
  // if there are previously reclaimed pages, use them first (this does not change the
  // size of the heap)
  if (current->user_heap.free_pages_count > 0) {
    va =  current->user_heap.free_pages_address[--current->user_heap.free_pages_count];
    assert(va < current->user_heap.heap_top);
  } else {
    // otherwise, allocate a new page (this increases the size of the heap by one page)
    va = current->user_heap.heap_top;
    current->user_heap.heap_top += PGSIZE;

    current->mapped_info[HEAP_SEGMENT].npages++;
  }
  user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  return va;
}

//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_free_page(uint64 va) {
  user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  // add the reclaimed page to the free page list
  current->user_heap.free_pages_address[current->user_heap.free_pages_count++] = va;
  return 0;
}

//
// kerenl entry point of naive_fork
//
ssize_t sys_user_fork() {
  sprint("User call fork.\n");
  return do_fork( current );
}

//
// kerenl entry point of yield. added @lab3_2
//
ssize_t sys_user_yield() {
  current->status = READY;
  insert_to_ready_queue(current);
  schedule();

  return 0;
}

//
// create a semaphore and return its identifier
//
ssize_t sys_user_sem_new(uint64 init_value) {
  if (init_value > 0x7fffffffUL)
    return -1;

  for (int i = 0; i < MAX_SEMAPHORES; i++) {
    if (semaphores[i].used)
      continue;

    semaphores[i].used = 1;
    semaphores[i].count = (int)init_value;
    semaphores[i].wait_queue_head = NULL;
    semaphores[i].wait_queue_tail = NULL;
    return i;
  }

  return -1;
}

//
// acquire a semaphore resource
//
ssize_t sys_user_sem_P(uint64 sem_id) {
  if (!valid_semaphore(sem_id))
    return -1;

  semaphore *sem = &semaphores[sem_id];
  if (sem->count > 0) {
    sem->count--;
    return 0;
  }

  current->status = BLOCKED;
  current->queue_next = NULL;
  if (sem->wait_queue_tail == NULL) {
    sem->wait_queue_head = current;
    sem->wait_queue_tail = current;
  } else {
    sem->wait_queue_tail->queue_next = current;
    sem->wait_queue_tail = current;
  }

  schedule();
  return 0;
}

//
// release a semaphore resource
//
ssize_t sys_user_sem_V(uint64 sem_id) {
  if (!valid_semaphore(sem_id))
    return -1;

  semaphore *sem = &semaphores[sem_id];
  if (sem->wait_queue_head == NULL) {
    sem->count++;
    return 0;
  }

  process *woken = sem->wait_queue_head;
  sem->wait_queue_head = woken->queue_next;
  if (sem->wait_queue_head == NULL)
    sem->wait_queue_tail = NULL;
  woken->queue_next = NULL;
  woken->status = READY;
  woken->trapframe->regs.a0 = 0;
  insert_to_ready_queue(woken);
  return 0;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    // added @lab2_2
    case SYS_user_allocate_page:
      return sys_user_allocate_page();
    case SYS_user_free_page:
      return sys_user_free_page(a1);
    case SYS_user_fork:
      return sys_user_fork();
    case SYS_user_yield:
      return sys_user_yield();
    case SYS_user_sem_new:
      return sys_user_sem_new(a1);
    case SYS_user_sem_P:
      return sys_user_sem_P(a1);
    case SYS_user_sem_V:
      return sys_user_sem_V(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
