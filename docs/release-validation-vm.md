# Release validation VM

This guide creates a reproducible Hyper-V environment for validating the TempoFlow Windows installer with Cubase Elements 15. Use it for installation, VST3 discovery, factory presets, user-data preservation, and clean uninstall checks.

The VM does not replace real-hardware audio validation. Hyper-V audio redirection is suitable for a functional playback check, but not for ASIO latency, buffer stability, sample accuracy, or audio-interface compatibility. Run those checks on the physical validation system with the [Cubase validation protocol](cubase-validation.md).

## Validation baseline

Use one documented baseline for every release candidate.

| Setting | TempoFlow baseline |
| --- | --- |
| Hypervisor | Hyper-V |
| VM generation | Generation 2 |
| Guest OS | Windows 11 Enterprise Evaluation x64 |
| Windows version | `10.0.26300` |
| Windows OS build | `26300` |
| Virtual processors | 4 |
| Memory | 8 GB fixed |
| System disk | 100 GB dynamically expanding VHDX |
| Network | Hyper-V Default Switch during setup and Cubase activation |
| Cubase | Cubase Elements 15.0.40 Build 324 x64 |
| TempoFlow | Release installer built from the commit under test |

This Windows 11 VM is the clean release-installer validation baseline. It does not replace the separately documented Windows 10 MVP compatibility commitment or the physical Windows 10 Cubase validation evidence.

## Windows installation image

Use this exact Microsoft evaluation image when rebuilding the clean validation VM:

| Property | Recorded value |
| --- | --- |
| Edition | Windows 11 Enterprise Evaluation |
| Version | `10.0.26300` |
| OS build | `26300` |
| Installation media build | `26300.9457` |
| ISO filename | `26300.9457.260913-1737.26h2_ge_release_svc_refresh_CLIENTENTERPRISEEVAL_OEMRET_x64FRE_de-de.iso` |
| ISO SHA-256 | `D67C5606AB6AB3F2C7A7D3EB7C66387D03714427236FD5930E3734D50716D9DF` |
| Official download | [Microsoft Windows 11 Enterprise Evaluation x64, German](https://go.microsoft.com/fwlink/?LinkId=2382603&clcid=0x407&culture=de-de&country=de) |

Microsoft's redirect can point to a newer evaluation image in the future. A newer filename or hash is a baseline change and must not silently replace the image recorded above.

Verify the downloaded image before attaching it to the VM:

```powershell
$iso = 'C:\Path\To\26300.9457.260913-1737.26h2_ge_release_svc_refresh_CLIENTENTERPRISEEVAL_OEMRET_x64FRE_de-de.iso'
$expectedHash = 'D67C5606AB6AB3F2C7A7D3EB7C66387D03714427236FD5930E3734D50716D9DF'
$actualHash = (Get-FileHash -LiteralPath $iso -Algorithm SHA256).Hash

if ($actualHash -ne $expectedHash) {
    throw "Windows ISO SHA-256 mismatch: expected $expectedHash, actual $actualHash"
}
```

## Host prerequisites

- Windows 10 or Windows 11 Pro, Enterprise, or Education
- Hardware virtualization enabled in UEFI or BIOS
- A 64-bit processor with second-level address translation
- At least 16 GB host memory; 24 GB or more is preferable while Cubase and development tools are also open on the host
- At least 120 GB free storage for the VM, checkpoints, and temporary installer copies
- The official Windows 11 Enterprise Evaluation image recorded above
- A Cubase Elements 15 license and MySteinberg account

Cubase activation can consume one of the activations available to the account. Plan the activation before creating disposable or duplicate VMs. Never store MySteinberg credentials, activation data, or Windows product keys in the repository.

## Enable Hyper-V

Check whether Hyper-V is available from an elevated PowerShell session on the host:

```powershell
Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All
```

If it is disabled, enable it and restart Windows:

```powershell
Enable-WindowsOptionalFeature `
  -Online `
  -FeatureName Microsoft-Hyper-V-All `
  -All
```

Hyper-V is not available as an optional feature on Windows Home editions.

## Create the virtual machine

1. Open **Hyper-V Manager**.
2. Select the local Hyper-V host.
3. Select **New > Virtual Machine**.
4. Name the VM `TempoFlow Release Validation`.
5. Select **Generation 2**.
6. Assign `8192 MB` startup memory.
7. Leave **Use Dynamic Memory** disabled. Fixed memory removes one source of variation between validation runs.
8. Connect the network adapter to **Default Switch**.
9. Create a dynamically expanding `100 GB` VHDX.
10. Select the verified Windows evaluation ISO as the boot image.
11. Finish the wizard, but do not start the VM yet.

Open the VM settings and apply these values:

1. Under **Processor**, assign `4` virtual processors.
2. Under **Security**, keep Secure Boot enabled with the **Microsoft Windows** template.
3. For a Windows 11 guest, enable the virtual TPM. Windows 11 Hyper-V guests require Generation 2, Secure Boot, at least 4 GB memory, at least two virtual processors, and TPM support.
4. Under **Checkpoints**, disable automatic checkpoints.
5. Set the checkpoint type to **Production checkpoints only**.
6. Under **Integration Services**, enable **Guest Service Interface**. This allows controlled host-to-guest copies with `Copy-VMFile`.
7. Set **Automatic Stop Action** to **Shut down the guest operating system**.

Do not enable processor compatibility mode. The validation VM is not intended for live migration between unrelated hosts, and hiding processor features can change application behavior.

## Install and record Windows

1. Start the VM and install Windows normally.
2. Create a dedicated local Windows account for release validation.
3. Apply the intended Windows security updates.
4. Install Hyper-V integration-service updates through Windows Update when offered.
5. Set the display resolution required for Cubase, but do not install unrelated development tools.
6. Record the Windows edition, version, OS build, ISO filename, and ISO SHA-256 in the VM baseline record.
7. Shut down the guest cleanly.
8. Create a production checkpoint named `windows-clean`.

Calculate the ISO hash on the host before archiving the baseline record:

```powershell
Get-FileHash `
  -LiteralPath 'C:\Path\To\Windows.iso' `
  -Algorithm SHA256
```

The exact OS build is part of the test evidence. Updating the guest changes the baseline and requires a new baseline record and checkpoint.

## Install Cubase Elements 15

1. Start the VM from `windows-clean`.
2. Download Cubase from Steinberg's official download source or use a previously verified installer.
3. Verify and record the Cubase installer version and hash.
4. Install Cubase Elements 15.0.40 Build 324 x64.
5. Install the Steinberg built-in ASIO driver if no usable virtual audio device is available.
6. Skip optional content libraries that are not required for TempoFlow validation.
7. Sign in through Steinberg Licensing and activate Cubase for this VM.
8. Start Cubase once and confirm that it opens as Cubase Elements 15.0.40 Build 324.
9. Close Cubase and shut down Windows cleanly.
10. Create a production checkpoint named `cubase-15.0.40-clean`.

After restoring this checkpoint, confirm that Cubase remains activated. If Steinberg Licensing rejects the restored state, do not repeatedly clone or reactivate the VM. Keep one VM identity and manage its activation through Steinberg Activation Manager.

## Configure the VM connection

Use Hyper-V Enhanced Session Mode when available:

1. Open the VM connection window.
2. Select **Show Options > Local Resources**.
3. Set remote audio playback to **Play on this computer**.
4. Do not share the host system drive or the complete TempoFlow repository.

Audio redirection is only for confirming that the plug-in produces audible output. Do not use its latency or timing as release evidence.

## Restore the baseline

Before every release-package validation:

1. Shut down the VM.
2. Apply the `cubase-15.0.40-clean` checkpoint.
3. Start the VM.
4. Confirm the recorded Windows and Cubase versions.
5. Confirm that no TempoFlow system-wide or per-user development installation exists.

The corresponding host-side PowerShell command is:

```powershell
Restore-VMCheckpoint `
  -VMName 'TempoFlow Release Validation' `
  -Name 'cubase-15.0.40-clean' `
  -Confirm:$false
```

Checkpoints are test-state controls, not backups. Export the powered-off baseline VM separately if rebuilding it would be costly.

## Transfer the release candidate

Copy only the installer and its checksum into the guest. Run these commands from an elevated host PowerShell session:

```powershell
$vmName = 'TempoFlow Release Validation'
$packageRoot = 'C:\Users\joerg\Documents\src\tempflow\build\windows-x64-release\package'
$installerName = 'TempoFlow-0.1.0-windows-x64.exe'

Enable-VMIntegrationService `
  -VMName $vmName `
  -Name 'Guest Service Interface'

Copy-VMFile `
  -VMName $vmName `
  -SourcePath (Join-Path $packageRoot $installerName) `
  -DestinationPath "C:\Validation\$installerName" `
  -FileSource Host `
  -CreateFullPath

Copy-VMFile `
  -VMName $vmName `
  -SourcePath (Join-Path $packageRoot "$installerName.sha256") `
  -DestinationPath "C:\Validation\$installerName.sha256" `
  -FileSource Host `
  -CreateFullPath
```

Adjust the host package path and installer version when required. Do not map a writable host source directory into the guest.

Verify the copied installer inside the guest:

```powershell
$installer = 'C:\Validation\TempoFlow-0.1.0-windows-x64.exe'
$recordedHash = ((Get-Content -Raw "$installer.sha256").Trim() -split '\s+')[0]
$actualHash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash

if ($actualHash -ne $recordedHash) {
    throw "Installer SHA-256 mismatch: expected $recordedHash, actual $actualHash"
}
```

## Validate installation

1. Confirm that these paths do not exist before installation:
   - `%LOCALAPPDATA%\Programs\Common\VST3\TempoFlow.vst3`
   - `%APPDATA%\VST3 Presets\jfheinrich\TempoFlow`
   - `%ProgramFiles%\Common Files\VST3\TempoFlow.vst3`
   - `%ProgramData%\VST3 Presets\jfheinrich\TempoFlow`
2. Run the verified installer interactively.
3. Confirm that Windows requests administrator approval.
4. Accept the displayed TempoFlow license and complete the installation.
5. Verify the complete VST3 bundle at `%ProgramFiles%\Common Files\VST3\TempoFlow.vst3`.
6. Verify exactly seven `.vstpreset` files at `%ProgramData%\VST3 Presets\jfheinrich\TempoFlow`.
7. Verify the project and dependency license files at `%ProgramFiles%\TempoFlow`.
8. Search the installed TempoFlow paths and confirm that no `.tempoflow` file was installed.
9. Confirm that **TempoFlow 0.1.0** appears in Windows **Installed apps**.

## Validate Cubase integration

1. Start Cubase and wait for VST3 scanning to finish.
2. Confirm that TempoFlow is not blocklisted and appears as an instrument.
3. Create an empty project and add one TempoFlow instrument track.
4. Confirm functional playback through the redirected or built-in audio device.
5. Confirm that all seven factory presets are visible.
6. Load at least one non-default preset and verify its expected audible state.
7. Verify the documented preset catalogue fields in MediaBay.
8. Save a Cubase-managed user preset.
9. Confirm that the user preset is stored below `%USERPROFILE%\Documents\VST3 Presets\jfheinrich\TempoFlow`.
10. Save, close, and reopen the Cubase project. Confirm that the non-default state is restored.
11. Close Cubase before uninstalling TempoFlow.

Do not use this VM run as evidence for ASIO buffer-size, latency, dropout, or sample-accuracy acceptance.

## Validate uninstall and user-data preservation

1. Uninstall **TempoFlow 0.1.0** through Windows **Installed apps**.
2. Verify that `%ProgramFiles%\Common Files\VST3\TempoFlow.vst3` was removed.
3. Verify that the seven TempoFlow factory presets were removed from `%ProgramData%`.
4. Verify that the TempoFlow license directory under `%ProgramFiles%` was removed.
5. Verify that the Cubase-managed user preset still exists below `%USERPROFILE%\Documents\VST3 Presets\jfheinrich\TempoFlow`.
6. Verify that unrelated VST3 plug-ins and preset directories were not modified.
7. Start Cubase and confirm that TempoFlow is no longer available after rescanning.

Do not clean up the guest manually to prepare the next run. Restore the baseline checkpoint instead.

## Record the result

Copy this record into the pull request or release evidence:

```text
Date:
Tester:
TempoFlow commit:
Installer filename:
Installer SHA-256:
VM name:
VM generation:
VM checkpoint:
Virtual processors:
Assigned memory:
Windows edition and version:
Windows OS build:
Windows ISO filename and SHA-256:
Cubase edition and version:
Cubase installer SHA-256:

Baseline restored before test: PASS / FAIL
Installer checksum: PASS / FAIL
Administrator prompt: PASS / FAIL
VST3 system installation: PASS / FAIL
Factory preset installation: PASS / FAIL
Factory preset count: 7 / other
Raw .tempoflow exclusion: PASS / FAIL
License files installed: PASS / FAIL
Windows Installed apps entry: PASS / FAIL
Cubase plug-in discovery: PASS / FAIL
Cubase factory preset discovery: PASS / FAIL
Non-default preset load: PASS / FAIL
Cubase user preset save: PASS / FAIL
Cubase project-state restoration: PASS / FAIL
Uninstall completed: PASS / FAIL
System VST3 removed: PASS / FAIL
Factory presets removed: PASS / FAIL
License files removed: PASS / FAIL
User preset preserved: PASS / FAIL
Unrelated VST3 content unchanged: PASS / FAIL

Result: PASS / FAIL
Notes:
```

Issue #28 can be closed only after this record passes without an unexplained deviation.

## References

- [Hyper-V requirements](https://learn.microsoft.com/windows-server/virtualization/hyper-v/host-hardware-requirements?pivots=windows)
- [Install Hyper-V](https://learn.microsoft.com/windows-server/virtualization/hyper-v/get-started/install-hyper-v?pivots=windows)
- [Windows 11 VM requirements](https://learn.microsoft.com/windows/whats-new/windows-11-requirements)
- [Hyper-V checkpoints](https://learn.microsoft.com/windows-server/virtualization/hyper-v/checkpoints)
- [Hyper-V integration services](https://learn.microsoft.com/windows-server/virtualization/hyper-v/integration-services)
- [Hyper-V Enhanced Session Mode](https://learn.microsoft.com/windows-server/virtualization/hyper-v/enhanced-session-mode)
- [Cubase 15 downloads](https://o.steinberg.net/en/support/downloads/cubase_15.html)
- [Steinberg system requirements](https://www.steinberg.net/system-requirements/)
