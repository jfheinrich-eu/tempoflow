# TempoFlow PowerShell Scripts

This page documents the PowerShell scripts in [`scripts/`](../scripts/). Run repository commands from the TempoFlow repository root unless a script's section says otherwise.

## `invoke-vstvalidator.ps1`

Defines the developer function `Invoke-VstValidator`. It does not run commands or modify the PowerShell profile when loaded.

Load it into the current PowerShell session:

```powershell
. .\scripts\invoke-vstvalidator.ps1
```

Build the TempoFlow Release bundle first, and install or build Steinberg's `validator.exe` separately. Then run:

```powershell
Invoke-VstValidator `
  '.\build\windows-x64-release\TempoFlowPlugin_artefacts\Release\VST3\TempoFlow.vst3'
```

The helper passes the supplied arguments to `validator.exe` as separate process arguments; it does not build or evaluate a command string. Validator output is written directly to the session.

Validator discovery uses the first available setting:

1. `TEMPOFLOW_VST3_VALIDATOR`, when set to the full executable path.
2. A recursive search under `TEMPOFLOW_VST3_VALIDATOR_ROOT`, when set.
3. A recursive search under `C:\Tools\Steinberg\vst3sdk\build`.

For example, configure a non-default SDK build directory in the current session:

```powershell
$env:TEMPOFLOW_VST3_VALIDATOR_ROOT = 'D:\Tools\Steinberg\vst3sdk\build'
```

If no validator is found, more than one is found, the selected executable is missing, or the process returns a non-zero exit code, the function throws an error. Set `TEMPOFLOW_VST3_VALIDATOR` to disambiguate multiple validator builds. A zero exit code indicates process success; also inspect the validator's test summary. The helper does not interpret validator output or decide whether warnings are acceptable.

See the [Steinberg VST 3 Validator guide](steinberg-validator.md) for SDK setup and [Building the TempoFlow VST3](building-vst3.md) for the plugin build workflow.

## `setup-dev.ps1`

Checks the Windows development prerequisites, configures the repository's local Git hooks path as `.githooks`, and installs missing required VS Code extensions through the `code` command.

Run it from **Developer PowerShell for VS 18**:

```powershell
.\scripts\setup-dev.ps1
```

The script checks for Git, CMake 3.25 or newer, the Visual Studio 18 2026 CMake generator, MSVC, VS Code 1.116 or newer, `clang-format`, and `clang-tidy`. It also checks and may configure the local hooks path, and installs these VS Code extensions if missing:

- `ms-vscode.cpptools`
- `ms-vscode.cmake-tools`
- `jiadongchen.licenseheader`
- `yzhang.markdown-all-in-one`
- `DavidAnson.vscode-markdownlint`

Use `-WhatIf` to report setup actions without applying the changes. The script can request network access when VS Code needs to download extensions.

## `format.ps1`

Formats tracked C/C++ source files, JSON files, and `.tempoflow` preset files according to the repository formatting rules. It validates JSON syntax and text conventions while working and modifies files when formatting is needed.

```powershell
.\scripts\format.ps1
```

It requires `clang-format` on `PATH`, normally available from Developer PowerShell for VS 18.

## `verify-format.ps1`

Read-only verification of the formatting rules for tracked C/C++ source, JSON, and `.tempoflow` files. It checks JSON syntax, text conventions, formatting, and `git diff --check` for working-tree and staged changes.

```powershell
.\scripts\verify-format.ps1
```

If formatting is incorrect, the script reports the issue and recommends running `.\scripts\format.ps1`. It requires `clang-format` on `PATH`.

## `format-common.ps1`

Shared implementation for `format.ps1` and `verify-format.ps1`. It provides repository discovery, tracked-file enumeration and exclusions, JSON and text validation, `clang-format` invocation, and Git whitespace checks. The two public formatting scripts load it automatically; it is not intended to be run directly.

## `install-factory-presets.ps1`

Copies the exact seven expected generated factory `.vstpreset` files to the current user's TempoFlow/Cubase preset directory. It checks for missing or unexpected preset files before copying and supports PowerShell's `-WhatIf` and confirmation behavior.

```powershell
.\scripts\install-factory-presets.ps1
```

Parameters:

- `-Configuration Debug|Release` selects the build output configuration; default is `Release`.
- `-SourceDirectory <path>` overrides the generated preset directory. By default, it uses `build\windows-x64-<configuration>\factory-presets\<configuration>` under the repository root.
- `-DestinationDirectory <path>` overrides the per-user destination. The default is `%APPDATA%\VST3 Presets\jfheinrich\TempoFlow`.

Preview the operation without copying files:

```powershell
.\scripts\install-factory-presets.ps1 -WhatIf
```

Existing destination files with the same preset names are overwritten when the operation is confirmed. Factory preset source files under `presets/factory` are not modified by this script.

## `validate-commit-message.ps1`

Validates commit-message text against the repository's Conventional Commits and DCO rules. The Git commit-message hook invokes this script automatically. It can also be run directly with a message file or literal text:

```powershell
.\scripts\validate-commit-message.ps1 -MessageFile .\commit-message.txt
.\scripts\validate-commit-message.ps1 -MessageText "docs: clarify helper usage`n`nSigned-off-by: Name <name@example.com>"
```

It checks the subject type and optional scope, subject punctuation, blank-line separation, a valid `Signed-off-by` footer, and consistency between the `!` marker and `BREAKING CHANGE:` footer. Invalid messages cause a non-zero script result.
