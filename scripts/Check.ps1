[CmdletBinding()]
param(
    [ValidateSet("debug", "releasedbg")]
    [string]$Mode = "releasedbg",

    [switch]$Strict,

    [switch]$IncludeAnalyze,

    [switch]$IncludeClangTidy,

    [switch]$IncludeTests,

    [switch]$Quiet
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($Strict) {
    $IncludeAnalyze = $true
    $IncludeClangTidy = $true
    $IncludeTests = $true
}

$formatScript = Join-Path $PSScriptRoot "Format.ps1"
$tidyScript = Join-Path $PSScriptRoot "Tidy.ps1"
$buildScript = Join-Path $PSScriptRoot "Build.ps1"
$testScript = Join-Path $PSScriptRoot "Test.ps1"

& $formatScript -Check -Quiet:$Quiet
& $buildScript -Mode $Mode -Analyze:$IncludeAnalyze -IncludeTests:$IncludeTests -Quiet:$Quiet

if ($IncludeClangTidy) {
    & $tidyScript -Mode $Mode -Quiet:$Quiet
}

if ($IncludeTests) {
    & $testScript -Mode $Mode -Quiet:$Quiet
}
