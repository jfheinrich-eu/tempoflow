# CodeQL Analysis

TempoFlow uses GitHub CodeQL to scan first-party C and C++ source code. The scan is intentionally separate from dependency monitoring so that findings in downloaded build dependencies do not obscure findings owned by this project.

## Analysis scope

The CodeQL configuration includes these repository directories:

- `src/`
- `tests/`
- `tools/`

Add any future directory containing first-party C or C++ source files to `.github/codeql/codeql-config.yml`. Generated files, build output, and sources downloaded through CMake `FetchContent` are outside the analysis scope.

The workflow uses CodeQL's no-build C/C++ extraction on the Windows runner. The regular CI workflows remain responsible for configuring, compiling, and testing the supported MSVC builds.

## Why dependency code is excluded

JUCE is downloaded during CMake configuration. JUCE modules are then compiled directly into TempoFlow targets, so a manual CodeQL build also captures JUCE and its embedded libraries. Those files are not maintained in this repository and cannot be corrected here safely.

Excluding dependency source from the first-party scan is not a statement that dependency findings are false positives. Dependency risk is handled separately by:

- pinning JUCE to a full commit SHA;
- reviewing JUCE releases and security information;
- evaluating relevant upstream findings before a dependency update;
- rebuilding and running the complete test suite after an update.

Do not copy or patch generated files under `build/**/_deps`. Apply dependency corrections by updating the pinned upstream version after review.

## Alert triage

Classify every CodeQL alert by ownership and reachability:

1. Fix findings in first-party production code before merging unless a maintainer documents an explicit exception.
2. Fix test and tool findings when they can affect CI integrity, developer machines, or untrusted input handling.
3. Track findings in upstream dependencies against the pinned dependency version and the affected TempoFlow data path.
4. Use `won't fix`, not `false positive`, when closing a valid finding in code maintained by an upstream dependency. Include the dependency, pinned version, ownership boundary, and tracking decision in the dismissal comment.

Reassess dependency findings before exposing new input paths, especially images, fonts, SVG documents, archives, network data, or interprocess communication.

## Limitations and review trigger

No-build extraction can be less precise when behavior depends heavily on generated code, compiler flags, or external macros. Reevaluate the analysis mode when the VST3 target or other generated targets are introduced. If a manual build becomes necessary, it must preserve a distinct first-party result category and prevent downloaded dependency sources from flooding the project-owned alert set.

The CodeQL scope is acceptable when:

- the workflow completes successfully;
- all first-party C and C++ directories are included;
- no result is reported from `build/**` or `_deps/**`;
- CI continues to compile and test Debug and Release configurations;
- dependency findings remain documented and reviewable outside the first-party alert set.
