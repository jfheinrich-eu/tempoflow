[CmdletBinding()]
param(
    [ValidateSet('Release')]
    [string] $Configuration = 'Release',

    [string] $BuildDirectory,

    [string] $PackageDirectory,

    [string] $NsisExecutable
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($env:OS -ne 'Windows_NT') {
    throw 'The TempoFlow Windows installer can only be built on Windows.'
}

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $repositoryRoot 'build\windows-x64-release'
}
elseif (-not [IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory = Join-Path $repositoryRoot $BuildDirectory
}
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)

if ([string]::IsNullOrWhiteSpace($PackageDirectory)) {
    $PackageDirectory = Join-Path $BuildDirectory 'package'
}
elseif (-not [IO.Path]::IsPathRooted($PackageDirectory)) {
    $PackageDirectory = Join-Path $repositoryRoot $PackageDirectory
}
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)

function Get-NsisExecutable {
    if (-not [string]::IsNullOrWhiteSpace($NsisExecutable)) {
        return (Resolve-Path -LiteralPath $NsisExecutable -ErrorAction Stop).Path
    }

    if (-not [string]::IsNullOrWhiteSpace($env:TEMPOFLOW_NSIS_EXECUTABLE)) {
        return (Resolve-Path -LiteralPath $env:TEMPOFLOW_NSIS_EXECUTABLE -ErrorAction Stop).Path
    }

    $command = Get-Command makensis -CommandType Application -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        return $command.Source
    }

    $programFilesX86 = [Environment]::GetFolderPath([Environment+SpecialFolder]::ProgramFilesX86)
    $candidates = @(
        'C:\Tools\NSIS-3.13\nsis-3.13\makensis.exe'
        (Join-Path $programFilesX86 'NSIS\makensis.exe')
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw 'NSIS 3.13 was not found. Pass -NsisExecutable or set TEMPOFLOW_NSIS_EXECUTABLE.'
}

foreach ($commandName in @('cmake', 'cpack')) {
    if ($null -eq (Get-Command $commandName -CommandType Application -ErrorAction SilentlyContinue)) {
        throw "$commandName was not found. Run this script from Developer PowerShell for VS 18."
    }
}

$resolvedNsisExecutable = Get-NsisExecutable
$nsisVersion = (& $resolvedNsisExecutable /VERSION).Trim()
if ($LASTEXITCODE -ne 0 -or $nsisVersion -ne 'v3.13') {
    throw "NSIS 3.13 is required. Detected: '$nsisVersion'"
}

$cmakeCache = Join-Path $BuildDirectory 'CMakeCache.txt'
if (-not (Test-Path -LiteralPath $cmakeCache -PathType Leaf)) {
    $defaultBuildDirectory = [IO.Path]::GetFullPath((Join-Path $repositoryRoot 'build\windows-x64-release'))
    if (-not $BuildDirectory.Equals($defaultBuildDirectory, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Custom build directory is not configured: $BuildDirectory"
    }

    & cmake --preset windows-x64-release
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed with exit code $LASTEXITCODE."
    }
}
else {
    & cmake -S $repositoryRoot -B $BuildDirectory
    if ($LASTEXITCODE -ne 0) {
        throw "CMake reconfiguration failed with exit code $LASTEXITCODE."
    }
}

& cmake --build $BuildDirectory --config $Configuration --target TempoFlowPlugin_VST3 TempoFlowFactoryPresets
if ($LASTEXITCODE -ne 0) {
    throw "Installer input build failed with exit code $LASTEXITCODE."
}

$cpackConfig = Join-Path $BuildDirectory 'CPackConfig.cmake'
if (-not (Test-Path -LiteralPath $cpackConfig -PathType Leaf)) {
    throw "CPack configuration was not generated: $cpackConfig"
}

$null = New-Item -ItemType Directory -Path $PackageDirectory -Force
& cpack `
    --config $cpackConfig `
    -C $Configuration `
    -G NSIS64 `
    -B $PackageDirectory `
    -D "CPACK_NSIS_EXECUTABLE=$resolvedNsisExecutable"
if ($LASTEXITCODE -ne 0) {
    throw "CPack failed with exit code $LASTEXITCODE."
}

$packageNamePath = Join-Path $BuildDirectory 'TempoFlowPackageName.txt'
if (-not (Test-Path -LiteralPath $packageNamePath -PathType Leaf)) {
    throw "Package name metadata was not generated: $packageNamePath"
}
$packageBaseName = (Get-Content -Raw -LiteralPath $packageNamePath).Trim()
if ($packageBaseName -notmatch '^TempoFlow-\d+\.\d+\.\d+-windows-x64$') {
    throw "Unexpected Windows package name: '$packageBaseName'"
}
$installerPath = Join-Path $PackageDirectory "$packageBaseName.exe"
& (Join-Path $repositoryRoot 'tests\windows-installer.tests.ps1') `
    -BuildDirectory $BuildDirectory `
    -Configuration $Configuration `
    -PackageDirectory $PackageDirectory `
    -InstallerPath $installerPath
if ($LASTEXITCODE -ne 0) {
    throw "Windows installer verification failed with exit code $LASTEXITCODE."
}

Write-Output "Windows installer created and verified: $installerPath"
