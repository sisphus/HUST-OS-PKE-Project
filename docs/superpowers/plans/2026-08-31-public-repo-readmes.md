# Public Repository and Bilingual README Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use inline execution with checkpoints for this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Publish `sisphus/HUST-OS-PKE-Project`, make `lab4_3_hardlink` its default branch, and replace the upstream README with accurate Chinese and English project documentation.

**Architecture:** Keep the implementation scope in the nested `riscv-pke` Git repository. Use `README.md` as the Chinese landing page and `README_EN.md` as the English counterpart, with reciprocal links. Apply GitHub repository settings only after local content and credential checks pass.

**Tech Stack:** Markdown, Git, GitHub CLI (`gh`), GitHub SSH remote.

## Global Constraints

- The exact repository is `sisphus/HUST-OS-PKE-Project`.
- The default branch after the change is `lab4_3_hardlink`.
- The repository visibility after the change is public.
- Describe only the verified Lab1–Lab4.3 scope.
- Do not claim `lab2_challenge2_singlepageheap` or later Lab4/Lab5 branches are implemented.
- Do not change kernel behavior or add tests.
- Preserve `LICENSE.txt` and existing branch history.

---

### Task 1: Write the bilingual repository README

**Files:**
- Modify: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/README.md`
- Create: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/README_EN.md`

**Interfaces:**
- Both README files describe the same project facts and completed scope.
- `README.md` links to `README_EN.md`; `README_EN.md` links to `README.md`.

- [ ] **Step 1: Replace the Chinese landing page**

Write `README.md` with these sections in this order:

1. Title and language links;
2. Project overview: RISC-V PKE proxy kernel running on Spike;
3. Implemented scope: system calls, M-mode traps, timer interrupts, backtrace, DWARF source-line lookup, multicore state, Sv39 memory, page faults, fork, scheduling, wait, semaphores, COW, VFS/RFS, directories, and hard links;
4. Architecture diagram showing M-mode, S-mode, U-mode, and trap delegation;
5. Completed branch table for Lab1 through Lab4.3;
6. Build prerequisites and tested build command;
7. Single-hart `app_hardlink` run command and expected result;
8. Two-hart run command and what to verify;
9. Repository structure;
10. Known limitations and explicit out-of-scope branches;
11. Links to source files and `LICENSE.txt`.

Use this tested build/run sequence:

```bash
make clean
make march=-march=rv64imafd
spike --isa=rv64imafd obj/riscv-pke obj/app_hardlink
```

State that `rv64imafd` is used because the older PKE startup code has compatibility issues with the current Spike default compressed-ISA configuration. State that the displayed `All tests passed!` line comes from the course application; no new tests are added by this project.

- [ ] **Step 2: Add the English counterpart**

Write `README_EN.md` with the same section order and technical facts in English. Use the same commands, branch names, source paths, and limitations. Include a link back to `README.md`.

- [ ] **Step 3: Check documentation content**

Run:

```bash
test -s README.md
test -s README_EN.md
rg -n "lab4_3_hardlink|lab2_challenge2_singlepageheap|rv64imafd|README_EN" README.md README_EN.md
git diff --check
```

Expected result: both files exist and are non-empty, both language links and scope markers are present, and `git diff --check` prints no errors.

- [ ] **Step 4: Commit the README changes**

Run:

```bash
git add README.md README_EN.md
git commit -m "docs: add bilingual project README"
```

Expected result: one commit containing only the two README files.

---

### Task 2: Push the latest branch and verify the public-facing files

**Files:**
- Read: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/README.md`
- Read: `/Users/weileipeng/Desktop/gradCourses/MIT6.1810-OS/PKE/riscv-pke/README_EN.md`

**Interfaces:**
- Consumes the README commit from Task 1.
- Produces `github/lab4_3_hardlink` containing both README files.

- [ ] **Step 1: Confirm the commit scope and source state**

Run:

```bash
git status --short --branch
git show --stat --oneline HEAD
git diff HEAD^ HEAD -- README.md README_EN.md
```

Expected result: the only new content in the latest commit is the two README files, and no kernel file is modified.

- [ ] **Step 2: Push the latest branch**

Run:

```bash
git push github HEAD:lab4_3_hardlink
```

Expected result: GitHub accepts the update to `refs/heads/lab4_3_hardlink`.

- [ ] **Step 3: Verify the remote commit and file paths**

Run:

```bash
git ls-remote github refs/heads/lab4_3_hardlink
git show HEAD:README.md | sed -n '1,40p'
git show HEAD:README_EN.md | sed -n '1,40p'
```

Expected result: the remote branch points to the local HEAD commit, and both README files are readable from that commit.

---

### Task 3: Change GitHub visibility and default branch

**Files:**
- External repository settings: `sisphus/HUST-OS-PKE-Project`

**Interfaces:**
- Consumes the verified repository name, pushed `lab4_3_hardlink` branch, and authenticated `gh` session.
- Produces a public repository whose default branch is `lab4_3_hardlink`.

- [ ] **Step 1: Run the final tracked-file safety check**

Run:

```bash
git ls-files
rg -n -i "(github_pat_|ghp_|gho_|sk-[A-Za-z0-9]|BEGIN (RSA|OPENSSH|EC|DSA) PRIVATE KEY|aws_access_key_id|aws_secret_access_key|password[[:space:]]*=|secret[[:space:]]*=)" --glob '!obj/**' --glob '!*.md' . || true
```

Expected result: the tracked file list contains source and documentation only, and the credential scan returns no credential-like match. If a real credential is found, stop before changing visibility.

- [ ] **Step 2: Set the default branch**

Run:

```bash
gh repo edit sisphus/HUST-OS-PKE-Project --default-branch lab4_3_hardlink
```

Expected result: the GitHub repository default branch becomes `lab4_3_hardlink`.

- [ ] **Step 3: Change visibility to public**

Run:

```bash
gh repo edit sisphus/HUST-OS-PKE-Project --visibility public --accept-visibility-change-consequences
```

Expected result: GitHub confirms the repository is public.

- [ ] **Step 4: Verify all external settings**

Run:

```bash
gh repo view sisphus/HUST-OS-PKE-Project --json nameWithOwner,isPrivate,url,defaultBranchRef
git ls-remote github refs/heads/lab4_3_hardlink
```

Expected result:

```text
nameWithOwner = sisphus/HUST-OS-PKE-Project
isPrivate = false
defaultBranchRef.name = lab4_3_hardlink
```

The remote branch must point to the pushed README commit. Report the public repository URL only after these checks succeed.

