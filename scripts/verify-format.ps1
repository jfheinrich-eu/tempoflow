[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'format-common.ps1')

$formatCommand = '.\scripts\format.ps1'

try {
    $repositoryRoot = Get-TempoFlowRepositoryRoot
    $formatter = Get-TempoFlowFormatter
    $files = Get-TempoFlowFormatFiles -RepositoryRoot $repositoryRoot

    foreach ($path in $files.SourceFiles) {
        Invoke-TempoFlowClangFormat -Formatter $formatter -RepositoryRoot $repositoryRoot -Path $path -Kind Source -Mode Verify
    }

    foreach ($path in $files.JsonFiles) {
        Assert-TempoFlowTextConvention -RepositoryRoot $repositoryRoot -Path $path -Kind 'JSON'
        Assert-TempoFlowJsonSyntax -RepositoryRoot $repositoryRoot -Path $path -Kind 'JSON'
        Invoke-TempoFlowClangFormat -Formatter $formatter -RepositoryRoot $repositoryRoot -Path $path -Kind Json -Mode Verify
    }

    foreach ($path in $files.PresetFiles) {
        Assert-TempoFlowTextConvention -RepositoryRoot $repositoryRoot -Path $path -Kind 'TempoFlow preset JSON'
        Assert-TempoFlowJsonSyntax -RepositoryRoot $repositoryRoot -Path $path -Kind 'TempoFlow preset JSON'
        Invoke-TempoFlowClangFormat -Formatter $formatter -RepositoryRoot $repositoryRoot -Path $path -Kind Preset -Mode Verify
    }

    Invoke-TempoFlowDiffCheck -RepositoryRoot $repositoryRoot
}
catch {
    Write-Error "$($_.Exception.Message) Run $formatCommand and retry."
    exit 1
}

Write-Output (
    'Formatting verified: {0} C/C++ files, {1} JSON files, {2} TempoFlow preset files; Git whitespace clean.' -f
        $files.SourceFiles.Count,
        $files.JsonFiles.Count,
        $files.PresetFiles.Count
)
