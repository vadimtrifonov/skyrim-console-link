[CmdletBinding()]
param(
    [ValidateSet("debug", "releasedbg")]
    [string]$Mode = "debug",

    [string[]]$Path = @("src"),

    [switch]$Fix,

    [switch]$Quiet
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildScript = Join-Path $PSScriptRoot "Build.ps1"
$enterDevShell = Join-Path $PSScriptRoot "Enter-DevShell.ps1"

if (-not (Get-Command clang-tidy -ErrorAction SilentlyContinue)) {
    & $enterDevShell -Quiet
}

& $buildScript -Mode $Mode -ConfigureOnly -Quiet

$files = New-Object System.Collections.Generic.List[string]

Push-Location $repoRoot
try {
    foreach ($relativePath in $Path) {
        $targetPath = if ([System.IO.Path]::IsPathRooted($relativePath)) {
            $relativePath
        } else {
            Join-Path $repoRoot $relativePath
        }

        if (-not (Test-Path -LiteralPath $targetPath)) {
            continue
        }

        Get-ChildItem -LiteralPath $targetPath -Recurse -File -Include *.cpp, *.cc, *.cxx |
            Sort-Object FullName |
            ForEach-Object { [void]$files.Add($_.FullName) }
    }

    if ($files.Count -eq 0) {
        if (-not $Quiet) {
            Write-Host "No translation units matched the requested paths." -ForegroundColor Yellow
        }
        return
    }

    if (-not $Quiet) {
        Write-Host "Running clang-tidy" -ForegroundColor Cyan
        Write-Host "  mode:  $Mode"
        Write-Host "  files: $($files.Count)"
        Write-Host ""
    }

    $args = @(
        "-p", $repoRoot,
        "--header-filter=.*[\\\\/]src[\\\\/].*",
        "--extra-arg-before=--driver-mode=cl",
        "--extra-arg=-Wno-unused-command-line-argument"
    )
    if ($Fix) {
        $args += "-fix"
        $args += "-format-style=file"
    }

    $output = & clang-tidy @args @($files.ToArray()) 2>&1
    if ($output) {
        $output
    }

    if ($LASTEXITCODE -ne 0) {
        throw "clang-tidy failed."
    }
} finally {
    Pop-Location
}
