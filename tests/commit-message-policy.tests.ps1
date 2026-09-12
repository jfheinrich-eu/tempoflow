[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$validator = Join-Path $PSScriptRoot '..\scripts\validate-commit-message.ps1'
$shell = (Get-Process -Id $PID).Path

$cases = @(
    @{
        Name = 'accepts a conventional commit with sign-off'
        Valid = $true
        Message = "build: require clang tooling`n`nSigned-off-by: Jane Doe <jane@example.com>"
    },
    @{
        Name = 'accepts a breaking commit with both markers'
        Valid = $true
        Message = "feat(api)!: replace the preset contract`n`nBREAKING CHANGE: Consumers must migrate to version 2.`n`nSigned-off-by: Jane Doe <jane@example.com>"
    },
    @{
        Name = 'rejects a non-conventional subject'
        Valid = $false
        Message = "Require clang tooling`n`nSigned-off-by: Jane Doe <jane@example.com>"
    },
    @{
        Name = 'rejects a missing sign-off'
        Valid = $false
        Message = 'build: require clang tooling'
    },
    @{
        Name = 'rejects a breaking marker without a footer'
        Valid = $false
        Message = "feat!: replace the preset contract`n`nSigned-off-by: Jane Doe <jane@example.com>"
    },
    @{
        Name = 'rejects a breaking footer without a marker'
        Valid = $false
        Message = "feat: replace the preset contract`n`nBREAKING CHANGE: Consumers must migrate to version 2.`n`nSigned-off-by: Jane Doe <jane@example.com>"
    }
)

$failures = [System.Collections.Generic.List[string]]::new()

foreach ($case in $cases) {
    & $shell -NoLogo -NoProfile -ExecutionPolicy Bypass -File $validator -MessageText $case.Message *> $null
    $accepted = $LASTEXITCODE -eq 0

    if ($accepted -ne $case.Valid) {
        $failures.Add($case.Name)
    }
}

if ($failures.Count -gt 0) {
    throw "Commit message policy test failures: $($failures -join ', ')"
}

Write-Output "Commit message policy tests passed: $($cases.Count) cases."
