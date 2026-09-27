[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $GeneratorPath,

    [Parameter(Mandatory)]
    [string] $PluginPath,

    [Parameter(Mandatory)]
    [string] $SourceDirectory,

    [Parameter(Mandatory)]
    [string] $OutputRoot
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedPresetNames = @(
    '12-8 Blues.vstpreset'
    'Beat 1 Only.vstpreset'
    'Beats 2 + 4.vstpreset'
    'Slow Blues Shuffle.vstpreset'
    'Standard 3-4.vstpreset'
    'Standard 4-4.vstpreset'
    'Standard 6-8.vstpreset'
)

$firstOutput = Join-Path $OutputRoot 'first'
$secondOutput = Join-Path $OutputRoot 'second'

foreach ($outputDirectory in @($firstOutput, $secondOutput)) {
    & $GeneratorPath $PluginPath $SourceDirectory $outputDirectory
    if ($LASTEXITCODE -ne 0) {
        throw "Factory preset generation failed with exit code $LASTEXITCODE."
    }
}

function Get-PresetHashes {
    param(
        [Parameter(Mandatory)]
        [string] $Directory
    )

    $files = @(
        Get-ChildItem -LiteralPath $Directory -Filter '*.vstpreset' -File |
            Sort-Object -Property Name
    )
    $names = @($files.Name)
    $missing = @($expectedPresetNames | Where-Object { $_ -notin $names })
    $unexpected = @($names | Where-Object { $_ -notin $expectedPresetNames })

    if ($missing.Count -gt 0) {
        throw "Missing generated factory presets: $($missing -join ', ')"
    }
    if ($unexpected.Count -gt 0) {
        throw "Unexpected generated factory presets: $($unexpected -join ', ')"
    }

    return @(
        $files | ForEach-Object {
            [PSCustomObject]@{
                Name = $_.Name
                Hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
            }
        }
    )
}

$firstHashes = @(Get-PresetHashes -Directory $firstOutput)
$secondHashes = @(Get-PresetHashes -Directory $secondOutput)
$differences = @(Compare-Object $firstHashes $secondHashes -Property Name, Hash)

if ($differences.Count -gt 0) {
    throw 'Repeated factory preset generation produced different files.'
}

Write-Output "Factory preset generation is byte-reproducible for $($firstHashes.Count) presets."
