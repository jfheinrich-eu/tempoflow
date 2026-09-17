---
title: TempoFlow Project Kickoff
aliases:
  - TempoFlow Kickoff
project: TempoFlow
status: discovery
updated: 2026-09-11
tags:
  - tempoflow
  - metronome
  - base44
  - pwa
  - vst3
  - cubase
  - preset
---

# TempoFlow Project Kickoff

## Executive summary

TempoFlow is a cross-platform metronome system. Its initial product family consists of:

1. a Base44 PWA for standalone browser use;
2. a native VST3 plug-in synchronized to a DAW;
3. the shared, platform-independent `.tempoflow` preset format.

The PWA and VST3 plug-in do not share application source code. They share the preset schema, musical semantics, validation rules, and preset files.

The first engineering target is a lean VST3 MVP for Cubase Elements 15. It follows the host with sample accuracy, generates metronome audio, and loads the existing `.tempoflow` presets. Direct cloud or Base44 integration is explicitly outside the MVP.

## Product concept

A TempoFlow setup describes:

- tempo;
- meter and musical grouping;
- subdivision;
- beat pattern;
- click roles;
- sound set;
- master volume;
- playback mode.

The same preset must work in at least the PWA and VST3 plug-in. Standalone, mobile, and desktop applications may follow later.

### Target audience hypothesis

> TempoFlow is for musicians who need flexible metronome patterns for practice and production and want to move those patterns between a browser and a DAW.

This target audience still requires explicit product validation.

## Current assets

### Base44 PWA

- Name: TempoFlow
- Base44 app ID: `6a1fa5495a74e26d9d8fe055`
- Intended availability: public
- Visitor login: not planned
- Base44 login is limited to operator access through “Edit with Base44”.
- Previous inspection exposed only the built-in `admin` and `user` model.
- Custom entity schemas were not visible.
- The PWA represents Internal Mode and controls tempo, start, stop, meter, subdivision, and pattern.
- The intended implementation uses JavaScript or TypeScript and Web Audio.

It remains unclear whether the PWA processes data exclusively in the client, in hidden Base44 resources, or through external services.

### Preset Format 1.0

Available artifacts:

- approved domain specification;
- Draft 2020-12 JSON Schema;
- seven reference presets.

Documentation:

- [TempoFlow Preset Format 1.0](docs/Preset%20Format%201.0/Tempoflow%20Preset%20Format%201.0.md)
- [TempoFlow Preset JSON Schema](docs/Preset%20Format%201.0/tempoflow-preset.schema.json)
- [Reference presets](docs/Preset%20Format%201.0/presets)

All seven reference files are valid JSON and satisfy the documented semantic core checks for BPM, meter, grouping, beat positions, click roles, subdivision, and volume.

### VST3

The VST3 prototype is specified but not implemented.

- Technology: C++ and JUCE
- Plug-in format: VST3 only
- Reference host: Cubase Elements 15
- DAW mode: automatic Host Mode
- Internal audio processing: mono
- Main output: mono
- Additional outputs: preserve architectural extensibility; do not implement in the MVP
- Timing inputs: host transport, PPQ/sample position, and audio-buffer position
- Goal: sample-accurate click generation

A browser-to-ASIO bridge and a Base44 runtime dependency are excluded.

## Shared domain model

```text
TempoFlow Core
|
+-- Tempo
+-- Meter
|   +-- Grouping
+-- Subdivision
+-- Pattern
|   +-- ClickType
+-- SoundSet
+-- Volume
+-- Playback Mode
```

Keep these technical areas separate:

```text
TempoFlow
|
+-- Musical Model
+-- Preset Model
+-- Playback Engine
+-- User Interface
```

## Preset contract

- Extension: `.tempoflow`
- Encoding and representation: UTF-8 JSON
- Schema discriminator: `tempoflow-preset`
- Current reference schema version: `1.0.0`
- Versioning: Semantic Versioning

Core rules:

- BPM range: `20` to `300`; fractional values are allowed.
- V1 meter denominators: `2`, `4`, `8`, and `16`.
- `meter.grouping` is required.
- The sum of `grouping` equals `meter.numerator`.
- The beat count equals `meter.numerator`.
- Beat positions cover `1..numerator` without omissions or duplicates.
- Volume range: `0.0` to `1.0`.
- Unknown optional fields are ignored.
- Unsupported required values produce an error.

V1 subdivisions:

- `none` with `partsPerBeat: 1`;
- `triplet` with `partsPerBeat: 3`.

Click roles:

```text
accent
normal
high
low
wood
mute
```

A click role is not a concrete audio file. The selected sound set maps each role to a sample or synthesized sound.

Application-specific state is not stored in a preset. This includes ASIO devices, audio interfaces, Cubase buses, sample rate, buffer size, window geometry, UI theme, browser state, and Base44 user data.

## Playback modes

### Internal Mode

TempoFlow controls tempo, transport, meter, subdivision, and pattern. The preset BPM is active.

### Host Mode

Loading TempoFlow as VST3 activates Host Mode automatically. The DAW controls tempo, transport, song/sample position, and time signature. TempoFlow continues to control click roles, pattern, sound set, master volume, and subdivision behavior.

Host tempo and time signature override preset values at runtime without modifying the preset. TempoFlow may run alongside the Cubase metronome and does not disable or replace it.

## Audio strategy

- Prefer redistributable samples with documented provenance and licensing.
- Synthesize a click when no suitable sample is available.
- `mute` needs no audio resource.
- V1 has one master volume at `sound.volume`.
- V1 has no per-click-role volume.

The MVP uses synthetic clicks so unresolved sample licenses cannot block the prototype.

## VST3 MVP

The first viable prototype must:

- load reliably in Cubase Elements 15;
- provide one mono main output;
- generate audio;
- follow host transport, tempo, and time signature;
- detect beat 1 with sample accuracy;
- support all six click roles;
- load and validate `.tempoflow` presets;
- apply the master volume;
- restore consistent state when reopening a Cubase project;
- operate without Base44 or network access.

The MVP excludes cloud synchronization, direct Base44 integration, a plug-in preset editor, Silent Bar Trainer, Tempo Trainer, user-defined sound sets, mobile integration, a custom plug-in editor, and functional multi-output routing. The initial VST3 scaffold exposes no project-owned graphical interface.

## Public PWA requirements

Before publication, provide:

- a legal notice;
- a privacy policy based on verified data flows;
- a central contact address;
- a suitable operator address;
- documentation of external services and data processing.

The “Edit with Base44” button is a low technical risk and a P3 branding issue. Remove it after the planned plan change, but do not change plans for this reason alone. Recheck plans, pricing, GitHub synchronization, and external-agent access immediately before purchase.

## Confirmed decisions

- `.tempoflow` is the shared preset format.
- JSON is the internal representation.
- The preset model remains platform independent.
- Pattern and sound set remain separate.
- `meter.grouping` is mandatory.
- PWA and VST3 do not share application implementation.
- VST3 uses C++ and JUCE.
- VST3 uses host timing rather than an independent clock.
- Cubase is the timing authority in Host Mode.
- Host tempo and meter override preset values only at runtime.
- Audio processing and the main output are mono.
- Additional outputs remain architecturally possible.
- Base44 is not a VST3 runtime dependency.
- No browser-to-ASIO bridge will be built.
- Redistributable samples are preferred; synthesis is the fallback.

## VST3 engineering decisions

### VST-001 — Development environment

Status: confirmed

- Editor: Visual Studio Code
- Development and test OS: Windows 10 x64
- Project system: CMake
- Compiler: MSVC from Visual Studio Build Tools 2026
- Windows SDK: `10.0.26100.0`
- Build entry point: CMake Presets
- Language standard: C++17
- JUCE: `9.0.2`

### VST-002 — Local workflow

Status: confirmed

- Editing, builds, and tests run on Windows 10.
- VS Code is the only IDE.
- CMake Tools controls configuration and builds.
- CMake produces native Windows x64 builds with MSVC.
- Plug-in validation and Cubase tests run on the same Windows system.
- Debug and Release use separate build directories.

Native work on the target platform avoids unnecessary cross-build and ABI risks.

### VST-003 — Target and format

Status: confirmed

- MVP platform: Windows x64
- Supported OS: Windows 10
- Format: VST3 only
- Reference host: Cubase Elements 15
- Linux binary: outside the MVP
- Standalone application: outside the MVP
- Other plug-in formats: outside the MVP

Windows 11 has no MVP support commitment until it receives dedicated testing.

### VST-004 — Build and dependencies

Status: confirmed

- CMake 3.25 or newer
- Visual Studio 18 2026 generator with MSVC
- Windows SDK `10.0.26100.0`
- JUCE `9.0.2` fetched from commit `72782788ce18c2d4d760b28e0921d6ffc6431102`
- no global JUCE installation
- reproducible Debug and Release presets
- separate Debug and Release build directories
- no moving JUCE branches such as `master` or `develop`

Local verification on September 11, 2026 confirmed that both presets configure, build, and run the toolchain check and report `JUCE v9.0.2`.

License model: confirmed

- TempoFlow is developed and published under `AGPL-3.0-only`.
- JUCE is used under its AGPLv3 option.
- Distributed binaries must include corresponding source or an AGPL-compliant source offer.
- JUCE changes, TempoFlow source, build configuration, and required notices must be provided.

### VST-005 — Plug-in identity

Status: confirmed

- Plug-in name: `TempoFlow`
- Manufacturer: `jfheinrich`
- JUCE manufacturer code: `Jfhe`
- JUCE plug-in code: `Tflo`
- Bundle identifier: `eu.jfheinrich.tempoflow`
- Initial version: `0.1.0`

JUCE derives the VST3 class ID from the manufacturer and plug-in codes. Do not define a separate ID for this new JUCE plug-in. Product and manufacturer identifiers become immutable after first release.

### VST-006 — Preset locations

Status: confirmed

- Ship factory presets read-only in the plug-in bundle or installer.
- Store user presets in a user-specific TempoFlow directory.
- Never hard-code absolute paths.
- Use JUCE to resolve platform-appropriate directories.

Windows user-preset path:

```text
%APPDATA%\TempoFlow\Presets
```

Factory-preset repository path:

```text
presets/factory
```

User-scoped VST3 development installation path:

```text
%LOCALAPPDATA%\Programs\Common\VST3
```

### VST-007 — Preset failures

Status: proposed

- A failed preset must not modify active state.
- Validate JSON syntax, schema, and semantics before applying state.
- Errors identify the file, violated rule, and offending value.
- Ignore unknown optional fields.
- Reject unknown major versions.
- Load supported `1.x` versions when all known required values are valid.
- Never perform file I/O, show dialogs, or throw exceptions from the audio thread.
- Read and validate files outside the audio thread, then publish immutable validated state safely.

### VST-008 — MVP sound source

Status: confirmed

- Use synthesized clicks for the MVP.
- Give all six click roles clearly distinguishable behavior.
- `mute` produces no audio.
- Add external samples only after license review.

### VST-009 — Host timing and meter changes

Status: confirmed

- Read the current host position for every audio block.
- Use PPQ position, BPM, time signature, sample rate, and block size as the time base.
- Calculate click events as sample offsets within the current block.
- Reset scheduler state safely on start, stop, and seek.
- Apply tempo and meter changes no later than the first block containing new host data.
- Treat preset BPM and meter as reference values in Host Mode.
- Produce silence rather than free-running timing when host data is absent or invalid.

Test Cubase behavior for multiple host changes within one audio block during prototyping.

### VST-010 — Test matrix

Status: prepared

- Debug and Release
- 44.1, 48, and 96 kHz
- buffer sizes 32, 64, 128, 256, 512, and 1024
- fixed and automated tempo
- simple, compound, and odd meters
- start at a bar boundary and within a bar
- stop, restart, loop, and seek
- time-signature changes
- all seven reference presets
- invalid JSON, invalid schema, and invalid semantics
- Cubase project save and reopen

Run unit tests, builds, validator checks, and Cubase integration tests on Windows 10.

### VST-011 — Scaffold readiness

Status: confirmed

- User presets use `%APPDATA%\TempoFlow\Presets`.
- Factory presets originate from `presets/factory` and ship read-only with the plug-in or installer.
- The MVP uses synthesized click sounds.
- The initial VST3 scaffold has no custom graphical editor.

No unresolved decision in this section blocks the initial VST3 scaffold.

## Open product decisions

- Confirm the primary audience.
- Define the three most important user journeys.
- Explain the product advantage over built-in DAW metronomes.
- Demonstrate the value of preset exchange between PWA and VST3.

## Open Base44 work

- Inventory visible and hidden data storage.
- Inspect local storage, cookies, analytics, and external requests.
- Verify `.tempoflow` import and export.
- Align legal notice and privacy policy with actual data flows.
- Check public reachability of operator and admin routes.

## Open preset-format work

- Define schema migration behavior.
- Define stable error codes and user-facing error messages.
- Express semantic validation as a shared executable test suite.
- Confirm behavior for unknown major versions.

## Risks

### P1 — Real-time and host synchronization

Transport, meter changes, tempo automation, seeks, loops, and buffer boundaries must not drift or double-trigger.

### P1 — Semantic preset validation

JSON Schema cannot express all arithmetic relationships between sibling fields. Applications must separately validate grouping sums, beat counts, and beat positions.

### P1 — Unverified PWA data flows

The Base44 app has not been fully inventoried. Its privacy and security status remains provisional.

### P2 — Sound licenses

Unclear sample licenses can block distribution. Synthetic sounds are the safe MVP fallback.

### P3 — Base44 branding

The visible “Edit with Base44” button looks unprofessional but is not an urgent security issue.

## Delivery stages

### A — Specification completion

- Express schema and semantic rules as executable tests.
- Confirm the MVP sound source.

### B — VST3 scaffold

- Add the JUCE plug-in target.
- Define immutable VST3 metadata.
- Configure a mono bus.
- Load the plug-in in Cubase Elements 15.

### C — Host synchronization

- Read transport, tempo, time signature, PPQ, sample position, and buffer position.
- Calculate beat and bar boundaries with sample accuracy.

### D — Audio and pattern

- Implement the click engine.
- Map all six click roles.
- Apply master volume.
- Process grouping and triplet subdivision.

### E — Presets

- Load `.tempoflow` files.
- Validate structure and semantics.
- Report errors without destabilizing active state.
- Use all seven presets as integration tests.

### F — PWA alignment

- Verify import and export.
- Test identical semantics with the same reference files.
- Inspect data flows and publication requirements.

### G — Release preparation

- Document installation and preset paths.
- Provide dependency and sample license evidence.
- Run the full test matrix.
- Complete legal notice, privacy policy, and download page.

## VST3 prototype definition of done

The prototype is complete when Cubase Elements 15 loads it reliably; transport, position, tempo, and meter are correct; clicks remain sample accurate across buffer sizes; every click role behaves correctly; all reference presets load; invalid presets fail safely; Host Mode never overrides the host; project reopening restores consistent state; and core operation requires no Base44 or network connection.

## References

- [JUCE repository and requirements](https://github.com/juce-framework/JUCE/blob/9.0.2/README.md)
- [JUCE CMake API](https://github.com/juce-framework/JUCE/blob/9.0.2/docs/CMake%20API.md)
- [Steinberg VST3 development setup](https://steinbergmedia.github.io/vst3_dev_portal/pages/Getting%2BStarted/How%2Bto%2Bsetup%2Bmy%2Bsystem.html)
- [Steinberg VST3 locations](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/Locations%2BFormat/Plugin%2BLocations.html)
