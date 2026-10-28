# Windows release installer

TempoFlow uses CPack and NSIS 3.13 to create an unsigned Windows x64 installer. The installer requires administrator approval because it writes to Steinberg's system-wide VST3 and factory-preset locations.

## Installed content

| Content | Destination |
|---|---|
| `TempoFlow.vst3` bundle | `%ProgramFiles%\Common Files\VST3\TempoFlow.vst3` |
| Seven factory `.vstpreset` files | `%ProgramData%\VST3 Presets\jfheinrich\TempoFlow` |
| Project and dependency licenses | `%ProgramFiles%\TempoFlow` |

The package does not contain raw `.tempoflow` files. It does not write to `%USERPROFILE%\Documents\VST3 Presets` or `%APPDATA%\VST3 Presets`, so Cubase-managed user presets and development factory presets remain outside the installer's ownership.

## Build prerequisites

- Developer PowerShell for VS 18
- CMake 3.25 or newer
- NSIS 3.13

Pass a non-standard NSIS location explicitly:

```powershell
.\scripts\build-windows-installer.ps1 `
  -NsisExecutable 'C:\Tools\NSIS-3.13\nsis-3.13\makensis.exe'
```

The script accepts only NSIS 3.13. CI downloads the official portable archive and verifies this SHA-256 before use:

```text
ba63dffc4410ee89193e1cb5a41989991bd77c61068da17e3156d136b7b0b3d8
```

## Build the installer

Configure and test the Release build first:

```powershell
cmake --fresh --preset windows-x64-release
cmake --build --preset windows-x64-release
ctest --preset windows-x64-release
```

Create and verify the installer:

```powershell
.\scripts\build-windows-installer.ps1
```

Output:

```text
build/windows-x64-release/package/TempoFlow-0.1.0-windows-x64.exe
build/windows-x64-release/package/TempoFlow-0.1.0-windows-x64.exe.sha256
```

The build fails if the plug-in bundle, one of the seven factory presets, legal files, expected system destinations, or the installer executable is missing. It also fails if raw `.tempoflow` content enters the package definition.

## Clean installation test

Use a clean Windows Sandbox or disposable virtual machine. This avoids confusing the release package with a per-user development copy.

1. Confirm that these paths do not contain an older TempoFlow installation:
   - `%LOCALAPPDATA%\Programs\Common\VST3\TempoFlow.vst3`
   - `%APPDATA%\VST3 Presets\jfheinrich\TempoFlow`
   - `%ProgramFiles%\Common Files\VST3\TempoFlow.vst3`
   - `%ProgramData%\VST3 Presets\jfheinrich\TempoFlow`
2. Run `TempoFlow-0.1.0-windows-x64.exe` and approve the administrator prompt.
3. Verify the VST3 bundle, all seven `.vstpreset` files, and the license files at the documented locations.
4. Confirm that no `.tempoflow` file was installed.
5. Start Cubase, rescan VST3 plug-ins if required, and verify that TempoFlow and all seven factory presets are visible.
6. Load at least one non-default preset and confirm audible playback.

The per-user VST3 and factory-preset locations have higher discovery priority than their system-wide equivalents. Remove stale development copies before judging the installed release package.

## Clean uninstall test

1. Save a Cubase-managed user preset under `%USERPROFILE%\Documents\VST3 Presets\jfheinrich\TempoFlow`.
2. Uninstall **TempoFlow 0.1.0** through Windows **Installed apps**.
3. Verify that the system-wide `TempoFlow.vst3` bundle, seven factory presets, and `%ProgramFiles%\TempoFlow` legal files were removed.
4. Verify that the Cubase-managed user preset still exists.
5. Verify that unrelated VST3 plug-ins and presets were not changed.

Do not mark the release-package issue complete until this clean installation and uninstall procedure has passed on a disposable Windows environment.
