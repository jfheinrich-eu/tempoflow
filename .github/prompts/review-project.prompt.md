Perform a complete engineering review, security review, and documentation synchronization for TempoFlow.

## Scope and repository state

Determine the scope first:

1. Resolve the repository root and confirm that the current directory is inside the Git repository.
2. Stop and report the state without changing anything if a merge, rebase, cherry-pick, revert, or bisect is in progress, or if `HEAD` is detached.
3. Run `git status --short --untracked-files=all`, `git diff`, and `git diff --cached`.
4. If the working tree contains staged, unstaged, or untracked changes, review only those changes and their impact on surrounding code. Read the complete contents of relevant untracked files because Git diff does not include them.
5. If the working tree is clean, review the complete repository.
6. Never discard, reset, stash, amend, rebase, push, or force-push changes.

Apply `.github/copilot-instructions.md` and all matching path-specific instructions before reviewing or editing files.

## Engineering review

Perform everything required by `/review`, including:

- correctness, regressions, architecture, maintainability, and public contracts;
- C++17 and JUCE usage, ownership, lifetime, thread safety, and numeric boundaries;
- real-time audio safety, host timing, transport discontinuities, buffer boundaries, and state transfer;
- preset compatibility, validation, error handling, and failure behavior;
- tests, reference presets, build reproducibility, licensing, and dependency provenance;
- agreement between CMake, CMake presets, compiler generation, architecture, Windows SDK, CMake requirements, JUCE version, CI, CodeQL, coverage, and documentation;
- positive and negative automated coverage for documented invariants where practical.

Distinguish advisory configuration from enforced setup. Report a defect only when behavior or documentation claims a guarantee that does not exist.

Run safe read-only checks and relevant Debug and Release builds or tests when available. Do not infer a defect from a configuration convention without confirming the tool's behavior.

## Security review

Perform everything required by `/sec-review`, including a concise threat model covering:

- malicious presets and JSON parser abuse;
- untrusted paths, traversal, symbolic links, reparse points, and file races;
- dependency, build-chain, and artifact compromise;
- memory corruption, integer overflow, denial of service, and unsafe state transfer;
- audio-thread allocation, locking, I/O, logging, exceptions, and invalid host data;
- secrets, credentials, workflow permissions, pull-request trust, and artifact integrity;
- license, sample, and dependency provenance risks.

Do not expose secret values. Run safe read-only security checks when available.

## Documentation synchronization

Review every project documentation file matching both scopes:

- all files below `docs/`, recursively;
- every root-level `*.md` file, including `README.md`, `CHANGELOG.md`, `PROJECT_KICKOFF.md`, `CONTRIBUTING.md`, `SECURITY.md`, `SUPPORT.md`, and governance documents.

Exclude generated build output and the result file created by this prompt from the content comparison. Include any pre-existing result file in Git-state reporting, but do not treat it as authoritative project documentation.

Compare documentation claims with the current source, tests, CMake configuration, presets, CI workflows, security workflows, dependency versions, and repository status. Update documentation when it is stale, contradictory, incomplete for an implemented behavior, or claims an unimplemented feature works. Preserve valid historical changelog entries, normative specifications, and clearly labeled proposals or future work.

Documentation changes must:

- remain in English and follow repository documentation instructions;
- use relative links and valid Markdown;
- preserve the approved `.tempoflow` contract unless the source contract itself has intentionally changed;
- avoid marketing claims and unsupported release guarantees;
- keep the README, project kickoff, build guides, security documentation, preset specification, JSON Schema, changelog, and CI descriptions mutually consistent.

Do not modify source code, tests, CMake files, workflows, presets, schemas, or other non-documentation files as part of this command. If the review finds a code defect, report it; do not fix it in this workflow. The only allowed file changes are documentation corrections and the required result file below.

## Verification

After documentation edits, run the narrowest available checks first, then broader checks when practical:

- `git diff --check`;
- Markdown or documentation linters when available;
- JSON/schema validation for changed JSON documentation artifacts when applicable;
- relevant Debug and Release builds/tests if documentation changes affect documented commands, targets, or paths;
- safe repository-policy, secret, generated-file, and oversized-file checks.

Do not claim a check passed unless it was actually run. Record commands that could not run and why.

## Findings and output

Report only actionable findings, ordered by severity:

- P0: release-blocking or destructive;
- P1: high-impact correctness, real-time, compatibility, or security defect;
- P2: material maintainability, test, reliability, or documentation defect;
- P3: low-impact improvement.

For every finding, provide:

- severity and concise title;
- exact file and line or a precise file location;
- engineering or attack evidence;
- impact;
- relevant CWE for security findings when applicable;
- concrete recommended correction.

Then include:

- documentation files reviewed and documentation files changed;
- open questions and assumptions;
- trust boundaries for the security review;
- checks performed and checks not performed;
- residual risks;
- a short conclusion describing the actual project status.

If no findings exist, say so explicitly and identify remaining test or documentation gaps.

## Required result artifacts

Create or update exactly one Markdown result document in the repository root using the execution date in local ISO format:

`YYYY-MM-DD_review-project-results.md`

The file must contain the complete final report, including findings, documentation changes, open questions, trust boundaries, checks, residual risks, and conclusion. Do not create additional result files.

Finally, display the same complete report in the chat window. The chat report and the root result document must agree in substance; do not summarize away findings or omitted checks in either output.

Do not create commits or push changes. At the end, report the final Git status and every file modified or created by this command.
