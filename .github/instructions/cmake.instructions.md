---
applyTo: "**/{CMakeLists.txt,*.cmake,CMakePresets.json}"
---

# CMake Instructions

- Keep the minimum version at 3.25 unless a documented feature requires raising it.
- Use target-based commands with explicit `PRIVATE`, `PUBLIC`, or `INTERFACE` scope.
- Do not use global include directories, compile flags, or link directories.
- Keep JUCE 9.0.2 pinned to its full commit SHA and never track a tag or moving branch.
- Preserve separate Windows x64 Debug and Release presets.
- Keep generated content below `build/` and out of Git.
- Do not add blanket warning suppression.
