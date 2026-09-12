# Code Coverage

TempoFlow measures native C++ line coverage for the preset-validation library on Windows. Coverage is collected from the Debug unit-test executable with Microsoft Code Coverage and uploaded to Codecov as Cobertura XML.

Only production sources below `src/` are included. JUCE, tests, tools, documentation, and generated build files are excluded from the reported project coverage.

## Pull-request behavior

The `Coverage` workflow runs for pushes to `main`, pull requests, and manual dispatches. Codecov reports project and patch coverage as informational values. Coverage does not block pull-request approval while the project establishes a reliable baseline.

The upload uses GitHub OIDC and does not require a `CODECOV_TOKEN` repository secret. The Codecov GitHub App must be enabled for the repository. Coverage collection and upload run in separate jobs. Pull-request code executes only in the collection job, which has read-only repository permission and no OIDC permission. The upload job receives only the validated Cobertura artifact and owns the short-lived OIDC permission.

## Local collection

Build the instrumented Debug test executable from Developer PowerShell for VS 18:

```powershell
cmake --preset windows-x64-debug -B build/coverage
cmake --build build/coverage --config Debug --target TempoFlowPresetValidatorTests
```

Locate `Microsoft.CodeCoverage.Console.exe` below the active Visual Studio installation, then collect the report:

```powershell
$coverageTool = Join-Path $env:VSINSTALLDIR `
  'Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\Microsoft.CodeCoverage.Console.exe'
$testExecutable = (Resolve-Path `
  'build\coverage\TempoFlowPresetValidatorTests_artefacts\Debug\TempoFlowPresetValidatorTests.exe').Path

& $coverageTool collect `
  --include-files $testExecutable `
  --output 'build\coverage\coverage.cobertura.xml' `
  --output-format cobertura `
  --nologo `
  $testExecutable
```

The command fails if the test executable fails. Review `build/coverage/coverage.cobertura.xml` before changing Codecov targets or thresholds.
