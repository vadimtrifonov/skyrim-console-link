[CmdletBinding()]
param(
    [ValidateSet("debug", "releasedbg")]
    [string]$Mode = "debug",

    [switch]$Analyze,

    [switch]$ConfigureOnly,

    [switch]$Reconfigure,

    [switch]$Quiet
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$enterDevShell = Join-Path $PSScriptRoot "Enter-DevShell.ps1"

function Resolve-CommonLibRoot
{
    return Join-Path $repoRoot "lib/commonlibsse-ng"
}

function Get-CommonLibSetupCommand
{
    return "git submodule update --init --recursive"
}

function Get-ExpectedCommonLibCommit
{
    $entry = (& git -C $repoRoot ls-files --stage -- lib/commonlibsse-ng | Select-Object -First 1)
    if ($entry) {
        $parts = $entry -split '\s+'
        if ($parts.Count -ge 2) {
            return $parts[1]
        }

        throw "Unexpected ls-files output for lib/commonlibsse-ng: $entry"
    }

    $entry = & git -C $repoRoot ls-tree HEAD lib/commonlibsse-ng
    if (-not $entry) {
        throw "Could not read the pinned CommonLibSSE-NG submodule entry from the index or HEAD."
    }

    $parts = $entry -split '\s+'
    if ($parts.Count -lt 3) {
        throw "Unexpected ls-tree output for lib/commonlibsse-ng: $entry"
    }

    return $parts[2]
}

function Assert-CommonLibReady
{
    $commonLibRoot = Resolve-CommonLibRoot
    $setupCommand = Get-CommonLibSetupCommand
    if (-not (Test-Path -LiteralPath $commonLibRoot)) {
        throw "CommonLibSSE-NG is missing at '$commonLibRoot'. This repo expects that directory to be populated by Git submodules. Run '$setupCommand' from the repo root."
    }

    $commonLibXmake = Join-Path $commonLibRoot "xmake.lua"
    if (-not (Test-Path -LiteralPath $commonLibXmake)) {
        throw "CommonLibSSE-NG at '$commonLibRoot' is incomplete. Expected xmake project at '$commonLibXmake'. Re-run '$setupCommand' from the repo root."
    }

    $gitDir = Join-Path $commonLibRoot ".git"
    if (-not (Test-Path -LiteralPath $gitDir)) {
        throw "CommonLibSSE-NG at '$commonLibRoot' is not initialized as a submodule. Run '$setupCommand' from the repo root."
    }

    $expectedCommit = Get-ExpectedCommonLibCommit
    $actualCommit = (& git -C $commonLibRoot rev-parse HEAD).Trim()
    if ($actualCommit -ne $expectedCommit) {
        throw "CommonLibSSE-NG at '$commonLibRoot' is at $actualCommit but the repo pins $expectedCommit. Sync the submodule before building."
    }

    return $commonLibRoot
}

if (-not (Get-Command xmake -ErrorAction SilentlyContinue) -or -not (Get-Command cl -ErrorAction SilentlyContinue)) {
    & $enterDevShell -Quiet
}

if (-not (Get-Command xmake -ErrorAction SilentlyContinue)) {
    throw "xmake is not available on PATH. Install it and reopen the dev shell."
}
if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    throw "cl.exe is not available on PATH. Activate the Visual Studio dev shell first."
}

$commonLibRoot = Assert-CommonLibReady

$sdkVersion = $null
if ($env:WindowsSDKVersion) {
    $sdkVersion = $env:WindowsSDKVersion.TrimEnd("\")
}

$configureArgs = @("f", "-y", "-m", $Mode)
if ($Reconfigure) {
    $configureArgs += "-c"
}
if ($sdkVersion) {
    $configureArgs += "--vs_sdkver=$sdkVersion"
}
$configureArgs += if ($Analyze) { "--msvc_analyze=y" } else { "--msvc_analyze=n" }

Push-Location $repoRoot
try {
    if (-not $Quiet) {
        Write-Host "Configuring xmake:" -ForegroundColor Cyan
        Write-Host "  mode:        $Mode"
        if ($sdkVersion) {
            Write-Host "  sdk:         $sdkVersion"
        }
        Write-Host "  analyze:     $($Analyze.IsPresent)"
        Write-Host "  reconfigure: $($Reconfigure.IsPresent)"
        Write-Host "  commonlib:   $commonLibRoot"
        Write-Host ""
    }

    & xmake @configureArgs
    if ($LASTEXITCODE -ne 0) {
        throw "xmake configure failed."
    }

    & xmake project -k compile_commands
    if ($LASTEXITCODE -ne 0) {
        throw "xmake project -k compile_commands failed."
    }

    if (-not $ConfigureOnly) {
        & xmake -y
        if ($LASTEXITCODE -ne 0) {
            throw "xmake build failed."
        }
    }
} finally {
    Pop-Location
}
