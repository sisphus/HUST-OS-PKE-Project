#ifndef _CONFIG_H_
#define _CONFIG_H_

// we use two HART (cpu) in challenge3
#define NCPU 2

//interval of timer interrupt. added @lab1_3
#define TIMER_INTERVAL 1000000

#define DRAM_BASE 0x80000000

/* we use fixed physical (also logical) addresses for the stacks and trap frames as in
 Bare memory-mapping mode */
// Each hart gets a separate 16MB address window for its runtime state.
#define HART_MEM_STRIDE 0x01000000UL

// Per-hart user stack top.
#define USER_STACK_BASE 0x81100000UL
#define USER_STACK(hartid) (USER_STACK_BASE + (hartid) * HART_MEM_STRIDE)

// Per-hart stack used by PKE kernel when a syscall happens.
#define USER_KSTACK_BASE 0x81200000UL
#define USER_KSTACK(hartid) (USER_KSTACK_BASE + (hartid) * HART_MEM_STRIDE)

// Per-hart trap frame used to assemble the user process.
#define USER_TRAP_FRAME_BASE 0x81300000UL
#define USER_TRAP_FRAME(hartid) (USER_TRAP_FRAME_BASE + (hartid) * HART_MEM_STRIDE)

#endif
