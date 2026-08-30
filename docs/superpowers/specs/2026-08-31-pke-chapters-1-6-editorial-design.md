# PKE Chapters 1–6 Editorial Redesign

**Status:** Approved on 2026-08-31

## Objective

Rewrite Chapters 1–6 of `docs/pke-doc-master 2` in place so they teach the same complete body of material with clearer Chinese, lower cognitive load, and a consistent mechanism-first/lab-driven structure. The revised documents must remain useful as course notes, lab instructions, and later interview review material.

The baseline is commit `c30acbe`:

- Chapter 1: 946 lines
- Chapter 2: 496 lines
- Chapter 3: 2,015 lines
- Chapter 4: 1,257 lines
- Chapter 5: 1,601 lines
- Chapter 6: 2,763 lines
- Total: 9,078 lines

Line count is not a completion metric; it only describes the source scope. Completion requires semantic and structural coverage.

## Audience and Voice

The primary reader understands basic C but may have forgotten RISC-V and operating-system prerequisites. The documents must:

- explain one mechanism boundary at a time;
- separate current behaviour, intended behaviour, and the concrete execution sequence;
- introduce registers, data structures, and acronyms before relying on them;
- prefer causal language such as “because”, “therefore”, and “after this step”;
- use concrete addresses, register values, call chains, and state transitions where they reduce ambiguity;
- keep the tone direct and practical rather than formal or promotional;
- avoid unexplained jumps from theory to a large code listing.

## Unified Teaching Structure

### Chapter opening

Every chapter begins with:

1. what problem this chapter solves;
2. prerequisite concepts;
3. a short learning route;
4. questions the reader should be able to answer after finishing;
5. the original table of contents and numbered section coverage.

### Foundation sections

Conceptual sections follow this order where applicable:

1. **Problem:** what cannot be explained or implemented without this mechanism;
2. **Mental model:** the smallest accurate picture;
3. **Execution flow:** a concrete sequence involving registers, memory, processes, or files;
4. **Code connection:** the corresponding PKE files, functions, and data structures;
5. **Boundary and common mistake:** what this mechanism must not be confused with;
6. **Section summary:** the invariant the reader should retain.

### Lab sections

Each Lab and challenge follows this order while retaining its original requirements:

1. given application and observable behaviour;
2. target behaviour;
3. end-to-end call path;
4. implementation task and relevant TODO;
5. step-by-step reasoning;
6. why the implementation is written that way;
7. build/run or expected-output verification;
8. original reference answer or further guidance.

The rewrite may add subheadings inside an original section, but the original numbered Lab headings and link anchors must remain available.

## Chapter-Specific Teaching Focus

### Chapter 1 — RISC-V foundations

Build a progressive model from instructions and registers to privilege modes, traps, paging, and tools. Explain each register by its role in a concrete event rather than as an isolated definition.

### Chapter 2 — environment and repository workflow

Separate installation choices from verification. Every command block states its purpose, expected result, and the next diagnostic step when the result differs. Preserve all supported environment paths and platform material.

### Chapter 3 — traps and Lab 1

Make compilation/linking, PKE startup, ELF loading, and HTIF a single causal startup path. For each trap lab, distinguish the trapping instruction, the privilege mode that receives it, the saved context, the handler, and whether the PC advances or retries.

### Chapter 4 — memory and Lab 2

Use one address-translation mental model throughout: virtual address → VPN indexes/offset → PTE → physical page plus offset. Separate page-table lookup, mapping creation, physical allocation, page fault handling, and invalid-access rejection.

### Chapter 5 — processes and Lab 3

Introduce process state, trapframe, address space, and queues before scheduling operations. Show state transitions for fork, yield, round-robin, wait, semaphore blocking, and COW, and explicitly distinguish shared code from private or copy-on-write data.

### Chapter 6 — filesystems and Lab 4

Introduce VFS objects and RFS disk objects as two layers. Maintain a stable mapping among path, dentry, vinode, dinode, directory entry, and data block. Explain every lab through the user syscall → VFS → inode operation → RFS/device path.

## Content-Preservation Contract

Lower cognitive load must not be achieved by deleting difficult material.

The rewrite must preserve:

- every original numbered chapter, subsection, Lab, challenge, task, and bonus item;
- every experiment requirement, TODO, hint, reference answer, and “more guidance” section;
- every code listing and command body;
- every image reference and local media asset;
- every external link and course-resource link;
- every important warning, constraint, expected output, address, register, function, structure, and branch name;
- the teaching-document license and attribution.

Content may be reordered, split, paraphrased, or supplemented. If wording is replaced, every factual claim in the original section must remain represented in the rewritten section or an explicitly mapped subsection.

## Preservation Evidence

A verifier will compare the rewritten files with baseline commit `c30acbe`. It will record and check, per chapter:

- original numbered headings and explicit HTML anchors;
- fenced code block bodies and their order within each original numbered section;
- image targets;
- Markdown link targets;
- shell commands and expected-output blocks;
- Lab/challenge names and Educoder links.

A chapter-level coverage ledger will map each original section to its revised location and note any intentional reordering. Programmatic checks supplement, but do not replace, a manual section-by-section semantic review.

## Editing and Commit Strategy

Work stays on branch `docs/pke-doc-master-2-interview-guide`.

1. Create the preservation verifier and baseline manifests.
2. Rewrite Chapters 1–2 and run chapter checks.
3. Rewrite Chapters 3–4 and run chapter checks.
4. Rewrite Chapters 5–6 and run chapter checks.
5. Run the complete preservation audit and Markdown structural checks.
6. Review the final Git diff for source-code isolation.
7. Commit the final documentation changes and push the same GitHub branch.

Local commits may be created at chapter boundaries. The branch is pushed only after the full audit passes unless a recovery checkpoint is necessary.

## Verification

Completion requires all of the following:

- all six files are non-empty and parse as balanced Markdown fences;
- all original numbered sections have mapped revised sections;
- all original code bodies, image targets, link targets, and Lab names pass the preservation verifier;
- every local image/file link referenced by Chapters 1–6 resolves inside `docs/pke-doc-master 2`;
- `git diff --check` reports no new formatting errors in revised files;
- the diff from `c30acbe` changes documentation/audit files only;
- a manual review confirms each original prose section’s facts are represented;
- the GitHub branch points to the final local commit and exposes Chapters 1–6.

No kernel code or user application is changed, and no operating-system test suite is added.
