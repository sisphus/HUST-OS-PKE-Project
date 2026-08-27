# Lab 2 Challenge 3 Multicore Memory Management Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `lab2_challenge3_multicoremem` run `app_alloc0` and `app_alloc1` on two harts with isolated process virtual state, serialized shared physical-page allocation, synchronized startup, and one final shutdown.

**Architecture:** Keep the existing course-kernel structure. Store machine trap frames, `current`, timer counters, and allocation-stage flags in per-hart arrays indexed by the hart ID in `tp`; store the virtual heap cursor in each `process`; protect only the shared physical free-list mutation with an `amoswap` spinlock. Hart 0 performs one-time HTIF/DTB, physical-memory, and kernel-page-table initialization, while independent barriers release all harts into per-process loading and execution.

**Tech Stack:** C and RISC-V assembly; GCC `riscv64-unknown-elf-gcc`; RISC-V `amoswap.w`; Spike with `--isa=rv64imafd`; GNU Make; Git.

## Global Constraints

- Keep `NCPU` equal to `2`.
- Base this branch on `lab2_3_pagefault`; do not merge the other challenge branches or `lab1_challenge3_multicore`.
- Use `tp` to carry the hart ID into supervisor-mode code and use `read_tp()` for per-hart lookup.
- Keep `g_itrframe`, `current`, timer accounting, and allocation-stage state per hart.
- Keep each process's page table and virtual heap cursor independent; both first heap allocations must be virtual address `0x400000`.
- Protect the shared physical free-page list with an atomic spinlock, but do not hold that lock across `user_vm_map()` or a complete system call.
- Initialize shared HTIF/DTB, physical-memory, and kernel-page-table state once, then release all harts with separate monotonic barriers.
- Select `argv[hartid]` after the kernel argument is removed, so hart 0 runs `app_alloc0` and hart 1 runs `app_alloc1`.
- Wait for both user applications before only hart 0 executes `shutdown()`.
- Do not create or run agent-authored tests or add a test framework.
- Verify with `git diff --check`, `make clean && make march=-march=rv64imafd`, and `spike --isa=rv64imafd -p2 obj/riscv-pke obj/app_alloc0 obj/app_alloc1`.
- After source implementation and runtime verification, create the explanatory document in the outer workspace folder `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/lab2_challenge3_multicoremem_why/README.md`.

---

## File map

| File | Responsibility |
|---|---|
| `kernel/sync_utils.h` | Existing barrier plus atomic spinlock helpers |
| `kernel/pmm.c` | Shared physical free-list lock and per-hart allocation logging |
| `kernel/machine/minit.c` | Per-hart machine trap frame, one-time machine startup, per-hart timer setup |
| `kernel/process.h` | Per-process heap cursor, per-hart `current` declaration and accessor |
| `kernel/process.c` | Per-hart current-process selection and user-mode switch setup |
| `kernel/kernel.c` | Per-hart process array, one-time S-mode memory initialization, process construction |
| `kernel/elf.h` | Loader signature carrying the selected application index |
| `kernel/elf.c` | Select and open the application for the current hart |
| `kernel/syscall.c` | Process-relative page allocation/free and synchronized exit |
| `kernel/strap.c` | Per-hart timer accounting and process-relative trap/page-fault handling |
| `lab2_challenge3_multicoremem_why/README.md` | Post-implementation explanation with source references |

The existing `kernel/machine/mentry.S` and `kernel/machine/mtrap_vector.S` already
calculate the per-hart machine stack and use `mscratch` as the trap-frame
pointer. Re-read them after the C changes; modify them only if a concrete
symbol or layout error appears.

## Task 1: Add synchronization and isolate machine-mode resources

**Files:**

- Modify: `kernel/sync_utils.h`
- Modify: `kernel/pmm.c`
- Modify: `kernel/machine/minit.c`
- Inspect: `kernel/machine/mentry.S`
- Inspect: `kernel/machine/mtrap_vector.S`

**Interfaces:**

- Consumes: existing `sync_barrier(volatile int *counter, int all)`, `NCPU`, `read_tp()`, and `g_free_mem_list`.
- Produces: `spin_lock(volatile int *)`, `spin_unlock(volatile int *)`, `g_itrframe[NCPU]`, `m_boot_count`, and serialized free-list mutation.

- [ ] **Step 1: Add atomic lock helpers to `kernel/sync_utils.h`.**

Keep the existing barrier and append:

```c
static inline void spin_lock(volatile int *lock) {
  int old;
  do {
    asm volatile("amoswap.w.aq %0, %2, (%1)"
                 : "=r"(old)
                 : "r"(lock), "r"(1)
                 : "memory");
  } while (old != 0);
}

static inline void spin_unlock(volatile int *lock) {
  asm volatile("amoswap.w.rl x0, %1, (%0)"
               :
               : "r"(lock), "r"(0)
               : "memory");
}
```

The lock value is zero when free and one when held. Acquire must make the
read-and-write indivisible; a C check followed by an assignment is not enough.

- [ ] **Step 2: Protect `g_free_mem_list` in `kernel/pmm.c`.**

Include `sync_utils.h` and add:

```c
static volatile int g_pmm_lock = 0;
```

In `free_page()`, hold the lock only while inserting the validated page:

```c
spin_lock(&g_pmm_lock);
list_node *n = (list_node *)pa;
n->next = g_free_mem_list.next;
g_free_mem_list.next = n;
spin_unlock(&g_pmm_lock);
```

In `alloc_page()`, hold it only while reading and updating the head:

```c
spin_lock(&g_pmm_lock);
list_node *n = g_free_mem_list.next;
if (n != 0)
  g_free_mem_list.next = n->next;
spin_unlock(&g_pmm_lock);
```

Use `uint64 hartid = read_tp();` for the existing user-allocation log and
`vm_alloc_stage[hartid]`. Do not hold the lock while printing, mapping, or
loading an ELF, because those paths can allocate additional pages.

- [ ] **Step 3: Make the machine trap frame per hart.**

In `kernel/machine/minit.c`, replace:

```c
riscv_regs g_itrframe;
```

with:

```c
riscv_regs g_itrframe[NCPU];
```

Add:

```c
static volatile int m_boot_count = 0;
```

Include `kernel/sync_utils.h`.

- [ ] **Step 4: Synchronize `m_start()`.**

At the start of `m_start(uintptr_t hartid, uintptr_t dtb)`, call
`write_tp(hartid)`. Let only hart 0 call `spike_file_init()` and
`init_dtb(dtb)`, then call:

```c
sync_barrier(&m_boot_count, NCPU);
```

After the barrier, configure each hart's own `mscratch`, `mtvec`,
delegation, interrupt enables, and timer. In particular:

```c
write_csr(mscratch, &g_itrframe[hartid]);
write_csr(mtvec, (uint64)mtrapvec);
timerinit(hartid);
```

Preserve the existing `mret` transition to `s_start()`.

- [ ] **Step 5: Compile the machine and allocator changes.**

```bash
make clean && make march=-march=rv64imafd
```

Expected: all kernel and user objects compile and link. The two-hart behavior
is not considered complete until the remaining global process state is removed.

- [ ] **Step 6: Commit Task 1.**

```bash
git diff --check
git add kernel/sync_utils.h kernel/pmm.c kernel/machine/minit.c
git commit -m "feat: isolate machine traps and lock physical pages"
```

## Task 2: Create per-hart processes and select the correct application

**Files:**

- Modify: `kernel/process.h`
- Modify: `kernel/process.c`
- Modify: `kernel/kernel.c`
- Modify: `kernel/elf.h`
- Modify: `kernel/elf.c`

**Interfaces:**

- Consumes: `read_tp()`, Task 1 synchronization, the locked allocator, and
  `parse_args()`.
- Produces: `process user_app[NCPU]`, `process *current[NCPU]`,
  `get_current_process()`, `process.ufree_page`, and
  `load_bincode_from_host_elf(process *, uint64)`.

- [ ] **Step 1: Extend `kernel/process.h`.**

Include `config.h`, add the cursor, and replace the single-process declarations:

```c
typedef struct process_t {
  uint64 kstack;
  pagetable_t pagetable;
  trapframe *trapframe;
  uint64 ufree_page;
} process;

extern process *current[NCPU];

static inline process *get_current_process(void) {
  return current[read_tp()];
}
```

Remove the old `g_ufree_page` declaration.

- [ ] **Step 2: Convert `kernel/process.c`.**

Define:

```c
process *current[NCPU] = { 0 };
```

Remove the global heap cursor. At the start of `switch_to(process *proc)`,
record:

```c
uint64 hartid = read_tp();
current[hartid] = proc;
```

Keep the existing per-hart CSR setup and `return_to_user()` call.

- [ ] **Step 3: Build one process per hart in `kernel/kernel.c`.**

Replace the scalar process with:

```c
process user_app[NCPU];
```

Change `load_user_program` to receive `uint64 hartid`. Initialize:

```c
proc->ufree_page = USER_FREE_ADDRESS_START;
proc->trapframe->regs.tp = hartid;
```

Keep each process's page-table root, trapframe, kernel stack, user stack, ELF
mappings, and user trap-vector mapping separate. Keep one shared kernel page
table.

- [ ] **Step 4: Make S-mode shared initialization one-time.**

Include `config.h` and `sync_utils.h`, add:

```c
static volatile int s_boot_count = 0;
```

Use `uint64 hartid = read_tp();`. Every hart clears its own `satp`; only
hart 0 calls `pmm_init()` and `kern_vm_init()`. Then all harts call:

```c
sync_barrier(&s_boot_count, NCPU);
```

Only after the barrier call `enable_paging()`, load
`user_app[hartid]`, set that hart's allocation stage, and call
`switch_to(&user_app[hartid])`.

- [ ] **Step 5: Pass the application index to the ELF loader.**

Change the declaration in `kernel/elf.h` to:

```c
void load_bincode_from_host_elf(process *p, uint64 app_index);
```

Change the definition in `kernel/elf.c` and replace the existing application
selection statements with these exact changes:

```c
- void load_bincode_from_host_elf(process *p) {
+ void load_bincode_from_host_elf(process *p, uint64 app_index) {
```

Immediately after `size_t argc = parse_args(&arg_bug_msg);`, add:

```c
if (app_index >= argc)
  panic("No application argument for this hart.\n");
char *app = arg_bug_msg.argv[app_index];
```

Replace the application log and file-open argument:

```c
- sprint("hartid = ?: Application: %s\n", arg_bug_msg.argv[0]);
+ sprint("hartid = %ld: Application: %s\n", read_tp(), app);

- info.f = spike_file_open(arg_bug_msg.argv[0], O_RDONLY, 0);
+ info.f = spike_file_open(app, O_RDONLY, 0);
```

Keep the existing declarations for `elfloader` and `info`, the `info.p = p`
assignment, file-open error check, ELF initialization, and ELF loading. Only
the function signature, argument validation, selected application pointer, and
application log/open argument change.

- [ ] **Step 6: Compile and commit Task 2.**

```bash
make clean && make march=-march=rv64imafd
git diff --check
git add kernel/process.h kernel/process.c kernel/kernel.c kernel/elf.h kernel/elf.c
git commit -m "feat: isolate multicore process state"
```

## Task 3: Make traps, heap syscalls, and exit process-relative

**Files:**

- Modify: `kernel/syscall.c`
- Modify: `kernel/strap.c`

**Interfaces:**

- Consumes: `get_current_process()`, `process.ufree_page`,
  `current[NCPU]`, `vm_alloc_stage[NCPU]`, and the locked allocator.
- Produces: process-relative heap allocation/free, per-hart timer accounting,
  and synchronized exit.

- [ ] **Step 1: Update heap syscalls in `kernel/syscall.c`.**

Use a local process pointer:

```c
process *proc = get_current_process();
assert(proc);
```

Implement `sys_user_allocate_page()` in this order:

```c
void *pa = alloc_page();
if (pa == 0)
  panic("cannot allocate a physical page for user heap.\n");

uint64 va = proc->ufree_page;
user_vm_map((pagetable_t)proc->pagetable, va, PGSIZE, (uint64)pa,
            prot_to_type(PROT_WRITE | PROT_READ, 1));
proc->ufree_page += PGSIZE;

sprint("hartid = %ld: vaddr 0x%lx is mapped to paddr 0x%lx\n",
       read_tp(), va, (uint64)pa);
return va;
```

Use `proc->pagetable` in `sys_user_free_page()`. Do not move the cursor
backward when a page is freed.

- [ ] **Step 2: Update supervisor trap handling in `kernel/strap.c`.**

At the start of `smode_trap_handler()`, bind:

```c
process *proc = get_current_process();
assert(proc);
```

Use `proc->trapframe` for `sepc`, syscall dispatch, and
`switch_to(proc)`. Pass `proc->pagetable` to the inherited page-fault
mapping path. Do not add Challenge 1's restricted stack policy.

- [ ] **Step 3: Make timer accounting per hart.**

Replace the scalar counter with:

```c
static uint64 g_ticks[NCPU] = { 0 };
```

Use `uint64 hartid = read_tp();` in `handle_mtimer_trap()`, increment
`g_ticks[hartid]`, and clear the current hart's `SIP_SSIP`.

- [ ] **Step 4: Synchronize application exit.**

Include `config.h` and `sync_utils.h` in `kernel/syscall.c`, add:

```c
static volatile int exit_count = 0;
```

In `sys_user_exit(uint64 code)`, print the current hart, call:

```c
sync_barrier(&exit_count, NCPU);
if (read_tp() == 0)
  shutdown(code);
for (;;)
  ;
```

Thus the first exiting hart cannot stop the other application.

- [ ] **Step 5: Compile and commit Task 3.**

```bash
make clean && make march=-march=rv64imafd
git diff --check
git add kernel/syscall.c kernel/strap.c
git commit -m "feat: make multicore heap and traps process-relative"
```

## Task 4: Verify the two-hart workload

**Files:**

- Inspect: `Makefile`
- Inspect: `user/app_alloc0.c`
- Inspect: `user/app_alloc1.c`
- Modify only a file from Tasks 1–3 if a concrete scoped defect is found

**Interfaces:**

- Consumes: the complete two-hart kernel and existing allocation applications.
- Produces: observed proof of virtual-address isolation, physical-page
  uniqueness, value preservation, and coordinated shutdown.

- [ ] **Step 1: Build from clean objects.**

```bash
make clean && make march=-march=rv64imafd
```

Expected outputs: `obj/riscv-pke`, `obj/app_alloc0`, and
`obj/app_alloc1` are linked.

- [ ] **Step 2: Run both applications on two harts.**

```bash
spike --isa=rv64imafd -p2 obj/riscv-pke obj/app_alloc0 obj/app_alloc1
```

Regardless of line interleaving, confirm:

```text
hart0 virtual heap: 0x00400000, 0x00401000, 0x00402000, 0x00403000, 0x00404000
hart1 virtual heap: 0x00400000, 0x00401000, 0x00402000, 0x00403000, 0x00404000
app_alloc0 values: 0, 1, 2, 3, 4
app_alloc1 values: 5, 6, 7, 8, 9
```

The allocator logs must show distinct physical pages for the two harts, and
the final shutdown must occur once after both exit messages.

- [ ] **Step 3: If the run fails, classify before editing.**

```bash
git status --short
rg -n "current->|g_ufree_page|g_itrframe|uint64 hartid = 0|shutdown|g_free_mem_list" kernel
```

Map the failure to per-hart trap state, current process, process cursor, loader
selection, physical-list lock, startup barrier, or exit barrier. Do not merge
another challenge branch or add unrelated behavior.

- [ ] **Step 4: Rebuild and rerun after a scoped fix.**

```bash
make clean && make march=-march=rv64imafd
spike --isa=rv64imafd -p2 obj/riscv-pke obj/app_alloc0 obj/app_alloc1
```

- [ ] **Step 5: Commit only if this task changed source.**

```bash
git diff --check
git status --short
git commit -am "verify: support multicore memory challenge"
```

Do not create an empty commit.

## Task 5: Write the requested post-implementation explanation

**Files:**

- Create: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/lab2_challenge3_multicoremem_why/README.md`
- Reference: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/kernel/sync_utils.h`
- Reference: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/kernel/pmm.c`
- Reference: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/kernel/machine/minit.c`
- Reference: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/kernel/process.h`
- Reference: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/kernel/process.c`
- Reference: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/kernel/kernel.c`
- Reference: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/kernel/elf.c`
- Reference: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/kernel/syscall.c`
- Reference: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/kernel/strap.c`

**Interfaces:**

- Consumes: the final source diff and verified Spike output.
- Produces: a Chinese explanation of why each change is needed, with links to actual source lines.

- [ ] **Step 1: Create the outer README with `apply_patch`.**

Explain, in this order:

1. why equal virtual addresses are valid across independent page tables;
2. why `g_free_mem_list` needs an atomic lock;
3. why `g_itrframe`, `current`, timers, and `tp` are per hart;
4. why shared startup initialization needs barriers;
5. why `argv[hartid]` selects the correct application;
6. why the heap cursor belongs to `process`;
7. why exit waits for both harts;
8. how the final build and Spike command verify the behavior.

Link each mechanism to the corresponding absolute source path and function or
symbol. Describe the implementation actually present in the final diff.

- [ ] **Step 2: Review the README against the final diff.**

Confirm that it describes no absent behavior and makes no claim that tests were
run.

- [ ] **Step 3: Commit the explanation.**

```bash
git add /Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/lab2_challenge3_multicoremem_why/README.md
git commit -m "docs: explain multicore memory management"
```

## Task 6: Final verification and publish

**Files:**

- Inspect: all implementation files in the file map and the outer README

**Interfaces:**

- Consumes: all committed source and documentation changes.
- Produces: a clean, verified, pushed `lab2_challenge3_multicoremem` branch.

- [ ] **Step 1: Check final status and whitespace.**

```bash
git status --short --branch
git diff --check
```

Expected: no uncommitted changes and no whitespace diagnostic.

- [ ] **Step 2: Run the fresh clean build.**

```bash
make clean && make march=-march=rv64imafd
```

Expected: the kernel and both applications compile and link from a clean object
directory.

- [ ] **Step 3: Run the fresh two-hart workload.**

```bash
spike --isa=rv64imafd -p2 obj/riscv-pke obj/app_alloc0 obj/app_alloc1
```

Confirm all invariants from Task 4, including distinct physical pages and one
shutdown after both exits.

- [ ] **Step 4: Push the completed branch.**

```bash
git push github lab2_challenge3_multicoremem
```

- [ ] **Step 5: Report exact evidence.**

Report the implementation commit(s), README path, clean build result, Spike
command, and observed invariant output. Do not report tests because none were
authorized or run.
