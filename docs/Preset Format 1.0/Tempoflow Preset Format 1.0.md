---
title: TempoFlow Preset Format 1.0
aliases:
  - TempoFlow Preset Specification
  - TempoFlow Preset Schema
project: TempoFlow
created-by: ChatGPT
created-at: 2026-09-06T21:30:00+02:00
updated:
  - by: ChatGPT
    at: 2026-09-06T21:30:00+02:00
    comment: Initial creation
  - by: Joerg Heinrich
    at: 2026-09-07T00:28:00+02:00
    comment: Completed phases 3B and 3C
  - by: Codex
    at: 2026-09-11T00:00:00+02:00
    comment: Synchronized, translated, and consolidated the approved specification
version: 1.1.2
status: approved
tags:
  - tempoflow
  - metronome
  - vst3
  - cubase
  - preset
  - specification
  - audio
sources:
  - TempoFlow Base44 screenshots supplied by Joerg Heinrich
  - TempoFlow architecture decisions
github-managed: true
github-repository: https://github.com/jfheinrich-eu/tempoflow
---

# TempoFlow Preset Format 1.0 — Approved Specification

## 1. Purpose

The TempoFlow Preset Format defines a shared, platform-independent metronome setup. It is intended for the TempoFlow PWA, TempoFlow VST3, future standalone or mobile applications, and preset libraries. It must not depend on Base44, Cubase, or a programming language.

## 2. Architecture

```text
TempoFlow
|
+-- Musical Model
+-- Preset Model
+-- Playback Engine
+-- User Interface
```

A preset contains musical parameters, click roles, playback settings, and optional practice settings. It excludes UI state, window geometry, Cubase project position, browser state, audio-device configuration, and ASIO configuration.

## 3. File format

- Extension: `.tempoflow`
- Encoding: UTF-8
- Representation: JSON
- Example: `Slow Blues Shuffle.tempoflow`

Every file must contain valid JSON.

## 4. Root structure

```json
{
  "schema": "tempoflow-preset",
  "schemaVersion": "1.0.0",
  "metadata": {},
  "tempo": {},
  "meter": {},
  "subdivision": {},
  "pattern": {},
  "sound": {},
  "playback": {}
}
```

## 5. Complete example

```json
{
  "schema": "tempoflow-preset",
  "schemaVersion": "1.0.0",
  "metadata": {
    "name": "Slow Blues Shuffle",
    "description": "A slow blues shuffle practice preset",
    "category": "blues",
    "tags": ["blues", "shuffle", "practice"],
    "author": "Joerg",
    "createdAt": "2026-09-06T21:30:00+02:00",
    "updatedAt": "2026-09-06T21:30:00+02:00"
  },
  "tempo": { "bpm": 72 },
  "meter": {
    "numerator": 4,
    "denominator": 4,
    "grouping": [1, 1, 1, 1]
  },
  "subdivision": { "mode": "triplet", "partsPerBeat": 3 },
  "pattern": {
    "beats": [
      { "beat": 1, "click": "accent" },
      { "beat": 2, "click": "normal" },
      { "beat": 3, "click": "normal" },
      { "beat": 4, "click": "normal" }
    ]
  },
  "sound": { "soundSet": "default", "volume": 0.8 },
  "playback": { "mode": "internal" }
}
```

## 6. Schema version

`schemaVersion` is required and follows Semantic Versioning:

- PATCH: backward-compatible corrections.
- MINOR: backward-compatible optional additions.
- MAJOR: incompatible changes.

## 7. Metadata

`metadata.name` is required. `description`, `category`, `tags`, `author`, `createdAt`, and `updatedAt` are optional.

`description`, `category`, and `author` are strings. `tags` is an array containing only strings. Populated `createdAt` and `updatedAt` values use the RFC 3339 date-time format; an empty string remains valid for compatibility with existing presets.

## 8. Tempo model

`tempo.bpm` must be between 20 and 300 inclusive. Fractional values such as `120.5` are valid even when a UI displays only whole numbers.

## 9. Meter

The meter model requires `numerator`, `denominator`, and `grouping`.

```json
{
  "meter": {
    "numerator": 7,
    "denominator": 8,
    "grouping": [2, 2, 3]
  }
}
```

V1 supports denominators `2`, `4`, `8`, and `16`. The sum of all grouping entries must equal `numerator`. Grouping preserves distinct interpretations such as `2+2+3`, `3+2+2`, and `2+3+2` in 7/8.

## 10. Subdivision model

V1 supports exactly:

```json
{ "mode": "none", "partsPerBeat": 1 }
```

and:

```json
{ "mode": "triplet", "partsPerBeat": 3 }
```

## 11. Reserved subdivisions

`eighth`, `sixteenth`, `shuffle`, and `custom` are reserved for future schema versions and are not valid V1 values. Store the extensible `subdivision.mode`, not a Boolean triplet flag.

## 12. ClickType model

Valid click roles are `accent`, `normal`, `high`, `low`, `wood`, and `mute`.

## 13. ClickType semantics

A ClickType is a sound role, not a WAV filename. For example, `"click": "wood"` selects the wood role from the active sound set. This keeps patterns separate from sound libraries.

## 14. Pattern model

```json
{
  "pattern": {
    "beats": [
      { "beat": 1, "click": "accent" },
      { "beat": 2, "click": "normal" },
      { "beat": 3, "click": "normal" },
      { "beat": 4, "click": "normal" }
    ]
  }
}
```

## 15. Beat-pattern examples

```text
Standard 4/4:  A N N N
Rock backbeat: L H L H
Beat 1 only:   A - - -
Beats 2 and 4: - W - W
```

## 16. Sound model

```json
{ "sound": { "soundSet": "default", "volume": 0.8 } }
```

`volume` ranges from `0.0` for silence to `1.0` for full level.

## 17. Sound sets

A sound set maps click roles to concrete sounds. Prefer suitable freely redistributable samples when their license permits plug-in distribution, their source is documented, and their quality is adequate. Synthesize a role when no suitable sample exists. `mute` needs no audio resource.

## 18. Sound-set separation

A preset stores a role; the sound set selects the concrete sound. One pattern can therefore be used with multiple sound sets.

## 19. Playback model

Supported `playback.mode` values are `internal` and `host`. TempoFlow VST3 processes audio in mono and exposes a mono main output. Preserve future output extensibility without implementing multi-output routing in the MVP. V1 has one master volume and no per-click-role volume.

## 20. Internal Mode

TempoFlow controls tempo, start, stop, meter, subdivision, and pattern.

## 21. Host Mode

Loading TempoFlow as VST3 activates Host Mode automatically. The host controls tempo, transport, position, and time signature. TempoFlow controls click roles, pattern, sound set, volume, and subdivision behavior. TempoFlow may run alongside the Cubase metronome and does not disable it.

## 22. VST timing rule

Host Mode must not use an independent timer or browser clock. Its time base is host transport, sample position, and audio-buffer position. The target is sample-accurate timing.

## 23. Preset and host tempo

Host BPM takes precedence in Host Mode. Preset BPM remains the initial, reference, and Internal Mode value.

## 24. Entering Host Mode

If a preset contains 72 BPM in 4/4 and Cubase is at 96 BPM in 6/8, TempoFlow runs at 96 BPM in 6/8 without modifying the stored preset.

## 25. Preset categories

Recommended, extensible values are `general`, `practice`, `blues`, `rock`, `jazz`, `funk`, `metal`, and `custom`.

## 26. Example library

```text
Presets/
+-- General/
|   +-- Standard 4-4.tempoflow
|   +-- Standard 3-4.tempoflow
|   +-- Standard 6-8.tempoflow
+-- Blues/
|   +-- Slow Blues.tempoflow
|   +-- Texas Shuffle.tempoflow
|   +-- 12-8 Blues.tempoflow
+-- Rock/
|   +-- Straight Rock.tempoflow
|   +-- Backbeat.tempoflow
+-- Practice/
    +-- Beat 1 Only.tempoflow
    +-- Beats 2 and 4.tempoflow
    +-- Silent Beats.tempoflow
```

## 27. V1 validation rules

- `schema` equals `tempoflow-preset`.
- `schemaVersion` is present and compatible.
- `20 <= bpm <= 300`.
- `numerator >= 1`.
- `denominator` is `2`, `4`, `8`, or `16`.
- `grouping` contains positive values and `sum(grouping) == numerator`.
- Beat count equals `numerator`.
- Beat positions cover `1..numerator` without omissions or duplicates.
- Every click uses a defined ClickType.
- `0.0 <= volume <= 1.0`.
- Preset files are read through one open file handle and rejected when more than 1 MiB is read.
- JSON is rejected before parsing when it exceeds 32 nesting levels, 2,048 containers, 256 properties in one object, 4,096 elements in one array, 8,192 properties in total, or 16,384 array elements in total.
- Directory validation does not follow symbolic links or reparse points. It is limited to 16 levels, 10,000 examined entries, 1,024 preset files, and 16 MiB of preset data.

## 28. Error behavior

Ignore unknown optional fields for forward compatibility. Reject unsupported required values. For example, `"click": "laser"` produces an error such as `Unsupported ClickType: laser`.

## 29. Future subdivision patterns

A future schema may add a `subdivisions` array to each beat to represent shuffle, complex triplets, 12/8 patterns, and ghost clicks. Introduce this no earlier than schema `1.1.0`.

## 30. Future Silent Bar Trainer

```json
{
  "trainer": {
    "enabled": true,
    "cycle": { "clickBars": 2, "silentBars": 2 }
  }
}
```

This feature is planned but excluded from the minimal V1 core.

## 31. Future Tempo Trainer

```json
{
  "tempoTrainer": {
    "enabled": true,
    "startBpm": 80,
    "targetBpm": 120,
    "increment": 2,
    "barsPerStep": 4
  }
}
```

## 32. Future sound sets

Future libraries may provide Default, Classic, Woodblock, Studio, Soft Practice, and Custom sound sets. Custom sound sets may contain user-provided WAV files.

## 33. Excluded application state

Preset 1.0 excludes ASIO devices, audio interfaces, Cubase buses, sample rate, buffer size, window position and size, theme, PWA installation state, browser state, and Base44 user information.

## 34. TempoFlow Core

```text
TempoFlow Core
|
+-- Tempo
+-- Meter
+-- Subdivision
+-- Pattern
|   +-- ClickType
+-- SoundSet
+-- Volume
+-- Playback Mode
```

The PWA uses JavaScript or TypeScript. VST3 uses C++ and JUCE. They share schema, semantics, validation rules, and preset files rather than application source code.

## 35. Implementation sequence

- Phase 3A — preset schema: complete.
- Phase 3B — formal [JSON Schema](tempoflow-preset.schema.json): complete.
- Phase 3C — seven [reference presets](presets): complete.
- Phase 4A — Headless VST3 scaffold, host timing, synthetic click engine, and typed preset runtime model: implemented and unit-tested.
- Phase 4B — Plugin state loading, preset-driven beat roles and volume, and project-state restoration: implemented and unit-tested. User-facing preset selection, grouping-driven playback, and audible triplet subdivisions remain open.

## 36. VST3 MVP definition

The first viable prototype must load in Cubase Elements 15, generate audio, follow host transport/tempo/time signature, recognize beat 1, support every ClickType, load `.tempoflow` presets, and apply volume.

Cloud synchronization, Base44 integration, a preset editor, trainer features, custom samples, mobile integration, and elaborate animation are excluded.

## 37. Technical target

```text
                         .tempoflow preset
                                 |
                  +--------------+--------------+
                  |                             |
                  v                             v
            TempoFlow PWA                 TempoFlow VST3
            Base44 / Web                     JUCE / C++
                  |                             |
                  v                             v
             Web Audio                    Cubase host
                                                |
                                                v
                                        ASIO audio engine
```

## 38. Accepted decisions

- JSON-based `.tempoflow` format with Semantic Versioning.
- Platform-independent model and separation of patterns and sound sets.
- Six ClickTypes.
- Automatic Host Mode with Cubase as timing authority.
- Parallel operation with the Cubase metronome.
- C++ and JUCE for VST3.
- No browser-to-ASIO bridge or Base44 VST dependency.
- Mono processing and main output.
- Future output extensibility.
- One V1 master volume.
- Mandatory `meter.grouping`.
- Redistributable samples when suitable; synthesized fallback otherwise.

## 39. Final architecture decisions

Choose sound sources per click role. Process and output mono. Preserve optional future outputs. Use `sound.volume` rather than per-role volume. Activate Host Mode in compatible DAWs. Do not disable the Cubase metronome. Treat `meter.grouping` as mandatory.

## 40. Meter and grouping semantics

Grouping is musically meaningful. In 7/8, `2+2+3`, `3+2+2`, and `2+3+2` are distinct. Validation requires `sum(grouping) == numerator`.

```text
[2, 2, 3] -> valid for 7/8
[3, 2, 2] -> valid for 7/8
[2, 3, 2] -> valid for 7/8
[2, 2]    -> invalid for 7/8
```

## 41. Status

TempoFlow Preset Format 1.0 is approved. The specification document version is `1.1.2`.

Completed decisions cover grouping, subdivision, sound generation, host/internal behavior, mono processing, future output extensibility, and parallel Cubase-metronome operation. The preset schema, formal JSON Schema, seven reference presets, host-timing prototype, synthetic click engine, and typed runtime model are complete. The next phase is plugin-state integration and external VST3/Cubase validation.
