[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $ValidatorPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$temporaryRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$testRoot = Join-Path $temporaryRoot ('tempoflow-reparse-test-' + [guid]::NewGuid().ToString('N'))
$scanDirectory = Join-Path $testRoot 'scan'
$targetDirectory = Join-Path $testRoot 'target'
$targetFile = Join-Path $targetDirectory 'target.tempoflow'
$junctionPath = Join-Path $scanDirectory 'linked-presets'

if (-not $testRoot.StartsWith(
        $temporaryRoot,
        [System.StringComparison]::OrdinalIgnoreCase
    )) {
    throw 'Refusing to create the fixture outside the system temporary directory.'
}

New-Item -ItemType Directory -Path $scanDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null
New-Item -ItemType File -Path $targetFile | Out-Null

try {
    New-Item -ItemType Junction -Path $junctionPath -Target $targetDirectory | Out-Null

    $output = (& $ValidatorPath $scanDirectory 2>&1 | Out-String)
    $exitCode = $LASTEXITCODE

    if ($exitCode -ne 1) {
        throw "Expected exit code 1 for a reparse point, received $exitCode."
    }

    if ($output -notmatch 'symbolic links and reparse points are not allowed') {
        throw 'The validator did not report the rejected reparse point.'
    }
}
finally {
    if (Test-Path -LiteralPath $junctionPath) {
        [System.IO.Directory]::Delete($junctionPath)
    }
    if (Test-Path -LiteralPath $targetFile) {
        Remove-Item -LiteralPath $targetFile -Force
    }
    if (Test-Path -LiteralPath $targetDirectory) {
        [System.IO.Directory]::Delete($targetDirectory)
    }
    if (Test-Path -LiteralPath $scanDirectory) {
        [System.IO.Directory]::Delete($scanDirectory)
    }
    if (Test-Path -LiteralPath $testRoot) {
        [System.IO.Directory]::Delete($testRoot)
    }
}
