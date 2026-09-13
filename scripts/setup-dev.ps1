[CmdletBinding(SupportsShouldProcess)]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$requiredCommands = @('git', 'cmake', 'cl', 'code', 'clang-format', 'clang-tidy')
$missingCommands = @(
    $requiredCommands | Where-Object { -not (Get-Command $_ -ErrorAction SilentlyContinue) }
)

if ($missingCommands.Count -gt 0) {
    $message = @(
        "Missing required commands: $($missingCommands -join ', ')."
        'Run this script from Developer PowerShell for VS 18 after installing the documented prerequisites.'
    ) -join ' '
    throw $message
}

$codeVersionText = & code --version
$codeVersion = [version]($codeVersionText | Select-Object -First 1)
if ($codeVersion -lt [version]'1.116.0') {
    throw "VS Code 1.116 or newer is required because it includes Copilot Chat. Detected: $codeVersion"
}

$cmakeVersionText = (& cmake --version) -join "`n"
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to determine the installed CMake version.'
}

$cmakeVersionMatch = [regex]::Match($cmakeVersionText, '(?m)^cmake version (?<version>\d+\.\d+\.\d+)')
if (-not $cmakeVersionMatch.Success) {
    throw 'CMake returned an unrecognized version string.'
}

$cmakeVersion = [version]$cmakeVersionMatch.Groups['version'].Value
if ($cmakeVersion -lt [version]'3.25.0') {
    throw "CMake 3.25 or newer is required by CMakePresets.json. Detected: $cmakeVersion"
}

$cmakeHelp = & cmake --help
$cmakeHelpText = $cmakeHelp -join "`n"
if ($LASTEXITCODE -ne 0 -or $cmakeHelpText -notmatch 'Visual Studio 18 2026') {
    throw 'CMake does not expose the required Visual Studio 18 2026 generator.'
}

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$expectedHooksPath = '.githooks'
$configuredHooksPath = & git -C $repositoryRoot config --local --get core.hooksPath

if ($LASTEXITCODE -notin @(0, 1)) {
    throw 'Failed to read the local Git hooks configuration.'
}

if (-not [string]::IsNullOrWhiteSpace($configuredHooksPath) -and $configuredHooksPath -ne $expectedHooksPath) {
    throw "A different local Git hooks path is already configured: $configuredHooksPath"
}

if ([string]::IsNullOrWhiteSpace($configuredHooksPath)) {
    if ($PSCmdlet.ShouldProcess($repositoryRoot, "Configure Git hooks path as $expectedHooksPath")) {
        & git -C $repositoryRoot config --local core.hooksPath $expectedHooksPath
        if ($LASTEXITCODE -ne 0) {
            throw 'Failed to configure the local Git hooks path.'
        }
    }
}

$requiredExtensions = @(
    'ms-vscode.cpptools',
    'ms-vscode.cmake-tools',
    'jiadongchen.licenseheader',
    'yzhang.markdown-all-in-one',
    'DavidAnson.vscode-markdownlint'
)

$installedExtensions = @(
    & code --list-extensions | ForEach-Object { $_.Trim().ToLowerInvariant() }
)

foreach ($extension in $requiredExtensions) {
    if ($installedExtensions -contains $extension.ToLowerInvariant()) {
        Write-Output "Already installed: $extension"
        continue
    }

    if ($PSCmdlet.ShouldProcess($extension, 'Install or update VS Code extension')) {
        & code --install-extension $extension
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to install VS Code extension: $extension"
        }
    }
}

if ($WhatIfPreference) {
    Write-Output 'Validation complete. Missing setup actions were reported without changing the environment.'
}
else {
    Write-Output 'Development prerequisites, Git hooks, and VS Code extensions are installed.'
}

Write-Output 'VS Code activates extensions when their language and workspace conditions apply.'
