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

$expectedMusicalStyles = @{
    '12-8 Blues.vstpreset' = 'Blues'
    'Slow Blues Shuffle.vstpreset' = 'Blues'
}

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

function Get-PresetMetaInfo {
    param(
        [Parameter(Mandatory)]
        [string] $Path
    )

    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $reader = [System.IO.BinaryReader]::new($stream)
        if ([System.Text.Encoding]::ASCII.GetString($reader.ReadBytes(4)) -ne 'VST3') {
            throw "Invalid VST3 preset header: $Path"
        }

        [void] $reader.ReadInt32()
        [void] $reader.ReadBytes(32)
        $chunkListOffset = $reader.ReadInt64()
        $stream.Position = $chunkListOffset

        if ([System.Text.Encoding]::ASCII.GetString($reader.ReadBytes(4)) -ne 'List') {
            throw "Missing VST3 preset chunk list: $Path"
        }

        $entryCount = $reader.ReadInt32()
        $infoOffset = $null
        $infoSize = $null
        for ($index = 0; $index -lt $entryCount; $index++) {
            $chunkId = [System.Text.Encoding]::ASCII.GetString($reader.ReadBytes(4))
            $chunkOffset = $reader.ReadInt64()
            $chunkSize = $reader.ReadInt64()
            if ($chunkId -eq 'Info') {
                $infoOffset = $chunkOffset
                $infoSize = $chunkSize
            }
        }

        if ($null -eq $infoOffset -or $null -eq $infoSize -or $infoSize -le 0) {
            throw "Missing VST3 preset metadata chunk: $Path"
        }
        if ($infoSize -gt [int]::MaxValue) {
            throw "VST3 preset metadata is too large: $Path"
        }

        $stream.Position = $infoOffset
        $xmlText = [System.Text.Encoding]::UTF8.GetString($reader.ReadBytes([int] $infoSize))
        [xml] $xml = $xmlText
        $attributes = @{}
        foreach ($attribute in @($xml.MetaInfo.Attribute)) {
            $attributes[[string] $attribute.id] = [string] $attribute.value
        }

        return $attributes
    }
    finally {
        $stream.Dispose()
    }
}

function Assert-PresetMetaInfo {
    param(
        [Parameter(Mandatory)]
        [string] $Directory
    )

    foreach ($presetName in $expectedPresetNames) {
        $metadata = Get-PresetMetaInfo -Path (Join-Path $Directory $presetName)
        $expectedValues = @{
            MediaAuthor = 'Jörg Heinrich'
            MediaType = 'VstPreset'
            PlugInName = 'TempoFlow'
            PlugInVendor = 'jfheinrich'
            PlugInCategory = 'Instrument|Synth'
            VST3UniqueID = 'ABCDEF019182FAEB4A66686554666C6F'
            MusicalCategory = 'Drum&Perc'
            MusicalInstrument = 'Drum&Perc|Beats'
            MusicalCharacter = 'Percussive'
        }

        foreach ($entry in $expectedValues.GetEnumerator()) {
            if ($metadata[$entry.Key] -ne $entry.Value) {
                throw "Unexpected $($entry.Key) metadata in ${presetName}: '$($metadata[$entry.Key])'"
            }
        }

        $expectedStyle = $expectedMusicalStyles[$presetName]
        if ($null -ne $expectedStyle -and $metadata['MusicalStyle'] -ne $expectedStyle) {
            throw "Unexpected MusicalStyle metadata in ${presetName}: '$($metadata['MusicalStyle'])'"
        }
        if ($null -eq $expectedStyle -and $metadata.ContainsKey('MusicalStyle')) {
            throw "Unexpected MusicalStyle metadata in ${presetName}: '$($metadata['MusicalStyle'])'"
        }
    }
}

$firstHashes = @(Get-PresetHashes -Directory $firstOutput)
$secondHashes = @(Get-PresetHashes -Directory $secondOutput)
Assert-PresetMetaInfo -Directory $firstOutput
Assert-PresetMetaInfo -Directory $secondOutput
$differences = @(Compare-Object $firstHashes $secondHashes -Property Name, Hash)

if ($differences.Count -gt 0) {
    throw 'Repeated factory preset generation produced different files.'
}

Write-Output "Factory preset generation is byte-reproducible for $($firstHashes.Count) presets."
