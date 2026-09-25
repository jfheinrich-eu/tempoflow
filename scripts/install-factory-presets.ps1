[CmdletBinding(SupportsShouldProcess, ConfirmImpact = 'Medium')]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Release',

    [string] $SourceDirectory,

    [string] $DestinationDirectory = (Join-Path `
        ([Environment]::GetFolderPath([Environment+SpecialFolder]::ApplicationData)) `
        'VST3 Presets\jfheinrich\TempoFlow')
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

if ([string]::IsNullOrWhiteSpace($SourceDirectory)) {
    $repositoryRoot = Split-Path -Parent $PSScriptRoot
    $presetName = "windows-x64-$($Configuration.ToLowerInvariant())"
    $SourceDirectory = Join-Path $repositoryRoot "build\$presetName\factory-presets\$Configuration"
}

$resolvedSource = (Resolve-Path -LiteralPath $SourceDirectory -ErrorAction Stop).Path
$availablePresets = @(
    Get-ChildItem -LiteralPath $resolvedSource -Filter '*.vstpreset' -File |
        Sort-Object -Property Name
)
$availableNames = @($availablePresets.Name)

$missingPresets = @($expectedPresetNames | Where-Object { $_ -notin $availableNames })
$unexpectedPresets = @($availableNames | Where-Object { $_ -notin $expectedPresetNames })

if ($missingPresets.Count -gt 0) {
    throw "Missing generated factory presets: $($missingPresets -join ', ')"
}

if ($unexpectedPresets.Count -gt 0) {
    throw "Unexpected generated factory presets: $($unexpectedPresets -join ', ')"
}

if ($PSCmdlet.ShouldProcess($DestinationDirectory, 'Install seven TempoFlow VST3 factory presets')) {
    $null = New-Item -ItemType Directory -Path $DestinationDirectory -Force

    foreach ($presetName in $expectedPresetNames) {
        $source = Join-Path $resolvedSource $presetName
        $destination = Join-Path $DestinationDirectory $presetName
        Copy-Item -LiteralPath $source -Destination $destination -Force
        Write-Output "Installed: $destination"
    }
}
