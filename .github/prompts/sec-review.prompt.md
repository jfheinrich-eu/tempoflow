Perform a security-focused review of TempoFlow without modifying files.

Determine the scope first:

1. If the working tree contains staged, unstaged, or untracked changes, review only those changes and their security impact on surrounding code.
2. If the working tree is clean, review the complete repository.

Use `git status --short`, `git diff`, and `git diff --cached` to determine changed tracked files. Read the complete contents of relevant untracked files because Git diff does not include them.

Apply all repository and matching path-specific Copilot instructions. Build a concise threat model covering malicious presets, untrusted paths, dependency and build-chain compromise, memory corruption, integer overflow, denial of service, unsafe state transfer, audio-thread failure, secrets, workflow permissions, artifact integrity, and license or provenance risks.

Run safe read-only checks when available. Do not modify files or expose secrets.

Report actionable findings by P0 through P3 severity. Include exact file and line, attack or failure scenario, impact, relevant CWE when applicable, and concrete remediation. Then list trust boundaries, checks performed, checks not performed, and residual risks. If no findings exist, say so explicitly and identify untested attack surfaces.
