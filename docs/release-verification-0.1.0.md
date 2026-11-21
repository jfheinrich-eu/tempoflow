# TempoFlow 0.1.0 release verification

## Status

Release verification is complete.

The automated, native, Steinberg Validator, installer, and clean-VM checks pass
for commit `3826ce5f0272c213663f0b6a626ebed72fd29e24`. Issue
[#29](https://github.com/jfheinrich-eu/tempoflow/issues/29) can be closed after
this evidence is merged.

## Release candidate

| Item | Value |
| --- | --- |
| Verification completed | 2026-11-21 |
| Commit | `3826ce5f0272c213663f0b6a626ebed72fd29e24` |
| Branch | `codex/final-mvp-release-verification-issue-29` |
| Version | `0.1.0` |
| Installer | `TempoFlow-0.1.0-windows-x64.exe` |
| Installer SHA-256 | `DC69E2956B3C2AF208835A51A643C5DA234B22082D6A5381D274B93113A66127` |

## Local toolchain

| Component | Version |
| --- | --- |
| Operating system | Windows 10 Pro 22H2 x64, build `19045` |
| Visual Studio Build Tools | Visual Studio 2026 `18.10.0` |
| MSVC | `19.51.36257` x64 |
| MSBuild | `18.10.1-1.26427.6` |
| Windows SDK | `10.0.26100.0` |
| CMake | `4.3.1-msvc1` |
| JUCE | `9.0.2` at pinned commit `72782788ce18c2d4d760b28e0921d6ffc6431102` |
| NSIS | `3.13` |
| Steinberg Validator | Locally built VST3 SDK validator, SHA-256 `2D8A407C49A2B45BAABA14BD80CF8B8112975D2157E74EBDCFA73F044E5DCA57` |

## Verification results

| Check | Result | Evidence |
| --- | --- | --- |
| Clean Debug configure and build | PASS | `cmake --fresh --preset windows-x64-debug`; `cmake --build --preset windows-x64-debug` |
| Debug CTest | PASS | 8 of 8 tests passed |
| Debug toolchain check | PASS | `TempoFlow toolchain OK`; JUCE `9.0.2` |
| Clean Release configure and build | PASS | `cmake --fresh --preset windows-x64-release`; `cmake --build --preset windows-x64-release` |
| Release CTest | PASS | 8 of 8 tests passed |
| Release toolchain check | PASS | `TempoFlow toolchain OK`; JUCE `9.0.2` |
| Factory preset generation | PASS | All seven native `.vstpreset` files generated and round-tripped |
| Factory preset reproducibility | PASS | `vst3-factory-preset-generation` CTest passed in Debug and Release |
| Steinberg VST3 Validator | PASS | 47 tests passed, 0 failed |
| Windows installer build | PASS | NSIS package created and repository installer checks passed |
| Installer contents | PASS | VST3 bundle, seven factory presets, license files, metadata, components, and system-wide destinations verified |
| Repository formatting | PASS | 22 C/C++ files, 6 JSON files, and 7 TempoFlow preset files verified |
| Markdown lint | PASS | 31 files, 0 issues |
| npm audit | PASS | 0 vulnerabilities |
| Secret and generated-artifact scan | PASS | No tracked credential patterns or generated binaries found |
| Windows CI | PASS | [GitHub Actions run 44821651039](https://github.com/jfheinrich-eu/tempoflow/actions/runs/44821651039) |
| macOS Universal CI | PASS | `arm64` and `x86_64` job in GitHub Actions run 44821651039 |
| CodeQL | PASS | [GitHub Actions run 44821650937](https://github.com/jfheinrich-eu/tempoflow/actions/runs/44821650937); no open code-scanning alerts |
| Coverage | PASS | [GitHub Actions run 44821650894](https://github.com/jfheinrich-eu/tempoflow/actions/runs/44821650894) |
| Commit policy | PASS | PR #49 commit-policy validation passed before merge |
| Engineering review | PASS | No untracked P0 or P1 first-party finding identified |
| Security review | PASS | No actionable P0 through P3 security finding identified |
| Complete VST-010 matrix | PASS | Recorded on 2026-10-27; no plug-in source changed after that validation |
| Clean-VM installer and uninstall test | PASS | Completed on 2026-11-21 with the installer and SHA-256 recorded above |

## Commands executed

```powershell
cmake --fresh --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug
.\build\windows-x64-debug\TempoFlowToolchainCheck_artefacts\Debug\TempoFlowToolchainCheck.exe

cmake --fresh --preset windows-x64-release
cmake --build --preset windows-x64-release
ctest --preset windows-x64-release
.\build\windows-x64-release\TempoFlowToolchainCheck_artefacts\Release\TempoFlowToolchainCheck.exe

. .\scripts\invoke-vstvalidator.ps1
Invoke-VstValidator `
  '.\build\windows-x64-release\TempoFlowPlugin_artefacts\Release\VST3\TempoFlow.vst3'

.\scripts\build-windows-installer.ps1
Get-FileHash `
  '.\build\windows-x64-release\package\TempoFlow-0.1.0-windows-x64.exe' `
  -Algorithm SHA256

npm ci --ignore-scripts
npm audit --audit-level=low
npm run lint:markdown
.\scripts\verify-format.ps1
git diff --check
```

## Clean-VM verification

The procedure in [Release validation VM](release-validation-vm.md) completed
successfully with the exact release-candidate installer recorded above.

```text
Date: 2026-11-21
Tester: Joerg Heinrich
TempoFlow commit: 3826ce5f0272c213663f0b6a626ebed72fd29e24
Installer filename: TempoFlow-0.1.0-windows-x64.exe
Installer SHA-256: DC69E2956B3C2AF208835A51A643C5DA234B22082D6A5381D274B93113A66127
VM name: TempoFlow Release Validation
VM generation: Generation 2
VM checkpoint: cubase-15.0.40-clean
Virtual processors: 4
Assigned memory: 8 GB fixed
Windows edition and version: Windows 11 Enterprise Evaluation x64, 10.0.26300
Windows OS build: 26300
Windows ISO filename and SHA-256: 26300.9457.260913-1737.26h2_ge_release_svc_refresh_CLIENTENTERPRISEEVAL_OEMRET_x64FRE_de-de.iso; D67C5606AB6AB3F2C7A7D3EB7C66387D03714427236FD5930E3734D50716D9DF
Cubase edition and version: Cubase Elements 15.0.40 Build 324 x64
Cubase installer SHA-256: Not recorded

Baseline restored before test: PASS
Installer checksum: PASS
Administrator prompt: PASS
VST3 system installation: PASS
Factory preset installation: PASS
Factory preset count: 7
Raw .tempoflow exclusion: PASS
License files installed: PASS
Windows Installed apps entry: PASS
Cubase plug-in discovery: PASS
Cubase factory preset discovery: PASS
Non-default preset load: PASS
Cubase host-managed preset copy save: PASS
Cubase project-state restoration: PASS
Uninstall completed: PASS
System VST3 removed: PASS
Factory presets removed: PASS
License files removed: PASS
Host-managed preset copy preserved: PASS
Unrelated VST3 content unchanged: PASS

Result: PASS
Notes: The host-managed preset copy contains an unchanged loaded factory state. TempoFlow 0.1.0 does not provide a custom preset editor or import workflow. The Cubase installer SHA-256 was not captured when the VM baseline was created.
```

## Known limitation

Issue [#45](https://github.com/jfheinrich-eu/tempoflow/issues/45) tracks an
infrequent missing click during extended Cubase playback at 65 BPM with the
`Standard 4/4` preset. The trigger is not yet known.

This defect is accepted as a known, non-blocking limitation for TempoFlow 0.1.0.
It remains open for post-MVP investigation. Reassess the release decision if the
dropout becomes more frequent, causes transport instability, or reproduces
during the final clean-VM verification.

## Review conclusion

No untracked P0 or P1 first-party engineering defect was identified. No
actionable P0 through P3 security finding was identified. The release candidate
satisfies the verification requirements for TempoFlow 0.1.0 with issue #45
accepted as the documented non-blocking limitation.
