Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:TempoFlowSourceExtensions = @('.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx')
$script:TempoFlowExcludedPathPrefixes = @(
    'build/',
    'external/',
    'extern/',
    'third_party/',
    'vendor/',
    'juce/',
    'vst3sdk/'
)
$script:TempoFlowJsonStyle = 'file'
$script:TempoFlowPresetStyle = '{BasedOnStyle: LLVM, Language: Json, ColumnLimit: 120, IndentWidth: 4, TabWidth: 4, UseTab: Never, BreakArrays: true, LineEnding: LF}'

function Get-TempoFlowRepositoryRoot {
    $repositoryRoot = & git -C $PSScriptRoot rev-parse --show-toplevel
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($repositoryRoot)) {
        throw 'Failed to locate the TempoFlow repository root.'
    }

    return [IO.Path]::GetFullPath($repositoryRoot.Trim())
}

function Test-TempoFlowExcludedPath {
    param(
        [Parameter(Mandatory)]
        [string] $Path
    )

    $normalizedPath = $Path.Replace('\', '/').TrimStart('/')
    foreach ($prefix in $script:TempoFlowExcludedPathPrefixes) {
        if ($normalizedPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }

    return $false
}

function Get-TempoFlowFormatFiles {
    param(
        [Parameter(Mandatory)]
        [string] $RepositoryRoot
    )

    $trackedFiles = @(& git -c core.quotepath=false -C $RepositoryRoot ls-files --cached)
    if ($LASTEXITCODE -ne 0) {
        throw 'Failed to enumerate tracked repository files.'
    }

    $sourceFiles = [Collections.Generic.List[string]]::new()
    $jsonFiles = [Collections.Generic.List[string]]::new()
    $presetFiles = [Collections.Generic.List[string]]::new()

    foreach ($relativePath in $trackedFiles) {
        if ([string]::IsNullOrWhiteSpace($relativePath) -or (Test-TempoFlowExcludedPath -Path $relativePath)) {
            continue
        }

        $extension = [IO.Path]::GetExtension($relativePath).ToLowerInvariant()
        $absolutePath = [IO.Path]::GetFullPath((Join-Path $RepositoryRoot $relativePath))

        if ($script:TempoFlowSourceExtensions -contains $extension) {
            $sourceFiles.Add($absolutePath)
        }
        elseif ($extension -eq '.json') {
            $jsonFiles.Add($absolutePath)
        }
        elseif ($extension -eq '.tempoflow') {
            $presetFiles.Add($absolutePath)
        }
    }

    return [PSCustomObject]@{
        SourceFiles = @($sourceFiles)
        JsonFiles   = @($jsonFiles)
        PresetFiles = @($presetFiles)
    }
}

function Get-TempoFlowFormatter {
    $formatter = Get-Command clang-format -CommandType Application -ErrorAction SilentlyContinue
    if ($null -eq $formatter) {
        throw 'clang-format was not found. Run this command from Developer PowerShell for VS 18.'
    }

    return $formatter.Source
}

function Get-TempoFlowRelativePath {
    param(
        [Parameter(Mandatory)]
        [string] $RepositoryRoot,

        [Parameter(Mandatory)]
        [string] $Path
    )

    $rootUri = [Uri]::new($RepositoryRoot.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar)
    $pathUri = [Uri]::new($Path)
    return [Uri]::UnescapeDataString($rootUri.MakeRelativeUri($pathUri).ToString()).Replace('/', '\')
}

function Assert-TempoFlowJsonSyntax {
    param(
        [Parameter(Mandatory)]
        [string] $RepositoryRoot,

        [Parameter(Mandatory)]
        [string] $Path,

        [Parameter(Mandatory)]
        [string] $Kind
    )

    $relativePath = Get-TempoFlowRelativePath -RepositoryRoot $RepositoryRoot -Path $Path
    try {
        Get-Content -Raw -LiteralPath $Path -Encoding UTF8 | ConvertFrom-Json | Out-Null
    }
    catch {
        throw "$Kind syntax validation failed for '$relativePath': $($_.Exception.Message)"
    }
}

function Assert-TempoFlowTextConvention {
    param(
        [Parameter(Mandatory)]
        [string] $RepositoryRoot,

        [Parameter(Mandatory)]
        [string] $Path,

        [Parameter(Mandatory)]
        [string] $Kind
    )

    $relativePath = Get-TempoFlowRelativePath -RepositoryRoot $RepositoryRoot -Path $Path
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        throw "$Kind encoding validation failed for '$relativePath': UTF-8 BOM is not allowed."
    }

    try {
        $strictUtf8 = [Text.UTF8Encoding]::new($false, $true)
        $text = $strictUtf8.GetString($bytes)
    }
    catch {
        throw "$Kind encoding validation failed for '$relativePath': the file is not valid UTF-8."
    }

    if ($text.Contains("`r")) {
        throw "$Kind line-ending validation failed for '$relativePath': only LF is allowed."
    }

    if (-not $text.EndsWith("`n", [StringComparison]::Ordinal)) {
        throw "$Kind final-newline validation failed for '$relativePath'."
    }
}

function Invoke-TempoFlowClangFormat {
    param(
        [Parameter(Mandatory)]
        [string] $Formatter,

        [Parameter(Mandatory)]
        [string] $RepositoryRoot,

        [Parameter(Mandatory)]
        [string] $Path,

        [Parameter(Mandatory)]
        [ValidateSet('Source', 'Json', 'Preset')]
        [string] $Kind,

        [Parameter(Mandatory)]
        [ValidateSet('Format', 'Verify')]
        [string] $Mode
    )

    $relativePath = Get-TempoFlowRelativePath -RepositoryRoot $RepositoryRoot -Path $Path
    $formatPath = $Path
    $temporaryPath = $null

    try {
        if ($Kind -eq 'Preset') {
            $temporaryPath = Join-Path ([IO.Path]::GetTempPath()) ("tempoflow-format-{0}.json" -f [Guid]::NewGuid())
            [IO.File]::WriteAllBytes($temporaryPath, [IO.File]::ReadAllBytes($Path))
            $formatPath = $temporaryPath
        }

        $arguments = [Collections.Generic.List[string]]::new()
        if ($Mode -eq 'Format') {
            $arguments.Add('-i')
        }
        else {
            $arguments.Add('--dry-run')
            $arguments.Add('--Werror')
        }

        $arguments.Add('--fallback-style=none')
        if ($Kind -eq 'Preset') {
            $arguments.Add("--style=$script:TempoFlowPresetStyle")
        }
        else {
            $arguments.Add("--style=$script:TempoFlowJsonStyle")
        }

        $arguments.Add('--')
        $arguments.Add($formatPath)

        & $Formatter @arguments
        if ($LASTEXITCODE -ne 0) {
            $action = if ($Mode -eq 'Format') { 'formatting' } else { 'format verification' }
            throw "clang-format $action failed for '$relativePath'."
        }

        if ($Mode -eq 'Format' -and $Kind -eq 'Preset') {
            [IO.File]::WriteAllBytes($Path, [IO.File]::ReadAllBytes($temporaryPath))
        }
    }
    finally {
        if ($null -ne $temporaryPath -and [IO.File]::Exists($temporaryPath)) {
            [IO.File]::Delete($temporaryPath)
        }
    }
}

function Invoke-TempoFlowDiffCheck {
    param(
        [Parameter(Mandatory)]
        [string] $RepositoryRoot
    )

    & git -C $RepositoryRoot diff --check
    if ($LASTEXITCODE -ne 0) {
        throw 'git diff --check failed.'
    }

    & git -C $RepositoryRoot diff --cached --check
    if ($LASTEXITCODE -ne 0) {
        throw 'git diff --cached --check failed.'
    }
}
