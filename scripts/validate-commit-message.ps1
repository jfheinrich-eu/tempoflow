[CmdletBinding(DefaultParameterSetName = 'File')]
param(
    [Parameter(Mandatory, ParameterSetName = 'File')]
    [ValidateNotNullOrEmpty()]
    [string] $MessageFile,

    [Parameter(Mandatory, ParameterSetName = 'Text')]
    [AllowEmptyString()]
    [string] $MessageText
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($PSCmdlet.ParameterSetName -eq 'File') {
    if (-not (Test-Path -LiteralPath $MessageFile -PathType Leaf)) {
        throw "Commit message file does not exist: $MessageFile"
    }

    $MessageText = Get-Content -LiteralPath $MessageFile -Raw
}

$normalizedMessage = $MessageText -replace "`r`n", "`n" -replace "`r", "`n"
$lines = @($normalizedMessage -split "`n")
$subject = $lines[0]
$errors = [System.Collections.Generic.List[string]]::new()

$subjectPattern = '^(?<type>feat|fix|docs|test|build|ci|refactor|perf|style|chore|revert)(\([a-z0-9][a-z0-9._/-]*\))?(?<breaking>!)?: (?<description>\S.*)$'
$subjectMatch = [regex]::Match($subject, $subjectPattern)

if (-not $subjectMatch.Success) {
    $errors.Add('The subject must follow Conventional Commits: <type>[optional scope][!]: <description>.')
}
elseif ($subject.EndsWith('.')) {
    $errors.Add('The subject must not end with a period.')
}

if ($lines.Count -gt 1 -and $lines[1].Length -ne 0) {
    $errors.Add('Separate the subject from the body and footers with a blank line.')
}

$signOffPattern = '(?m)^Signed-off-by: .+ <[^<>\s@]+@[^<>\s]+>$'
if (-not [regex]::IsMatch($normalizedMessage, $signOffPattern)) {
    $errors.Add('Add a valid Signed-off-by footer by committing with git commit -s.')
}

$breakingFooterPattern = '(?m)^BREAKING CHANGE: \S.*$'
$hasBreakingFooter = [regex]::IsMatch($normalizedMessage, $breakingFooterPattern)
$hasBreakingMarker = $subjectMatch.Success -and $subjectMatch.Groups['breaking'].Success

if ($hasBreakingMarker -and -not $hasBreakingFooter) {
    $errors.Add('A subject containing ! also requires a BREAKING CHANGE: footer.')
}
elseif ($hasBreakingFooter -and -not $hasBreakingMarker) {
    $errors.Add('A BREAKING CHANGE: footer also requires ! before the subject colon.')
}

if ($errors.Count -gt 0) {
    Write-Error ("Commit message policy failed:`n- " + ($errors -join "`n- "))
    exit 1
}

Write-Output "Commit message policy passed: $subject"
