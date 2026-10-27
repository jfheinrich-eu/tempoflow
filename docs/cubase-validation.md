# Cubase Validation Protocol

This protocol records manual TempoFlow VST3 checks in Cubase Elements 15. It complements automated unit tests and Steinberg's command-line validator; it does not replace either one.

## Validation status

Manual Cubase acceptance completed successfully on September 26, 2026. Cubase discovered all seven generated factory `.vstpreset` files, loaded the expected audible states, restored a Cubase-managed user preset, and restored the same non-default state after reopening a project.

A focused playback acceptance run completed successfully on October 26, 2026. The Release plug-in reproduced the expected audible behavior for all seven factory presets at 48 kHz with a 192-sample ASIO process buffer. This run confirmed grouping-aware accents and audible triplet subdivisions in Cubase.

The complete VST-010 matrix was executed on October 27, 2026, with Cubase Elements 15.0.30 Build 287 (x64) on Windows 10 Pro 22H2, system build 19045.7807. It covered Debug and Release, all required sample rates and buffer sizes, fixed and automated tempo, simple through odd meters, transport operations, time-signature changes, all seven reference presets, invalid preset inputs, and Cubase project restoration. No release-blocking failure was reported. The intermittent non-blocking missing click observed during extended playback is tracked separately in [issue #45](https://github.com/jfheinrich-eu/tempoflow/issues/45).

Automated generation additionally verifies the Steinberg container, processor class ID, component-state round trip, and byte reproducibility before accepting the files.

The initial September 26 acceptance run predated generated VST3 catalogue metadata. Preset discovery, loading, and state restoration passed, but Cubase's preset search filters were empty. A follow-up run with metadata-enabled presets confirmed all expected catalogue fields in the full MediaBay result columns. Cubase Elements 15 displays `MediaAuthor` as **Staff > Author**; it does not use the technical VST3 attribute name in the user interface.

## Prerequisites

- Use a clean checkout of the intended commit.
- Run the Debug and Release CTest presets successfully.
- Build, validate, and install the Release bundle by following [Building the TempoFlow VST3](building-vst3.md).
- Build the `TempoFlowFactoryPresets` target and install the generated presets with `scripts/install-factory-presets.ps1`.
- Confirm that `Invoke-VstValidator` reports 47 passed tests and no failures.
- Close Cubase before replacing an installed VST3 bundle.

Installed development bundle:

```text
%LOCALAPPDATA%\Programs\Common\VST3\TempoFlow.vst3
```

Expected user preset root:

```text
%USERPROFILE%\Documents\VST3 Presets\jfheinrich\TempoFlow
```

Expected per-user factory preset root:

```text
%APPDATA%\VST3 Presets\jfheinrich\TempoFlow
```

## Phase 1 — Host smoke test

1. Start Cubase Elements 15 and allow the VST3 scan to finish.
2. Confirm that TempoFlow is not blocklisted and appears as an instrument.
3. Create an empty project and add one TempoFlow instrument track.
4. Record the audio sample rate and ASIO buffer size.
5. Start playback at a bar boundary in 4/4 and confirm that TempoFlow produces the default click pattern.
6. Test stop, restart, seek, and loop playback.
7. Change the host tempo and confirm that TempoFlow follows without free-running timing.
8. Change the host meter and confirm stable playback without a crash or stuck click tail.
9. Save the current state through Cubase's VST preset command. Cubase wording can vary by version.
10. Confirm the actual `.vstpreset` path and record it below.
11. Reload the saved preset and confirm that Cubase reports no state-loading error.
12. Save the Cubase project, close it, reopen it, and confirm that the plug-in loads and plays.

## Phase 2 — Non-default state restoration

1. Load a factory `.vstpreset` through Cubase whose click roles and volume differ audibly from the default.
2. Record the factory preset name, source `.tempoflow` reference, and expected audible behavior.
3. Save the state as a Cubase-managed `.vstpreset`.
4. Switch to a different TempoFlow preset, then reload the saved `.vstpreset`.
5. Verify that click roles and master volume match the originally saved non-default preset.
6. Save the Cubase project with the non-default state.
7. Close and reopen the project.
8. Verify the same audible state again.
9. Confirm through diagnostic tooling or a focused test build that the complete `.tempoflow` JSON survived both restore paths.

## Result record

### September 26, 2026

```text
Date: 2026-09-26
Tester: Joerg Heinrich
TempoFlow commit: 7ac5eda
Cubase edition and version: Cubase Elements 15; maintenance version not recorded
Windows version: Windows 10 x64; build not recorded
Audio interface and driver: Not recorded
Sample rate: Not recorded
ASIO buffer size: Not recorded
Installed VST3 path: %LOCALAPPDATA%\Programs\Common\VST3\TempoFlow.vst3
Observed user preset path: %USERPROFILE%\Documents\VST3 Presets\jfheinrich\TempoFlow
Observed factory preset path: %APPDATA%\VST3 Presets\jfheinrich\TempoFlow
Factory presets: All seven generated reference presets

Debug CTest: PASS, 8/8
Release CTest: PASS, 8/8
Steinberg validator: PASS, 47/47
Plug-in discovery: PASS
Default playback: PASS
Stop and restart: PASS
Seek: PASS
Loop: PASS
Tempo change: PASS
Meter change: PASS
Factory preset discovery: PASS, 7/7
Factory preset selection and audible state: PASS, 7/7
Factory preset PlugInName field: PASS
Factory preset PlugInVendor field: PASS
Factory preset PlugInCategory field: PASS
Factory preset MediaAuthor field: PASS, displayed as Staff > Author
Factory preset MusicalCategory field: PASS
Factory preset MusicalInstrument field: PASS
Factory preset MusicalCharacter field: PASS
Factory preset MusicalStyle field: PASS where applicable
Cubase-managed user preset restoration: PASS
Cubase project reopen: PASS
Non-default project-state restoration: PASS

Notes:
An initial playback-synchronous Cubase window flicker was observed when the
window occupied approximately 90% of the monitor. Follow-up checks found no
TempoFlow preset, state, or audio-processing failure. The visual behavior is
therefore not a VST-012 or VST-013 acceptance blocker.
```

### October 26, 2026 — Grouping and triplet playback

```text
Date: 2026-10-26
Tester: Joerg Heinrich
TempoFlow commit: Not recorded; Release build included the merged PR #42 playback behavior
Cubase edition and version: Cubase Elements 15.0.30 Build 287 (x64)
Windows version: Windows 10 x64; build not recorded
Audio interface and driver: Native Instruments Komplete Audio 6; driver version not recorded
Sample rate: 48 kHz
ASIO process buffer size: 192 samples
Driver output latency: 8.4 ms
Build configuration: Release
Factory presets: All seven generated reference presets

Factory preset selection and audible state: PASS, 7/7
Grouping-aware playback: PASS
Standard 6-8 grouping 3+3: PASS
12-8 Blues grouping 3+3+3+3: PASS
Slow Blues Shuffle triplet subdivisions: PASS
Unexpected playback behavior: NONE OBSERVED

Notes:
This focused run closes the manual Cubase acceptance gap for grouping and
triplet playback. The complete VST-010 matrix was executed on October 27, 2026.
The 1 ms USB buffer shown by the interface driver is separate from the
192-sample ASIO process buffer recorded above.
```

### October 27, 2026 — Complete VST-010 matrix

```text
Date: 2026-10-27
Tester: Joerg Heinrich
TempoFlow commit: Not recorded
Cubase edition and version: Cubase Elements 15.0.30 Build 287 (x64)
Windows version: Windows 10 Pro 22H2; system build 19045.7807
Audio interface and driver: Not recorded for the complete matrix
Build configurations: Debug and Release
Sample rates: 44.1, 48, and 96 kHz
ASIO buffer sizes: 32, 64, 128, 256, 512, and 1024 samples

Debug and Release: COMPLETED
Fixed and automated tempo: COMPLETED
Simple, compound, and odd meters: COMPLETED
Start at a bar boundary and within a bar: COMPLETED
Stop, restart, loop, and seek: COMPLETED
Time-signature changes: COMPLETED
All seven reference presets: COMPLETED
Invalid JSON, invalid schema, and invalid semantics: COMPLETED
Cubase project save and reopen: COMPLETED
Release-blocking failures: NONE REPORTED

Notes:
The test matrix records completed coverage, not a claim that no residual defect
exists. An intermittent missing click during extended playback is tracked in
issue #45 and is not currently classified as a VST3 MVP release blocker.
```

### Template

Copy this section for each validation run.

```text
Date:
Tester:
TempoFlow commit:
Cubase edition and version:
Windows version:
Audio interface and driver:
Sample rate:
ASIO buffer size:
Installed VST3 path:
Observed .vstpreset path:
Factory .vstpreset and source .tempoflow preset, if applicable:

Release CTest: PASS / FAIL / NOT RUN
Steinberg validator: PASS / FAIL / NOT RUN
Plug-in discovery: PASS / FAIL
Default playback: PASS / FAIL
Stop and restart: PASS / FAIL
Seek: PASS / FAIL
Loop: PASS / FAIL
Tempo change: PASS / FAIL
Meter change: PASS / FAIL
Default .vstpreset save and load: PASS / FAIL
Factory preset PlugInName field: PASS / FAIL / NOT RUN
Factory preset PlugInVendor field: PASS / FAIL / NOT RUN
Factory preset PlugInCategory field: PASS / FAIL / NOT RUN
Factory preset MediaAuthor field: PASS / FAIL / NOT RUN
Factory preset MusicalCategory field: PASS / FAIL / NOT RUN
Factory preset MusicalInstrument field: PASS / FAIL / NOT RUN
Factory preset MusicalCharacter field: PASS / FAIL / NOT RUN
Factory preset MusicalStyle field: PASS / FAIL / NOT RUN
Cubase project reopen: PASS / FAIL
Non-default .vstpreset restoration: PASS / FAIL / BLOCKED
Non-default project-state restoration: PASS / FAIL / BLOCKED

Notes:
```

## Failure evidence

For every failed check, record:

- the exact action that triggered the failure;
- expected and observed behavior;
- Cubase version, sample rate, and buffer size;
- whether transport was stopped, playing, looping, or seeking;
- the relevant preset or project path;
- a screenshot or short audio capture when it clarifies the problem;
- whether the failure reproduces after restarting Cubase.

Do not attach private Cubase projects, licensed audio material, or machine-specific credentials to public issues.
