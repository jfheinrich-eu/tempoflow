# Building the TempoFlow VST3

This guide builds the native Windows x64 VST3 bundle from a clean TempoFlow checkout. Run all commands from the repository root in **Developer PowerShell for VS 18**.

## Prerequisites

Install the tools listed in the [README requirements](../README.md#requirements), then verify the local development environment:

```powershell
.\scripts\setup-dev.ps1
cmake --version
cl
```

JUCE is downloaded automatically from the commit pinned in `CMakeLists.txt`. A separate JUCE installation is not required. The first configuration therefore requires network access.

## Keep generated files out of the source tree

TempoFlow's CMake presets write generated files below `build/`. Do not configure directly into the repository root.

Do not run commands such as:

```powershell
cmake -B .
cmake -S . -B .
cmake .
```

Use the checked-in presets instead. Their build directories are:

| Configuration | Build directory             |
| ------------- | --------------------------- |
| Debug         | `build/windows-x64-debug`   |
| Release       | `build/windows-x64-release` |

The top-level `CMakeLists.txt` also rejects in-source builds with a fatal configuration error.

## Build the Release VST3

Configure the Release build:

```powershell
cmake --preset windows-x64-release
```

Build only the VST3 target:

```powershell
cmake --build build/windows-x64-release `
  --config Release `
  --target TempoFlowPlugin_VST3
```

The resulting VST3 bundle is:

```text
build/windows-x64-release/TempoFlowPlugin_artefacts/Release/VST3/TempoFlow.vst3
```

Copy the VST3 into the local users VST3 folder:

```powershell
$source = (Resolve-Path `
   '.\build\windows-x64-release\TempoFlowPlugin_artefacts\Release\VST3\TempoFlow.vst3').Path
 $destinationRoot = Join-Path $env:LOCALAPPDATA 'Programs\Common\VST3'
 $destination = Join-Path $destinationRoot 'TempoFlow.vst3'

 New-Item -ItemType Directory -Path $destinationRoot -Force | Out-Null
 if (Test-Path -LiteralPath $destination) {
     Remove-Item -LiteralPath $destination -Recurse -Force
 }
 Copy-Item -LiteralPath $source -Destination $destination -Recurse
```

On Windows, a `.vst3` bundle is a directory. Copy or move the complete `TempoFlow.vst3` directory, not only the binary below `Contents`.

## Build the Debug VST3

```powershell
cmake --preset windows-x64-debug
cmake --build build/windows-x64-debug `
  --config Debug `
  --target TempoFlowPlugin_VST3
```

The Debug bundle is:

```text
build/windows-x64-debug/TempoFlowPlugin_artefacts/Debug/VST3/TempoFlow.vst3
```

## Perform a clean reconfiguration

Use `--fresh` when the generator, SDK, toolchain, dependency configuration, or CMake cache has changed:

```powershell
cmake --fresh --preset windows-x64-release
```

This refreshes only `build/windows-x64-release`. It does not generate files in the repository root.

## Build and run all tests

The build preset compiles the VST3, command-line tools, and test executables:

```powershell
cmake --build --preset windows-x64-release
ctest --preset windows-x64-release
```

Use the corresponding `windows-x64-debug` presets for a Debug verification.

## Validate the bundle

Run Steinberg's validator through the local PowerShell helper:

```powershell
Invoke-VstValidator `
  '.\build\windows-x64-release\TempoFlowPlugin_artefacts\Release\VST3\TempoFlow.vst3'
```

The expected result is:

```text
Result: 47 tests passed, 0 tests failed
```

See the [Steinberg VST3 Validator guide](steinberg-validator.md) for SDK download and validator build instructions.

## Install for the current user

For development, the VST3 specification defines this per-user directory:

```text
%LOCALAPPDATA%\Programs\Common\VST3
```

Close Cubase before replacing a loaded bundle. The following commands install the Release bundle for the current user:

```powershell
$source = (Resolve-Path `
  '.\build\windows-x64-release\TempoFlowPlugin_artefacts\Release\VST3\TempoFlow.vst3').Path
$destinationRoot = Join-Path $env:LOCALAPPDATA 'Programs\Common\VST3'
$destination = Join-Path $destinationRoot 'TempoFlow.vst3'

New-Item -ItemType Directory -Path $destinationRoot -Force | Out-Null
if (Test-Path -LiteralPath $destination) {
    Remove-Item -LiteralPath $destination -Recurse -Force
}
Copy-Item -LiteralPath $source -Destination $destination -Recurse
```

The system-wide 64-bit location is `C:\Program Files\Common Files\VST3` and normally requires administrator privileges. The predefined locations are documented in the [Steinberg VST3 Developer Portal](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/Locations%2BFormat/Plugin%2BLocations.html).

After installation, start Cubase and allow it to rescan VST3 plug-ins. TempoFlow appears as a mono-output instrument.

## Troubleshooting

### `cmake` or `cl` is not found

Use the **Developer PowerShell for VS 18** Windows Terminal profile. A regular PowerShell session does not necessarily contain the Visual Studio toolchain paths.

### CMake generated files in the repository root

Stop before running another configure command. Remove only the generated root artifacts, then configure with one of the checked-in presets. Do not remove `src`, `tests`, `docs`, or other tracked project directories.

### Cubase does not find TempoFlow

Verify that the complete bundle exists at one of the predefined VST3 locations and restart Cubase. If the bundle is present but rejected, run `Invoke-VstValidator` before investigating Cubase-specific behavior.
