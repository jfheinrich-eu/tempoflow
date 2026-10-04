function Invoke-VstValidator {
    [CmdletBinding()]
    param(
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]] $ArgumentList
    )

    if ($null -eq $ArgumentList -or $ArgumentList.Count -eq 0) {
        throw 'Provide the VST3 bundle path and any required validator arguments.'
    }

    $validatorPath = $env:TEMPOFLOW_VST3_VALIDATOR

    if ([string]::IsNullOrWhiteSpace($validatorPath)) {
        $validatorRoot = $env:TEMPOFLOW_VST3_VALIDATOR_ROOT

        if ([string]::IsNullOrWhiteSpace($validatorRoot)) {
            $validatorRoot = 'C:\Tools\Steinberg\vst3sdk\build'
        }

        if (-not (Test-Path -LiteralPath $validatorRoot -PathType Container)) {
            throw "Validator search directory does not exist: $validatorRoot"
        }

        $validatorCandidates = @(
            Get-ChildItem -LiteralPath $validatorRoot `
                -Filter validator.exe `
                -File `
                -Recurse `
                -ErrorAction Stop
        )

        if ($validatorCandidates.Count -eq 0) {
            throw "validator.exe was not found under: $validatorRoot"
        }

        if ($validatorCandidates.Count -gt 1) {
            $paths = $validatorCandidates.FullName -join [Environment]::NewLine
            throw "Multiple validator.exe files were found:`n$paths"
        }

        $validatorPath = $validatorCandidates[0].FullName
    }

    if (-not (Test-Path -LiteralPath $validatorPath -PathType Leaf)) {
        throw "Validator executable does not exist: $validatorPath"
    }

    $resolvedValidatorPath = (Resolve-Path -LiteralPath $validatorPath -ErrorAction Stop).Path
    & $resolvedValidatorPath @ArgumentList
    $exitCode = $LASTEXITCODE

    if ($exitCode -ne 0) {
        throw "validator.exe failed with exit code $exitCode."
    }

    return $exitCode
}
