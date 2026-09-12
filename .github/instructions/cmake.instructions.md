---
applyTo: "**/{CMakeLists.txt,*.cmake,CMakePresets.json}"
---

# CMake Instructions

- Keep the minimum version at 3.22 unless a documented feature requires raising it.
- Use target-based commands with explicit `PRIVATE`, `PUBLIC`, or `INTERFACE` scope.
- Do not use global include directories, compile flags, or link directories.
- Keep JUCE pinned to version 9.0.2 and never track a moving branch.
- Preserve separate Windows x64 Debug and Release presets.
- Keep generated content below `build/` and out of Git.
- Do not add blanket warning suppression.
