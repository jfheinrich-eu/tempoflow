<p align="center">
  <img src="assets/tempoflow-logo.png" alt="TempoFlow logo" width="220">
</p>

<h1 align="center">TempoFlow</h1>

<p align="center">
  Sample-accurate metronome tools for musicians.
</p>

<p align="center">
  <a href="https://github.com/jfheinrich-eu/tempoflow/actions/workflows/ci.yml"><img src="https://github.com/jfheinrich-eu/tempoflow/actions/workflows/ci.yml/badge.svg" alt="CI status"></a>
  <a href="https://codecov.io/gh/jfheinrich-eu/tempoflow"><img src="https://codecov.io/gh/jfheinrich-eu/tempoflow/graph/badge.svg" alt="Code coverage"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-AGPL--3.0--only-blue.svg" alt="License: AGPL-3.0-only"></a>
  <img src="https://img.shields.io/badge/status-pre--alpha-orange.svg" alt="Status: pre-alpha">
</p>

## Overview

TempoFlow is an open-source, cross-platform metronome system built around a shared `.tempoflow` preset format. The planned product family consists of:

- a browser-based PWA for standalone practice;
- a native VST3 plug-in synchronized to a DAW host;
- a platform-independent JSON preset format shared by both applications.

The first engineering target is a lean Windows x64 VST3 MVP for Cubase Elements 15. It will follow host transport, tempo, time signature, PPQ position, and audio-buffer position to generate sample-accurate clicks.

## Project status

TempoFlow is in pre-alpha development. The preset specification, JSON Schema, semantic validator, seven reference presets, Windows build environment, and JUCE dependency setup are available. The VST3 processor is not implemented yet.

Do not use the current repository as a production plug-in or depend on API stability.

## Architecture

```text
                         .tempoflow preset
                                 |
                  +--------------+--------------+
                  |                             |
                  v                             v
          TempoFlow PWA                 TempoFlow VST3
       Base44 / TypeScript                 JUCE / C++
                  |                             |
                  v                             v
             Web Audio                    Cubase host
```

The PWA and VST3 plug-in do not share application source code. Their contract is the preset schema, musical semantics, validation rules, and reference presets.

## Requirements

- Windows 10 x64
- Visual Studio Build Tools 2026 with the MSVC x64 toolchain
- Windows SDK `10.0.26100.0`
- CMake 3.25 or newer
- Git
- `clang-format` and `clang-tidy` from the Visual Studio LLVM tools
- Visual Studio Code 1.116 or newer with built-in Copilot Chat
- Microsoft C/C++ and CMake Tools extensions

JUCE `9.0.2` is fetched automatically by CMake from its pinned commit. A global JUCE installation is not required.

Run the idempotent setup helper from Developer PowerShell to verify the required commands and install missing recommended VS Code extensions:

```powershell
.\scripts\setup-dev.ps1
```

Use `-WhatIf` to inspect extension installation actions without changing the VS Code installation. VS Code controls extension activation based on the open workspace and file types.

## Build the toolchain check

Clone the repository, then open it from the **Developer PowerShell for VS 18** profile:

```powershell
git clone git@github.com:jfheinrich-eu/tempoflow.git
cd tempoflow
code .
```

Configure, build, and run the Debug check:

```powershell
cmake --fresh --preset windows-x64-debug
cmake --build --preset windows-x64-debug
.\build\windows-x64-debug\TempoFlowToolchainCheck_artefacts\Debug\TempoFlowToolchainCheck.exe
```

Configure, build, and run the Release check:

```powershell
cmake --fresh --preset windows-x64-release
cmake --build --preset windows-x64-release
.\build\windows-x64-release\TempoFlowToolchainCheck_artefacts\Release\TempoFlowToolchainCheck.exe
```

Run the semantic preset tests in both configurations:

```powershell
ctest --preset windows-x64-debug
ctest --preset windows-x64-release
```

The [code-coverage guide](docs/code-coverage.md) documents local collection and the informational Codecov workflow.

Validate a preset file or an entire directory:

```powershell
.\build\windows-x64-release\TempoFlowPresetValidator_artefacts\Release\TempoFlowPresetValidator.exe `
  ".\docs\Preset Format 1.0\presets"
```

Expected output:

```text
TempoFlow toolchain OK
JUCE v9.0.2
```

## Preset format

TempoFlow presets are UTF-8 JSON files with the `.tempoflow` extension. Version 1 supports:

- tempos from 20 to 300 BPM;
- simple, compound, and odd meters with explicit grouping;
- no subdivision or triplet subdivision;
- six click roles: `accent`, `normal`, `high`, `low`, `wood`, and `mute`;
- a sound-set identifier and master volume;
- internal and host playback modes.

See the [Preset Format 1.0 specification](docs/Preset%20Format%201.0/Tempoflow%20Preset%20Format%201.0.md), the [JSON Schema](docs/Preset%20Format%201.0/tempoflow-preset.schema.json), and the [reference presets](docs/Preset%20Format%201.0/presets).

Native C++ coverage collection and Codecov behavior are documented in [Code Coverage](docs/code-coverage.md).

## Roadmap

1. Add the JUCE VST3 project with a mono output.
2. Implement sample-accurate host synchronization.
3. Add the synthetic click engine and click-role mapping.
4. Integrate the semantic preset validator with plug-in state loading.
5. Validate the plug-in with Steinberg tools and Cubase Elements 15.
6. Verify preset interoperability with the PWA.

Detailed decisions and risks are tracked in [PROJECT_KICKOFF.md](PROJECT_KICKOFF.md).

## Contributing

TempoFlow welcomes focused issues and pull requests. Read [CONTRIBUTING.md](CONTRIBUTING.md) before making changes. All contributions must follow the [Code of Conduct](CODE_OF_CONDUCT.md).

## Security

Do not report vulnerabilities in public issues. Follow the private reporting process in [SECURITY.md](SECURITY.md).

The [CodeQL analysis guide](docs/codeql-analysis.md) defines the first-party scan scope, dependency boundary, alert triage, and review criteria.

## License

TempoFlow is licensed under the [GNU Affero General Public License v3.0 only](LICENSE). JUCE is used under its AGPLv3 option. Distribution of binaries must comply with all corresponding-source and notice obligations.
