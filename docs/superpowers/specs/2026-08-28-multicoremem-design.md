# Lab 2 Challenge 3: Multicore Memory Management Design

## 1. Goal and scope

Implement the course requirements for `lab2_challenge3_multicoremem` on top of
`lab2_3_pagefault`:

- run `app_alloc0` and `app_alloc1` on two harts with `spike -p2`;
- keep machine-mode trap resources isolated per hart;
- initialize shared machine and kernel-memory resources once and synchronize all harts;
- protect the shared physical-page free list with an atomic spinlock;
- keep each process's current pointer and virtual heap cursor independent;
- load the application selected by the hart's command-line position;
- wait for both applications before one designated hart shuts down.

The implementation will not merge `lab1_challenge3_multicore`,
`lab2_challenge1_pagefaults`, or `lab2_challenge2_singlepageheap`. The
multicore runtime mechanisms will be reimplemented on this branch as required
by the course. No new tests or test framework will be added; verification is a
clean build and a real two-hart Spike run.

## 2. State ownership

### Per-hart state

The following state is indexed by the hart ID stored in `tp`:

- `g_itrframe[NCPU]`: machine-mode interrupt frames;
- `current[NCPU]`: the process currently running on each hart;
- `g_ticks[NCPU]`: supervisor timer accounting;
- `vm_alloc_stage[NCPU]`: whether physical-page allocation is occurring during
  user-process setup or execution.

`m_start()` writes the incoming hart ID to `tp`. Supervisor-mode code obtains
the current hart with `read_tp()`; it does not read the machine-only
`mhartid` CSR. Each hart's `mscratch` points to its own
`g_itrframe[hartid]`.

The existing `stack0[4096 * NCPU]` layout remains per-hart. The machine trap
entry code continues to use `mscratch` and the hart-specific stack offset, so
simultaneous machine traps do not share a save area or stack.

### Per-process state

`user_app[NCPU]` contains one process object per hart. Each process has its own:

- user page-table root;
- user trapframe;
- user-kernel stack;
- virtual heap cursor `ufree_page`.

Both cursors start at `USER_FREE_ADDRESS_START`, so the first allocation in
each process can use virtual address `0x400000`. The independent user page
tables make equal virtual addresses valid: each page table maps its own
`0x400000` to a different physical page.

During process construction, `proc->trapframe->regs.tp` is initialized to the
process's hart ID. `return_to_user()` restores this value with the other user
registers, preserving the hart ID across user-mode execution and traps.

`switch_to(proc)` records `proc` in `current[read_tp()]` and configures the
current hart's supervisor trap CSRs before returning to user mode. The user
trapframe restores `tp` as part of the normal register restore.

### Shared state

The physical free-page list and the kernel page table are shared by all harts.
HTIF state and the emulated-memory information obtained from the device tree
are also shared. These resources are initialized once and then made visible to
all harts through startup barriers.

## 3. Startup and synchronization

### Machine-mode startup

`m_start(hartid, dtb)` follows this order:

1. write `hartid` to `tp`;
2. let hart 0 perform `spike_file_init()` and `init_dtb(dtb)` exactly once;
3. make all harts wait at `m_boot_count`;
4. configure each hart's `mscratch`, `mtvec`, machine/supervisor delegation,
   interrupt enables, and timer;
5. execute `mret` into `s_start()`.

`timerinit(hartid)` runs on every hart because each hart has its own CLINT timer
comparison point. Per-hart CSR configuration is also performed by every hart.

### Supervisor-mode startup

Each hart initially clears its own `satp`. Hart 0 performs `pmm_init()` and
`kern_vm_init()` once. All harts then wait at `s_boot_count`. Only after that
barrier does every hart enable the shared kernel page table and build its own
user process.

The counters for machine startup, supervisor startup, and process exit are
separate. The barrier primitive is monotonic and does not reset its counter.

## 4. Physical-page allocator synchronization

`kernel/sync_utils.h` will provide a small spinlock backed by RISC-V
`amoswap`. A zero lock value means free and a one value means held. The lock
operation atomically swaps in one and spins while the returned old value is
one; unlock publishes zero.

The allocator lock protects only the shared free-list mutation:

```text
alloc_page: lock -> read head -> update head -> unlock -> return page
free_page:  lock -> insert page at head -> unlock
```

The lock is not held across `user_vm_map()`, ELF loading, or a whole system
call. Those operations may allocate additional page-table pages and would
otherwise attempt to acquire the same lock recursively.

`alloc_page()` uses `read_tp()` to select `vm_alloc_stage[hartid]` and to print
the correct hart ID for user allocations. A failed allocation reports an error
and does not advance a process's virtual heap cursor.

## 5. User-program loading and heap allocation

The command line is:

```text
spike -p2 obj/riscv-pke obj/app_alloc0 obj/app_alloc1
```

After the kernel argument is skipped, `parse_args()` produces:

```text
argv[0] = obj/app_alloc0
argv[1] = obj/app_alloc1
```

The loader selects `argv[hartid]`, so hart 0 loads `app_alloc0` and hart 1
loads `app_alloc1`. Every process receives its own trapframe, page-table root,
kernel stack, user stack mapping, and ELF mappings. Allocation of those
physical pages still goes through the shared locked allocator.

The user allocation path is:

```text
naive_malloc
  -> ECALL
  -> sys_user_allocate_page
  -> current[hartid]
  -> alloc_page
  -> proc->ufree_page
  -> user_vm_map(proc->pagetable, va, pa)
  -> increment proc->ufree_page
  -> return va
```

The cursor advances only after the mapping operation has succeeded. Freeing a
page unmaps it and returns its physical page to the shared allocator, but does
not move the simple monotonic virtual cursor backward.

`sys_user_free_page()` and the inherited user page-fault handler always use the
current hart's process and page table. Page-table manipulation is process-local;
only physical-page acquisition and release require the allocator lock.

## 6. Exit and failure behavior

`sys_user_exit(code)` records the exit and increments `exit_count` atomically.
Each hart waits until all `NCPU` applications have exited. Only hart 0 calls
`shutdown(code)`; other harts remain waiting until the machine is stopped.
This prevents an early exit from terminating an application that is still
reading or writing its allocated pages.

Out-of-memory or unrecoverable mapping failures stop the kernel with a clear
diagnostic. Challenge 1's restricted stack-address policy and Challenge 2's
single-page heap policy are intentionally outside this branch.

## 7. Verification

No agent-authored tests will be created or run. Verification consists of:

```bash
git diff --check
make clean && make march=-march=rv64imafd
spike --isa=rv64imafd -p2 obj/riscv-pke obj/app_alloc0 obj/app_alloc1
```

The run must show, independent of interleaving:

- each process receives virtual addresses `0x400000`, `0x401000`, ...,
  `0x404000`;
- `app_alloc0` reads back `0, 1, 2, 3, 4`;
- `app_alloc1` reads back `5, 6, 7, 8, 9`;
- user allocations on the two harts have distinct physical addresses;
- only one hart performs the final shutdown.
