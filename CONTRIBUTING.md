# Contributing to TempoFlow

Thank you for helping improve TempoFlow. Keep contributions focused, testable, and compatible with the project scope.

## Before starting

- Search existing issues and pull requests.
- Open an issue before a large architecture, format, identity, or compatibility change.
- Never use a public issue for a vulnerability; follow [SECURITY.md](SECURITY.md).

## Development setup

Use Windows 10 x64, Visual Studio Build Tools 2026, Windows SDK `10.0.26100.0`, CMake 3.25 or newer, VS Code 1.116 or newer, and Developer PowerShell for VS 18. JUCE 9.0.2 is fetched by CMake from its pinned commit.

```powershell
.\scripts\setup-dev.ps1
cmake --fresh --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug
cmake --fresh --preset windows-x64-release
cmake --build --preset windows-x64-release
ctest --preset windows-x64-release
```

See [Building the TempoFlow VST3](docs/building-vst3.md) for target-only builds, bundle locations, validation, installation, and in-source build safeguards.

## Contribution rules

- Use English for code, comments, documentation, commits, issues, and pull requests.
- Follow the enforced [commit policy](docs/commit-policy.md), including Conventional Commits 1.0.0, atomic grouping, and DCO sign-off.
- Keep each changed file in exactly one commit. Split mixed concerns before committing.
- Follow `.github/copilot-instructions.md`, `.editorconfig`, `.clang-format`, and matching path-specific instructions.
- Keep audio-thread code allocation-free, lock-free, exception-free, and free of I/O.
- Add tests for new behavior and regressions.
- Preserve preset compatibility and immutable plug-in identifiers.
- **Always follow the official Steinberg VST3 standards for plug-in behavior, lifecycle, formats, locations, preset handling, and host integration before introducing a project-specific solution.** Any necessary deviation requires an explicit engineering decision that documents the standard approach, the incompatibility or limitation, and the verification strategy.
- Do not commit generated output, secrets, personal data, or unlicensed assets.
- Sign off commits with `git commit -s` to certify the [Developer Certificate of Origin](https://developercertificate.org/).

## Dependency updates

Dependabot proposes updates for GitHub Actions while preserving full commit-SHA pinning. Review the upstream release notes and the resolved commit before merging an update.

JUCE updates are manual because Dependabot does not manage general CMake `FetchContent` declarations. Resolve the approved JUCE release tag to its full commit SHA from the official JUCE repository, update the SHA and version comment together, then run clean Debug and Release builds and tests. Never replace the SHA with a tag or branch.

Native C++ test coverage is reported through Codecov. See [Code Coverage](docs/code-coverage.md) for the scope, local command, and pull-request behavior. Do not introduce a blocking threshold without an approved baseline.

## Copilot prompt files

- `/review` performs an engineering review of current changes or the complete clean repository.
- `/sec-review` performs the corresponding security review.
- `/commit` interactively selects untracked files, proposes atomic Conventional Commit groups, and commits only individually approved groups.

## Pull requests

Use a feature branch, keep commits understandable, complete the pull-request template, and include exact verification commands. Maintainers may request a rebase, tests, design changes, or a smaller scope.

Follow the [pull request approval policy](docs/pull-request-policy.md). Pull-request authors cannot provide their own required approval.

By contributing, you agree that your contribution is licensed under `AGPL-3.0-only`.
