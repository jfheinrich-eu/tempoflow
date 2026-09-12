# TempoFlow Copilot Instructions

## Language and communication

- Write all source code, comments, identifiers, documentation, issue text, pull-request text, and commit messages in English.
- Use concise, direct technical language. Do not use marketing claims or fabricated status information.
- Keep Markdown valid in both GitHub and Obsidian. Use relative repository links.

## Project boundaries

- TempoFlow is an AGPL-3.0-only metronome system built with C++17, JUCE 9.0.2, CMake, and VST3.
- The MVP targets Windows 10 x64 and Cubase Elements 15.
- Treat the `.tempoflow` format, JSON Schema, semantic rules, and reference presets as the shared contract.
- Do not introduce Base44, browser, cloud, account, analytics, or network dependencies into core plug-in operation.
- Do not add another plug-in format, standalone target, multi-output implementation, or elaborate GUI unless explicitly approved.
- Do not change `Jfhe`, `Tflo`, `eu.jfheinrich.tempoflow`, or released parameter identifiers without an approved compatibility plan.

## Clean C++

- Follow the C++ Core Guidelines and RAII. Make ownership explicit and prefer value types.
- Keep classes and functions focused. Use intention-revealing names and narrow interfaces.
- Prefer `enum class`, `constexpr`, `std::optional`, and strong domain types where they prevent invalid states.
- Avoid raw owning pointers, manual `new`/`delete`, global mutable state, and hidden side effects.
- Apply const-correctness. Use `noexcept` only when the implementation guarantees it.
- Handle failures explicitly. Never silently fall back when timing or required preset data is invalid.
- Compile cleanly at high warning levels. Suppress warnings only for a documented, narrow reason.

## Source originality and security

- Generate original project code. Do not copy source code, tests, documentation, prompts, or other implementation content from external repositories or other third-party sources into this repository.
- Referencing public specifications, platform documentation, and API documentation is allowed, but implement the result independently and cite relevant sources when documentation or licensing requires it.
- Add dependencies through the documented dependency mechanism instead of copying their source into the repository.
- Apply Clean Code and relevant platform best practices to every generated change.
- Treat security as a primary design and review concern. Check input validation, bounds, integer conversions, memory safety, ownership, lifetime, thread safety, real-time constraints, filesystem access, dependency provenance, secrets, and failure behavior whenever code is created or changed.

## Real-time audio rules

- The audio callback must not allocate or free memory, lock, wait, sleep, access files or the network, log, display UI, or throw exceptions.
- Prepare buffers, samples, lookup tables, and validated state before entering the audio thread.
- Transfer state through bounded lock-free structures, atomics, or immutable snapshots with explicit lifetime.
- Derive Host Mode timing from host transport, PPQ/sample position, sample rate, and buffer position. Never use a wall clock or free-running fallback.
- Calculate click events as sample offsets within the current block. Handle start, stop, seek, loop, automation, meter changes, and buffer boundaries without drift or duplicate triggers.
- Use sufficient numeric width for sample positions. Check conversions, overflow, ranges, and invalid floating-point input.
- Produce silence when required host timing is missing or invalid.

## Preset and security rules

- Treat every preset and path as untrusted input.
- Validate JSON syntax, schema compatibility, required values, ranges, grouping sums, beat count, and beat positions before applying state.
- A failed load must leave active state unchanged.
- Reject unknown major versions and unsupported required values. Ignore unknown optional fields as specified.
- Enforce size and complexity limits before parsing or allocating from external data.
- Normalize and constrain file operations to intended directories. Prevent traversal and unsafe overwrites.
- Never commit credentials, tokens, private keys, personal data, generated builds, or machine-specific paths.
- Preserve MSVC compiler and platform security mitigations and enable additional analysis when practical.

## Build, tests, and changes

- Keep JUCE pinned to an immutable release reference. Never use `master` or `develop`.
- Use target-based CMake and CMake Presets locally. Keep generated content under `build/`.
- Add tests for behavior changes, especially timing, validation, state transfer, and regressions.
- Use all seven reference presets as acceptance fixtures. Do not alter them to hide defects.
- Run focused checks first, then all available Debug and Release checks.
- Keep changes focused. Do not reformat or rewrite unrelated files.
- Update public documentation and the changelog when behavior or requirements change.
- Before finishing, inspect all diffs, run `git diff --check`, and report checks that could not run.

## Git and commits

- Use Conventional Commits 1.0.0 for every commit: `<type>[optional scope][!]: <description>`.
- Use lowercase types such as `feat`, `fix`, `docs`, `test`, `build`, `ci`, `refactor`, `perf`, `style`, `chore`, and `revert`.
- Write commit subjects in imperative English without a trailing period. Add a body when the reason, behavior, risk, or verification is not obvious from the subject.
- Mark every incompatible change with `!` before the colon and a `BREAKING CHANGE: <description>` footer.
- Group changes by one coherent purpose and commit each group atomically. Do not combine unrelated work.
- Assign each changed file to exactly one commit group. If one file contains unrelated concerns that prevent an atomic file-level commit, stop and ask the user to split the file or choose its single group.
- Preserve user staging. Never unstage, discard, restore, stash, amend, rebase, or push unless the user explicitly authorizes that action.
- Every commit must contain a valid `Signed-off-by: Name <email>` footer as required by `CONTRIBUTING.md`. Use `git commit -s` when creating the commit. When preparing a complete message for an interface that cannot add `-s`, obtain the identity from `git config user.name` and `git config user.email` and include the footer in the proposed message; never invent an identity. Never bypass verification hooks with `--no-verify`.
