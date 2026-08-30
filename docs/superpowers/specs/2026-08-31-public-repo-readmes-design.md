# Design: Public Repository and Bilingual README

- Status: Approved
- Date: 2026-08-31
- Audience: repository visitors, OS interviewers, and future maintainers

## Context

The repository `sisphus/HUST-OS-PKE-Project` is currently private. Its default
branch is `lab1_1_syscall`, while the latest completed work is on
`lab4_3_hardlink`. The repository also has an inherited English README that
describes the upstream PKE project rather than this completed project scope.

## Goals

- Make the exact GitHub repository public after a final local safety check.
- Make `lab4_3_hardlink` the default branch so the public landing page shows the
  latest completed implementation.
- Replace the root `README.md` with a Chinese project overview.
- Add `README_EN.md` with an equivalent English overview and cross-links.
- Describe only the verified Lab1–Lab4.3 scope, commands, architecture, and
  known limitations.

## Non-goals

- Do not claim the paused `lab2_challenge2_singlepageheap` or later Lab4/Lab5
  branches are implemented.
- Do not change kernel behavior or add tests.
- Do not publish the separate outer interview notes unless explicitly requested.
- Do not rewrite the existing branch history.

## Proposed change

1. Inspect the exact remote, current visibility, default branch, repository
   contents, and obvious credential-like files.
2. Write `README.md` in Chinese and `README_EN.md` in English. Both documents
   will include project scope, privilege-level architecture, completed branch
   summary, build/run commands using `rv64imafd`, verification notes, source
   layout, and limitations.
3. Validate Markdown structure, links to repository-local files, and the clean
   source diff. Commit only the README files and this design record.
4. Push the commit to `github/lab4_3_hardlink`.
5. Set the GitHub default branch to `lab4_3_hardlink`.
6. Change the exact GitHub repository visibility from private to public.
7. Verify the remote branch, default branch, public visibility, and README files.

## Risks and rollback

Making the repository public exposes all branches, commits, and tracked files.
The final safety check must therefore stop if it finds an obvious secret or
private credential in the tracked repository. The visibility setting can be
changed back to private, and the default branch can be restored, but public
URLs, forks, or caches may persist.

## Validation

- `git diff --check`
- README file existence and non-empty content
- Repository-local README links resolve
- `gh repo view` reports `isPrivate: false` and the expected default branch
- `git ls-remote` reports the pushed commit on `github/lab4_3_hardlink`

