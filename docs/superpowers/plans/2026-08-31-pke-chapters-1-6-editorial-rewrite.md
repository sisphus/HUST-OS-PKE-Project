# PKE Chapters 1–6 Editorial Rewrite Implementation Plan

> **Execution requirement:** Use the `executing-plans` workflow in this session. Complete the tasks in order, verify each checkpoint from fresh command output, and do not ask the user to choose an execution style again.

**Goal:** Rewrite Chapters 1–6 under `docs/pke-doc-master 2/` using the approved hybrid teaching structure while preserving every original teaching requirement, example, command, code block, image, link, answer, and challenge.

**Architecture:** Treat commit `c30acbe` as an immutable source baseline. A repository-local verifier extracts structural and literal preservation evidence from the baseline and current files. Each chapter is rewritten in place, checked independently, and recorded in a coverage ledger. The branch is pushed only after the full six-chapter audit and documentation-only scope check pass.

**Stack:** Markdown, Python 3 standard library, Git, GitHub CLI/API, repository assets.

## Global constraints

- Work only on branch `docs/pke-doc-master-2-interview-guide`.
- Keep all six existing chapter filenames and their original numbered section identities.
- Use the approved structure from `docs/superpowers/specs/2026-08-31-pke-chapters-1-6-editorial-design.md`.
- Do not change kernel code, user applications, Makefiles, toolchain files, or binary assets.
- Do not add or run OS tests. Run documentation-specific verification only.
- Do not reduce complexity by deleting material. Reorder, split, paraphrase, and supplement it instead.
- Preserve the original teaching-document license and attribution.
- Do not push intermediate chapter commits. Push only after the complete audit passes.

### Task 1: Build the content-preservation verifier

**Files:**

- Create: `tools/verify_pke_docs.py`
- Create: `docs/pke-doc-master 2/editorial-baseline.json`
- Verify: all six chapter Markdown files against commit `c30acbe`

**Step 1: Define the six-file manifest**

Map chapter numbers to these paths:

- `docs/pke-doc-master 2/chapter1_riscv.md`
- `docs/pke-doc-master 2/chapter2_installation.md`
- `docs/pke-doc-master 2/chapter3_traps.md`
- `docs/pke-doc-master 2/chapter4_memory.md`
- `docs/pke-doc-master 2/chapter5_process.md`
- `docs/pke-doc-master 2/chapter6_filesystem.md`

The verifier CLI must support:

```sh
python3 tools/verify_pke_docs.py --baseline c30acbe
python3 tools/verify_pke_docs.py --baseline c30acbe --chapter 3
python3 tools/verify_pke_docs.py --baseline c30acbe --write-report docs/pke-doc-master\ 2/editorial-audit.json
```

**Step 2: Extract preservation evidence**

For each baseline and worktree chapter, extract:

- numbered Markdown headings and Lab/challenge names;
- explicit HTML anchors;
- fenced-code bodies, without requiring the same fence language label;
- Markdown image targets;
- Markdown link targets;
- Educoder URLs and other bare HTTP(S) URLs;
- local linked paths.

Use multisets where duplicates matter. Baseline evidence must be a subset of current evidence. Preserve fenced-code bodies byte-for-byte after normalizing line endings and only the outer blank lines. Do not weaken failures into warnings.

**Step 3: Validate Markdown and local assets**

Check that:

- every fence is closed;
- each chapter is non-empty;
- each local image or file target exists relative to its chapter or documentation root;
- external URLs are retained without requiring network access.

**Step 4: Produce a machine-readable report**

The JSON report must contain the baseline commit, each chapter path, counts, missing items, unresolved local links, and overall pass/fail status. Make output deterministic so later diffs are reviewable.

**Step 5: Prove the verifier on the untouched baseline**

Run:

```sh
python3 tools/verify_pke_docs.py --baseline c30acbe --write-report 'docs/pke-doc-master 2/editorial-baseline.json'
git diff --check
```

Expected: all six chapters pass because the current chapter contents still match the baseline at this checkpoint.

**Step 6: Review and commit**

Inspect the report and script, then commit:

```sh
git add tools/verify_pke_docs.py 'docs/pke-doc-master 2/editorial-baseline.json'
git commit -m "docs: add PKE content preservation audit"
```

### Task 2: Rewrite Chapter 1 — RISC-V foundations

**Files:**

- Modify: `docs/pke-doc-master 2/chapter1_riscv.md`
- Update later: `docs/pke-doc-master 2/EDITORIAL_COVERAGE.md`

**Step 1: Add the chapter learning map**

Open with the problem solved by the chapter, prerequisites, the route from instruction execution to traps and paging, and concrete end questions. Retain the original table of contents and section coverage.

**Step 2: Rebuild Sections 1.1–1.6**

Teach in this dependency order:

1. instruction execution and register roles;
2. RISC-V ISA and ABI boundaries;
3. privilege modes and CSR purpose;
4. trap entry/return as a concrete register-state transition;
5. paging as virtual address → PTE → physical address plus offset;
6. tools and PKE connection.

Within each original section use: problem → mental model → execution flow → PKE/code connection → boundary/common mistake → summary.

**Step 3: Preserve examples and literal artifacts**

Keep every original command, code block, image, link, address, register name, warning, and reference. Place a short interpretation before dense listings and a retained takeaway after them.

**Step 4: Verify and manually compare**

Run:

```sh
python3 tools/verify_pke_docs.py --baseline c30acbe --chapter 1
git diff --check -- 'docs/pke-doc-master 2/chapter1_riscv.md'
git diff --stat c30acbe -- 'docs/pke-doc-master 2/chapter1_riscv.md'
```

Read the baseline and rewrite section by section and record that every original prose fact is still represented.

**Step 5: Commit the chapter checkpoint**

```sh
git add 'docs/pke-doc-master 2/chapter1_riscv.md'
git commit -m "docs: rewrite PKE RISC-V foundations"
```

### Task 3: Rewrite Chapter 2 — environment and workflow

**Files:**

- Modify: `docs/pke-doc-master 2/chapter2_installation.md`

**Step 1: Add a setup decision map**

Explain which environment paths exist, who each path is for, what is shared among them, and how the reader knows setup is complete.

**Step 2: Rebuild Sections 2.1–2.3**

For every command group, state:

1. its purpose;
2. the expected observable result;
3. what a mismatch means;
4. the next diagnostic command or section.

Keep platform-specific instructions, repository workflow, branch names, download links, and all original alternatives.

**Step 3: Verify and commit**

Run the chapter-2 verifier, `git diff --check` on the file, and a manual baseline comparison. Then commit:

```sh
git add 'docs/pke-doc-master 2/chapter2_installation.md'
git commit -m "docs: rewrite PKE environment guide"
```

### Task 4: Rewrite Chapter 3 — traps and Lab 1

**Files:**

- Modify: `docs/pke-doc-master 2/chapter3_traps.md`

**Step 1: Establish one startup causal path**

Connect compilation, linking, entry code, machine-mode startup, supervisor/user transition, ELF loading, HTIF, and application launch. At each boundary show the current privilege mode, important register or CSR, and next function.

**Step 2: Rebuild Sections 3.1.1–3.1.6**

Use concrete execution records for each mechanism. Distinguish:

- the instruction or interrupt that triggers a trap;
- the mode in which it occurs;
- the mode that receives it;
- where register context is saved;
- which handler runs;
- whether `sepc`/`mepc` advances or retries.

**Step 3: Rebuild Labs 3.2–3.7**

For every Lab/challenge retain its branch, application, expected output, TODO, hint, answer, and further guidance. Present each as:

given application → target → call chain → TODO → reasoning → why → verification → retained answer.

Use the concrete learning points already present in the source, including syscall return values, illegal-instruction retry loops, timer-pending-bit clearing, stack backtracing, ELF symbols/line tables, multi-hart startup, per-hart state, barriers, timer initialization, `tp`, and global shutdown ownership.

**Step 4: Verify and commit**

Run the chapter-3 verifier, fence/link checks, `git diff --check`, and manual Lab-by-Lab comparison. Then commit:

```sh
git add 'docs/pke-doc-master 2/chapter3_traps.md'
git commit -m "docs: rewrite PKE traps and Lab 1 guide"
```

### Task 5: Rewrite Chapter 4 — memory and Lab 2

**Files:**

- Modify: `docs/pke-doc-master 2/chapter4_memory.md`

**Step 1: Establish one address-translation model**

Use the same notation throughout:

virtual address → VPN indexes plus page offset → selected PTE → physical page base plus unchanged offset.

Explain what belongs to a page table, process, PTE, page allocator, and fault record before combining them.

**Step 2: Rebuild Sections 4.1.1–4.1.4**

Separate lookup from creation, mapping from allocation, and translation from permission checking. Show why a query path must not allocate missing intermediate page tables.

**Step 3: Rebuild Labs 4.2–4.7**

Preserve all branches, TODOs, commands, expected faults, addresses, answers, and challenges. Explicitly trace:

- existing VA-to-PA lookup using the supplied page table;
- unmap order and recovering the old physical page before clearing a PTE;
- store page faults for previously unmapped pages;
- invalid-range rejection using `stval` and address-space boundaries;
- small-block allocation where one block is freed but the shared physical page remains mapped;
- multi-hart address spaces, per-process page tables, locks, per-hart trapframes/apps, and synchronized shutdown.

**Step 4: Verify and commit**

Run the chapter-4 verifier, `git diff --check`, and a manual Lab-by-Lab comparison. Then commit:

```sh
git add 'docs/pke-doc-master 2/chapter4_memory.md'
git commit -m "docs: rewrite PKE memory and Lab 2 guide"
```

### Task 6: Rewrite Chapter 5 — processes and Lab 3

**Files:**

- Modify: `docs/pke-doc-master 2/chapter5_process.md`

**Step 1: Introduce the persistent process model**

Define process state, trapframe, page table/address space, mapped-region records, parent/child relationship, and ready/blocked queues before scheduling operations.

**Step 2: Rebuild Sections 5.1.1–5.1.3**

Use state tables and concrete transitions only where they materially clarify ownership and scheduling. Keep the original section numbering and source references.

**Step 3: Rebuild Labs 5.2–5.7**

For fork, yield, round-robin, wait, semaphore, and COW, show:

- state before the operation;
- the exact queue/page-table/trapframe mutation;
- state after the operation;
- the return value observed by parent and child;
- the ownership rule that prevents process or memory loss.

Preserve all original distinctions, including parent versus child page tables, mapped-info bookkeeping, FIFO ready queues, blocked/ready transitions, private data pages, wait return behavior, per-semaphore wait queues, semaphore count rules, COW permission removal from both PTEs, old physical-page lookup, write-fault handling, reference sharing, and private-page creation.

**Step 4: Verify and commit**

Run the chapter-5 verifier, `git diff --check`, and manual Lab-by-Lab comparison. Then commit:

```sh
git add 'docs/pke-doc-master 2/chapter5_process.md'
git commit -m "docs: rewrite PKE processes and Lab 3 guide"
```

### Task 7: Rewrite Chapter 6 — filesystems and Lab 4

**Files:**

- Modify: `docs/pke-doc-master 2/chapter6_filesystem.md`

**Step 1: Establish the two-layer object map**

Introduce and repeatedly preserve this distinction:

- runtime/VFS layer: path, dentry, vinode, file, inode operations, superblock;
- persistent/RFS layer: dinode, inode number, directory entry, RFS block number, data block, RAM-disk device.

Show how `vinode->i_ops` selects the implementation, such as `hostfs_i_ops` versus RFS operations.

**Step 2: Rebuild Sections 6.1.1–6.1.5.3**

Explain object lifetime, disk persistence, lookup, read/write offsets, block allocation, directory representation, and multi-page file data using concrete paths and structure fields.

**Step 3: Rebuild Labs/challenges 6.2–6.8**

Trace every task through:

user syscall → VFS entry → path/dentry/vinode lookup → inode operation → RFS or host device action.

Retain all original branches, TODOs, commands, expected outputs, code listings, hints, answers, challenges, and bonus material. Explicitly preserve the reasoning around new dinode persistence, runtime `vinode` initialization, directory iteration and offset progression, empty-directory behavior, inode/block numbering, and files spanning multiple blocks/pages.

**Step 4: Verify and commit**

Run the chapter-6 verifier, `git diff --check`, and manual Lab-by-Lab comparison. Then commit:

```sh
git add 'docs/pke-doc-master 2/chapter6_filesystem.md'
git commit -m "docs: rewrite PKE filesystems and Lab 4 guide"
```

### Task 8: Add navigation and the manual coverage ledger

**Files:**

- Modify: `docs/pke-doc-master 2/README.zh-CN.md`
- Create: `docs/pke-doc-master 2/EDITORIAL_COVERAGE.md`
- Create: `docs/pke-doc-master 2/editorial-audit.json`

**Step 1: Add “如何使用这套文档”**

Explain the recommended first-reading route, Lab implementation route, and interview-review route. Link Chapters 1–6 and the interview guide without changing the existing license or project attribution.

**Step 2: Complete the coverage ledger**

For every original numbered section and Lab/challenge record:

- baseline heading;
- revised heading/location;
- teaching structure used;
- intentional reordering, if any;
- manual semantic review status.

Do not mark a row complete until the original prose and revised prose have been compared directly.

**Step 3: Generate the final audit report**

Run:

```sh
python3 tools/verify_pke_docs.py --baseline c30acbe --write-report 'docs/pke-doc-master 2/editorial-audit.json'
```

Expected: overall pass with no missing headings, anchors, code bodies, image/link targets, URLs, or local assets.

**Step 4: Commit navigation and audit artifacts**

```sh
git add 'docs/pke-doc-master 2/README.zh-CN.md' \
  'docs/pke-doc-master 2/EDITORIAL_COVERAGE.md' \
  'docs/pke-doc-master 2/editorial-audit.json'
git commit -m "docs: add PKE teaching routes and coverage audit"
```

### Task 9: Perform final audit and push

**Step 1: Run complete documentation verification**

```sh
python3 tools/verify_pke_docs.py --baseline c30acbe --write-report 'docs/pke-doc-master 2/editorial-audit.json'
git diff --check c30acbe..HEAD
git status --short
```

The generated report must match the committed report. The status must be clean.

**Step 2: Prove the diff is documentation-only**

```sh
git diff --name-only c30acbe..HEAD
```

Every changed path must be inside one of:

- `docs/pke-doc-master 2/`
- `docs/superpowers/specs/`
- `docs/superpowers/plans/`
- `tools/verify_pke_docs.py`

Reject the release if any kernel, user, Makefile, or binary-asset path changed.

**Step 3: Review the complete commit series**

```sh
git log --oneline c30acbe..HEAD
git diff --stat c30acbe..HEAD
```

Read the six chapter openings, every Lab/challenge heading, the coverage ledger, and all verifier failures/warnings one final time.

**Step 4: Push the documentation branch**

```sh
git push github docs/pke-doc-master-2-interview-guide
```

**Step 5: Verify the remote result**

Compare local and remote SHAs, then use the GitHub API or `gh api` to confirm the branch exposes at least:

- `docs/pke-doc-master 2/chapter1_riscv.md`
- `docs/pke-doc-master 2/chapter6_filesystem.md`
- `docs/pke-doc-master 2/EDITORIAL_COVERAGE.md`
- `docs/PKE_OS_interview_guide.md`

Report the branch URL, final SHA, six-chapter audit result, and documentation-only scope result.
