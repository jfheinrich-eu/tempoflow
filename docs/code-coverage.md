# Code Coverage

TempoFlow measures native C++ line coverage for the preset-validation, host-timing, and click-engine libraries on Windows. The current workflow does not yet collect the typed preset runtime model or plugin-processor tests. Coverage is collected from the selected Debug unit-test executables with Microsoft Code Coverage, merged, and uploaded to Codecov as Cobertura XML.

Only production sources below `src/` are included. JUCE, tests, tools, documentation, and generated build files are excluded from the reported project coverage.

## Pull-request behavior

The `Coverage` workflow runs for pushes to `main`, pull requests, and manual dispatches. Codecov reports project and patch coverage as informational values. Coverage does not block pull-request approval while the project establishes a reliable baseline.

The upload uses GitHub OIDC and does not require a `CODECOV_TOKEN` repository secret. The Codecov GitHub App must be enabled for the repository. Coverage collection and upload run in separate jobs. Pull-request code executes only in the collection job, which has read-only repository permission and no OIDC permission. The upload job receives only the validated Cobertura artifact and owns the short-lived OIDC permission.

## Local collection

Build the instrumented Debug test executables from Developer PowerShell for VS 18:

```powershell
cmake --preset windows-x64-debug -B build/coverage
cmake --build build/coverage --config Debug `
  --target TempoFlowPresetValidatorTests TempoFlowHostTimingTests TempoFlowClickEngineTests
```

Locate `Microsoft.CodeCoverage.Console.exe` below the active Visual Studio installation, then collect the report:

```powershell
$coverageTool = Join-Path $env:VSINSTALLDIR `
  'Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\Microsoft.CodeCoverage.Console.exe'
$presetTestExecutable = (Resolve-Path `
  'build\coverage\TempoFlowPresetValidatorTests_artefacts\Debug\TempoFlowPresetValidatorTests.exe').Path
$timingTestExecutable = (Resolve-Path `
  'build\coverage\Debug\TempoFlowHostTimingTests.exe').Path
$clickTestExecutable = (Resolve-Path `
  'build\coverage\Debug\TempoFlowClickEngineTests.exe').Path

& $coverageTool collect `
  --include-files $presetTestExecutable `
  --output 'build\coverage\preset-tests.coverage' `
  --output-format coverage `
  --nologo `
  $presetTestExecutable

& $coverageTool collect `
  --include-files $timingTestExecutable `
  --output 'build\coverage\timing-tests.coverage' `
  --output-format coverage `
  --nologo `
  $timingTestExecutable

& $coverageTool collect `
  --include-files $clickTestExecutable `
  --output 'build\coverage\click-tests.coverage' `
  --output-format coverage `
  --nologo `
  $clickTestExecutable

& $coverageTool merge `
  'build\coverage\preset-tests.coverage' `
  'build\coverage\timing-tests.coverage' `
  'build\coverage\click-tests.coverage' `
  --output 'build\coverage\coverage.cobertura.xml' `
  --output-format cobertura `
  --nologo
```

Each collection command fails if its test executable fails. Review `build/coverage/coverage.cobertura.xml` before changing Codecov targets or thresholds.
