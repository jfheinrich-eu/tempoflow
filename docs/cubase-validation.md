# Cubase Validation Protocol

This protocol records manual TempoFlow VST3 checks in Cubase Elements 15. It complements automated unit tests and Steinberg's command-line validator; it does not replace either one.

## Current limitation

TempoFlow generates native factory `.vstpreset` files, but Cubase discovery and audible non-default state restoration still require manual validation. Automated generation verifies the Steinberg container, processor class ID, and component-state round trip before writing each file.

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
