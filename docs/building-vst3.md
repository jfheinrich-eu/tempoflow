# Building the TempoFlow VST3

This guide builds the native Windows x64 and unsigned macOS Universal VST3 bundles from a clean TempoFlow checkout. Run all commands from the repository root.

## Prerequisites

For Windows, install the tools listed in the [README requirements](../README.md#requirements), then verify the local development environment from **Developer PowerShell for VS 18**:

```powershell
.\scripts\setup-dev.ps1
cmake --version
cl
```

JUCE is downloaded automatically from the commit pinned in `CMakeLists.txt`. A separate JUCE installation is not required. The first configuration therefore requires network access.

For macOS, install Xcode, select its command-line tools, and install CMake 3.25 or newer and PowerShell 7. The Xcode generator uses Apple Clang. No signing identity is required because the current macOS build is intentionally unsigned.

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
| macOS Debug   | `build/macos-universal-debug`   |
| macOS Release | `build/macos-universal-release` |

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

## Build the macOS Universal VST3

Configure and build the unsigned Release bundle with Xcode and Apple Clang:

```sh
cmake --fresh --preset macos-universal-release
cmake --build --preset macos-universal-release
ctest --preset macos-universal-release
```

The resulting bundle is:

```text
build/macos-universal-release/TempoFlowPlugin_artefacts/Release/VST3/TempoFlow.vst3
```

Verify that its executable contains both supported architectures:

```sh
lipo -verify_arch arm64 x86_64 \
  build/macos-universal-release/TempoFlowPlugin_artefacts/Release/VST3/TempoFlow.vst3/Contents/MacOS/TempoFlow
```

Use `macos-universal-debug` for the corresponding Debug build. Both presets set `CMAKE_OSX_ARCHITECTURES` to `arm64;x86_64` and target macOS 11 or newer. They disable Xcode code signing; notarization and distribution signing are outside this build task.

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

Load the repository's PowerShell helper in the current session, then run Steinberg's validator:

```powershell
. .\scripts\invoke-vstvalidator.ps1

Invoke-VstValidator `
  '.\build\windows-x64-release\TempoFlowPlugin_artefacts\Release\VST3\TempoFlow.vst3'
```

The expected result is:

```text
Result: 47 tests passed, 0 tests failed
```

See the [TempoFlow PowerShell scripts](project-helper.md) for function behavior and validator discovery options, and the [Steinberg VST3 Validator guide](steinberg-validator.md) for SDK download and validator build instructions.

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

## Generate and install factory presets

Build the native factory presets after building the Release VST3:

```powershell
cmake --build build/windows-x64-release `
  --config Release `
  --target TempoFlowFactoryPresets
```

The target validates every `.tempoflow` source, loads the actual built VST3 component, writes the native container with Steinberg's `PresetFile` helper, and verifies a round trip through a fresh component. It also writes a VST3 `Info` chunk so Cubase can associate the preset with the exact VST3 class and catalogue it by author, plug-in, vendor, plug-in category, musical category, musical instrument, musical character, and a supported musical style. Plug-in identity, category, and class ID come from the built VST3 class. The platform-neutral `metadata.category` value is translated only when it has a valid Steinberg musical-style equivalent.

The seven generated files are written to:

```text
build/windows-x64-release/factory-presets/Release
```

Preview the per-user development installation:

```powershell
.\scripts\install-factory-presets.ps1 -Configuration Release -WhatIf
```

Install the presets:

```powershell
.\scripts\install-factory-presets.ps1 -Configuration Release
```

The destination is Steinberg's per-user factory preset location:

```text
%APPDATA%\VST3 Presets\jfheinrich\TempoFlow
```

This location differs from Cubase's user-created preset location below `%USERPROFILE%\Documents\VST3 Presets`. Do not copy `.tempoflow` files into either VST3 preset directory.

## Troubleshooting

### `cmake` or `cl` is not found

Use the **Developer PowerShell for VS 18** Windows Terminal profile. A regular PowerShell session does not necessarily contain the Visual Studio toolchain paths.

### CMake generated files in the repository root

Stop before running another configure command. Remove only the generated root artifacts, then configure with one of the checked-in presets. Do not remove `src`, `tests`, `docs`, or other tracked project directories.

### Cubase does not find TempoFlow

Verify that the complete bundle exists at one of the predefined VST3 locations and restart Cubase. If the bundle is present but rejected, run `Invoke-VstValidator` before investigating Cubase-specific behavior.

### Cubase does not list the factory presets

Verify that all seven `.vstpreset` files exist below `%APPDATA%\VST3 Presets\jfheinrich\TempoFlow`. Restart Cubase after installing them. Files with the `.tempoflow` suffix are interchange sources and are not discoverable native VST3 presets.

### Cubase lists presets but leaves its search filters empty

Rebuild and reinstall the factory presets. Current generated files contain a VST3 `Info` chunk with Cubase catalogue metadata; older generated files contain only component state. Rescan the preset location in MediaBay or restart Cubase after replacing the files.

Cubase Elements 15 exposes the embedded attributes under these result-column groups:

| VST3 metadata | Cubase result column |
| --- | --- |
| `MediaAuthor` | Staff > Author |
| `MusicalCategory` | Musical > Category |
| `MusicalInstrument` | Musical > Category and Sub Category |
| `MusicalCharacter` | Musical > Character |
| `PlugInName` | Plugin > Plugin Name |
| `PlugInVendor` | Plugin > Plugin Vendor |
| `PlugInCategory` | Plugin > Plugin Category |

Open the full MediaBay window and use **Set up Result Columns** to expose attributes that are hidden by default. The compact preset browser does not display every available field.
