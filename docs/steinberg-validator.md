# Steinberg VST 3 Validator

The Steinberg VST 3 Validator is the reference command-line test host for checking a VST3 bundle against the VST 3 interface contract. Run it in addition to TempoFlow's unit tests and a manual Cubase test.

## Obtain the SDK

1. Open the [official Steinberg VST 3 SDK page](https://www.steinberg.net/developers/vstsdk/).
2. Download the **VST 3 Audio Plug-Ins SDK** ZIP archive.
3. Create a directory outside the TempoFlow repository, for example:

   ```text
   C:\Tools\Steinberg\VST3_SDK
   ```

4. Extract the complete archive into that directory.
5. Search the extracted SDK for a prebuilt validator:

   ```powershell
   Get-ChildItem 'C:\Tools\Steinberg\VST3_SDK' `
     -Filter validator.exe `
     -File `
     -Recurse
   ```

Do not copy the SDK or `validator.exe` into the TempoFlow repository. It is an external development tool.

## Build the validator when necessary

If the downloaded package does not contain a suitable `validator.exe`, build the `validator` target from the SDK sources.

Open **Developer PowerShell for VS 18**, then run:

```powershell
cd C:\Tools\Steinberg

git clone --recursive https://github.com/steinbergmedia/vst3sdk.git
cd vst3sdk

cmake -S . -B build `
  -G 'Visual Studio 18 2026' `
  -A x64 `
  -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF `
  -DSMTG_ENABLE_VSTGUI_SUPPORT=OFF `
  -DSMTG_CREATE_PLUGIN_LINK=0

cmake --build build `
  --config Release `
  --target validator
```

Locate the resulting executable:

```powershell
Get-ChildItem .\build `
  -Filter validator.exe `
  -File `
  -Recurse
```

Steinberg's current support matrix lists Visual Studio 2026 for Windows 11. For Windows 10, build the validator with Visual Studio 2022 if the Visual Studio 2026 build is not supported:

```powershell
cmake -S . -B build-vs2022 `
  -G 'Visual Studio 17 2022' `
  -A x64 `
  -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF `
  -DSMTG_ENABLE_VSTGUI_SUPPORT=OFF `
  -DSMTG_CREATE_PLUGIN_LINK=0

cmake --build build-vs2022 `
  --config Release `
  --target validator
```

The validator may be built with a different supported MSVC version than TempoFlow. It loads and tests the compiled VST3 bundle through the VST 3 interface.

## Build TempoFlow for validation

From the TempoFlow repository in **Developer PowerShell for VS 18**:

```powershell
cmake --fresh --preset windows-x64-release
cmake --build --preset windows-x64-release
```

The bundle to validate is:

```text
build\windows-x64-release\TempoFlowPlugin_artefacts\Release\VST3\TempoFlow.vst3
```

## Run the validator

Assign the actual validator path found or built above:

```powershell
$validator = 'C:\Tools\Steinberg\VST3_SDK\path\to\validator.exe'
$plugin = (Resolve-Path `
  '.\build\windows-x64-release\TempoFlowPlugin_artefacts\Release\VST3\TempoFlow.vst3').Path

& $validator $plugin
if ($LASTEXITCODE -ne 0) {
  throw "VST 3 validation failed with exit code $LASTEXITCODE."
}
```

To display the output and save it for investigation:

```powershell
& $validator $plugin 2>&1 |
  Tee-Object -FilePath .\validator-test.txt

$validatorExitCode = $LASTEXITCODE
if ($validatorExitCode -ne 0) {
  throw "VST 3 validation failed with exit code $validatorExitCode."
}
```

`validator-test.txt` is a local diagnostic artifact and should not normally be committed.

## Interpret the result

A successful validation must meet both conditions:

- the final summary reports zero failed tests;
- `validator.exe` returns exit code `0`.

Warnings about unsupported optional interfaces may be acceptable when the feature is intentionally absent. Any entry marked `Failed`, any `ERROR` line, or a non-zero exit code must be investigated before the VST3 scaffold is accepted.

### Unnamed factory program

A result such as the following indicates that the processor exposes a factory program without a valid name:

```text
ERROR: Programlist 000->Program 000: has no name!!!
Result: 46 tests passed, 1 tests failed
```

The implementation must return a stable, non-empty name for program index `0`. Add a regression test and repeat validation after correcting the processor.

## References

- [Steinberg VST 3 SDK download](https://www.steinberg.net/developers/vstsdk/)
- [Official VST 3 SDK repository and build instructions](https://github.com/steinbergmedia/vst3sdk)
- [Validator target source](https://github.com/steinbergmedia/vst3_public_sdk/tree/master/samples/vst-hosting/validator)
