[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'format-common.ps1')

$repositoryRoot = Get-TempoFlowRepositoryRoot
$formatter = Get-TempoFlowFormatter
$files = Get-TempoFlowFormatFiles -RepositoryRoot $repositoryRoot

foreach ($path in $files.JsonFiles) {
    Assert-TempoFlowJsonSyntax -RepositoryRoot $repositoryRoot -Path $path -Kind 'JSON'
}

foreach ($path in $files.PresetFiles) {
    Assert-TempoFlowJsonSyntax -RepositoryRoot $repositoryRoot -Path $path -Kind 'TempoFlow preset JSON'
}

foreach ($path in $files.SourceFiles) {
    Invoke-TempoFlowClangFormat -Formatter $formatter -RepositoryRoot $repositoryRoot -Path $path -Kind Source -Mode Format
}

foreach ($path in $files.JsonFiles) {
    Invoke-TempoFlowClangFormat -Formatter $formatter -RepositoryRoot $repositoryRoot -Path $path -Kind Json -Mode Format
    Assert-TempoFlowTextConvention -RepositoryRoot $repositoryRoot -Path $path -Kind 'JSON'
    Assert-TempoFlowJsonSyntax -RepositoryRoot $repositoryRoot -Path $path -Kind 'JSON'
}

foreach ($path in $files.PresetFiles) {
    Invoke-TempoFlowClangFormat -Formatter $formatter -RepositoryRoot $repositoryRoot -Path $path -Kind Preset -Mode Format
    Assert-TempoFlowTextConvention -RepositoryRoot $repositoryRoot -Path $path -Kind 'TempoFlow preset JSON'
    Assert-TempoFlowJsonSyntax -RepositoryRoot $repositoryRoot -Path $path -Kind 'TempoFlow preset JSON'
}

Write-Output (
    'Formatting complete: {0} C/C++ files, {1} JSON files, {2} TempoFlow preset files.' -f
        $files.SourceFiles.Count,
        $files.JsonFiles.Count,
        $files.PresetFiles.Count
)
