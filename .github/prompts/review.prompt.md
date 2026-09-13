Perform a complete engineering review of TempoFlow without modifying files.

Determine the scope first:

1. If the working tree contains staged, unstaged, or untracked changes, review only those changes and their impact on surrounding code.
2. If the working tree is clean, review the complete repository.

Use `git status --short`, `git diff`, and `git diff --cached` to determine changed tracked files. Read the complete contents of relevant untracked files because Git diff does not include them.

Apply all repository and matching path-specific Copilot instructions. Review correctness, regressions, architecture, maintainability, C++17 and JUCE usage, real-time safety, host timing, thread safety, lifetime and ownership, numeric boundaries, preset compatibility, error handling, tests, documentation, licensing, and build reproducibility.

Explicitly verify that the declared compiler generation, architecture, Windows SDK, CMake requirements, dependency versions, presets, documentation, CI, and security workflows agree. Check that every documented invariant has automated positive and negative coverage where practical, including all reference presets. Distinguish advisory configuration, such as editor recommendations, from enforced setup; report a defect only when behavior or documentation claims a guarantee that does not exist.

Run safe read-only checks and relevant Debug and Release builds or tests when available. Do not modify files. Do not infer a defect from a configuration convention without confirming the tool's documented behavior.

Report only actionable findings, ordered by severity: P0 release-blocking or destructive; P1 high-impact correctness, real-time, compatibility, or security defect; P2 material maintainability, test, or reliability issue; P3 low-impact improvement.

For each finding, provide a concise title, exact file and line, evidence, impact, and recommended correction. Then list open questions, checks performed, checks not performed, and a short conclusion. If no findings exist, say so explicitly and identify residual test gaps.
