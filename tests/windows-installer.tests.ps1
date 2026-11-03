[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $BuildDirectory,

    [Parameter(Mandatory)]
    [ValidateSet('Release')]
    [string] $Configuration,

    [Parameter(Mandatory)]
    [string] $PackageDirectory,

    [Parameter(Mandatory)]
    [string] $InstallerPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$expectedPresetNames = @(
    '12-8 Blues.vstpreset'
    'Beat 1 Only.vstpreset'
    'Beats 2 + 4.vstpreset'
    'Slow Blues Shuffle.vstpreset'
    'Standard 3-4.vstpreset'
    'Standard 4-4.vstpreset'
    'Standard 6-8.vstpreset'
)
$expectedLegalFileNames = @(
    'JUCE-LICENSE.md'
    'JUCE-SPDX.json'
    'LICENSE'
    'THIRD_PARTY_NOTICES.md'
    'VST3-SDK-LICENSE.txt'
)

$pluginDirectory = Join-Path $BuildDirectory "TempoFlowPlugin_artefacts\$Configuration\VST3\TempoFlow.vst3"
$pluginBinary = Join-Path $pluginDirectory 'Contents\x86_64-win\TempoFlow.vst3'
$moduleInfo = Join-Path $pluginDirectory 'Contents\Resources\moduleinfo.json'
foreach ($requiredPath in @($pluginBinary, $moduleInfo)) {
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "Missing Windows VST3 bundle file: $requiredPath"
    }
}

$presetDirectory = Join-Path $BuildDirectory "factory-presets\$Configuration"
$presetFiles = @(Get-ChildItem -LiteralPath $presetDirectory -Filter '*.vstpreset' -File | Sort-Object Name)
$presetNames = @($presetFiles.Name)
$missingPresets = @($expectedPresetNames | Where-Object { $_ -notin $presetNames })
$unexpectedPresets = @($presetNames | Where-Object { $_ -notin $expectedPresetNames })
if ($missingPresets.Count -gt 0) {
    throw "Missing installer factory presets: $($missingPresets -join ', ')"
}
if ($unexpectedPresets.Count -gt 0) {
    throw "Unexpected installer factory presets: $($unexpectedPresets -join ', ')"
}

$rawPresetFiles = @(
    Get-ChildItem -LiteralPath $pluginDirectory, $presetDirectory -Filter '*.tempoflow' -File -Recurse
)
if ($rawPresetFiles.Count -gt 0) {
    throw "Raw .tempoflow files must not enter the installer: $($rawPresetFiles.FullName -join ', ')"
}

if (-not (Test-Path -LiteralPath $InstallerPath -PathType Leaf)) {
    throw "Windows installer was not created: $InstallerPath"
}
$installerBytes = [IO.File]::ReadAllBytes($InstallerPath)
if ($installerBytes.Length -lt 2 -or $installerBytes[0] -ne 0x4D -or $installerBytes[1] -ne 0x5A) {
    throw "Windows installer does not have a valid PE header: $InstallerPath"
}

$installerVersionInfo = [Diagnostics.FileVersionInfo]::GetVersionInfo($InstallerPath)
$expectedInstallerVersionInfo = [ordered]@{
    CompanyName      = 'jfheinrich'
    FileDescription = 'TempoFlow Windows Installer'
    FileVersion     = '0.1.0'
    OriginalFilename = (Split-Path -Leaf $InstallerPath)
    ProductName     = 'TempoFlow'
    ProductVersion  = '0.1.0'
}
foreach ($propertyName in $expectedInstallerVersionInfo.Keys) {
    $expectedValue = $expectedInstallerVersionInfo[$propertyName]
    $actualValue = $installerVersionInfo.$propertyName
    if ($actualValue -ne $expectedValue) {
        throw "Unexpected installer $propertyName metadata: expected '$expectedValue', actual '$actualValue'"
    }
}

$checksumPath = "$InstallerPath.sha256"
if (-not (Test-Path -LiteralPath $checksumPath -PathType Leaf)) {
    throw "Windows installer checksum was not created: $checksumPath"
}
$recordedChecksum = ((Get-Content -Raw -LiteralPath $checksumPath).Trim() -split '\s+')[0]
$actualChecksum = (Get-FileHash -LiteralPath $InstallerPath -Algorithm SHA256).Hash
if ($recordedChecksum -ne $actualChecksum) {
    throw "Windows installer SHA-256 mismatch: expected $recordedChecksum, actual $actualChecksum"
}

$generatedScripts = @(
    Get-ChildItem -LiteralPath $PackageDirectory -Filter '*.nsi' -File -Recurse
)
if ($generatedScripts.Count -ne 1) {
    throw "Expected one generated NSIS script, found $($generatedScripts.Count)."
}

$nsisScript = Get-Content -Raw -LiteralPath $generatedScripts[0].FullName
if (-not $nsisScript.Contains('RequestExecutionLevel admin', [StringComparison]::Ordinal)) {
    throw 'Generated installer does not request the administrator rights required for system-wide installation.'
}
if (-not $nsisScript.Contains('TempoFlow by jfheinrich', [StringComparison]::Ordinal)) {
    throw 'Generated installer does not contain the expected manufacturer branding.'
}
if ($nsisScript -notmatch '(?ms)Push "Publisher"\s+Push "jfheinrich"') {
    throw 'Generated installer does not register the expected publisher.'
}

$expectedComponents = @(
    [pscustomobject] @{ Id = 'Documentation'; DisplayName = 'Licenses and notices' }
    [pscustomobject] @{ Id = 'FactoryPresets'; DisplayName = 'TempoFlow factory presets' }
    [pscustomobject] @{ Id = 'VST3'; DisplayName = 'TempoFlow VST3' }
)
foreach ($component in $expectedComponents) {
    $visibleDeclaration = "Section `"$($component.DisplayName)`" $($component.Id)"
    $hiddenDeclaration = "Section `"-$($component.DisplayName)`" $($component.Id)"

    if (-not $nsisScript.Contains($visibleDeclaration, [StringComparison]::Ordinal)) {
        throw "Generated installer does not display required component: $($component.DisplayName)"
    }
    if ($nsisScript.Contains($hiddenDeclaration, [StringComparison]::Ordinal)) {
        throw "Generated installer hides required component: $($component.DisplayName)"
    }

    $sectionStart = $nsisScript.IndexOf($visibleDeclaration, [StringComparison]::Ordinal)
    $sectionEnd = $nsisScript.IndexOf('SectionEnd', $sectionStart, [StringComparison]::Ordinal)
    if ($sectionEnd -lt 0) {
        throw "Generated installer component has no section end: $($component.DisplayName)"
    }

    $sectionLength = $sectionEnd - $sectionStart
    $sectionBody = $nsisScript.Substring($sectionStart, $sectionLength)
    if (-not $sectionBody.Contains('SectionIn RO', [StringComparison]::Ordinal)) {
        throw "Generated installer component is not read-only: $($component.DisplayName)"
    }
}

$requiredInstallerPaths = @(
    '$COMMONFILES64\VST3'
    '$COMMONPROGRAMDATA\VST3 Presets\jfheinrich\TempoFlow'
    '$PROGRAMFILES64\TempoFlow'
)
foreach ($requiredInstallerPath in $requiredInstallerPaths) {
    if (-not $nsisScript.Contains($requiredInstallerPath, [StringComparison]::Ordinal)) {
        throw "Generated installer does not contain required destination: $requiredInstallerPath"
    }
}

foreach ($presetName in $expectedPresetNames) {
    if (-not $nsisScript.Contains($presetName, [StringComparison]::Ordinal)) {
        throw "Generated installer does not contain factory preset: $presetName"
    }
}

foreach ($legalFileName in $expectedLegalFileNames) {
    if (-not $nsisScript.Contains($legalFileName, [StringComparison]::Ordinal)) {
        throw "Generated installer does not contain required legal file: $legalFileName"
    }
}
if ($nsisScript.Contains('.tempoflow', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Generated installer contains a raw .tempoflow reference.'
}

$forbiddenUserPaths = @(
    '$APPDATA\VST3 Presets\jfheinrich\TempoFlow'
    '$DOCUMENTS\VST3 Presets\jfheinrich\TempoFlow'
    '$LOCALAPPDATA\Programs\Common\VST3\TempoFlow.vst3'
)
foreach ($forbiddenUserPath in $forbiddenUserPaths) {
    if ($nsisScript.Contains($forbiddenUserPath, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Generated installer must not access a user-scoped TempoFlow path: $forbiddenUserPath"
    }
}

Write-Output 'Windows installer verification passed: VST3 bundle, 7 factory presets, legal files, and system-wide destinations.'
