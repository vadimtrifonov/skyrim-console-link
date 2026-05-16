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
$submodulePath = "lib/commonlibvr"
$submoduleRoot = Join-Path $repoRoot $submodulePath

function Get-ExpectedCommonLibCommit
{
    $entry = (& git -C $repoRoot ls-files --stage -- $submodulePath | Select-Object -First 1)
    if ($entry) {
        $parts = $entry -split '\s+'
        if ($parts.Count -ge 2) {
            return $parts[1]
        }

        throw "Unexpected ls-files output for ${submodulePath}: $entry"
    }

    $entry = & git -C $repoRoot ls-tree HEAD $submodulePath
    if (-not $entry) {
        throw "Could not read the pinned CommonLibVR submodule entry from the index or HEAD."
    }

    $parts = $entry -split '\s+'
    if ($parts.Count -lt 3) {
        throw "Unexpected ls-tree output for ${submodulePath}: $entry"
    }

    return $parts[2]
}

function Assert-CommonLibReady
{
    if (-not (Test-Path -LiteralPath $submoduleRoot)) {
        throw "CommonLibVR is missing at '$submoduleRoot'. Run 'git submodule update --init --recursive' from the repo root."
    }

    $commonLibXmake = Join-Path $submoduleRoot "xmake.lua"
    if (-not (Test-Path -LiteralPath $commonLibXmake)) {
        throw "CommonLibVR at '$submoduleRoot' is incomplete. Expected xmake project at '$commonLibXmake'."
    }

    $gitDir = Join-Path $submoduleRoot ".git"
    if (-not (Test-Path -LiteralPath $gitDir)) {
        throw "CommonLibVR at '$submoduleRoot' is not initialized as a submodule. Run 'git submodule update --init --recursive' from the repo root."
    }

    $expectedCommit = Get-ExpectedCommonLibCommit
    $actualCommit = (& git -C $submoduleRoot rev-parse HEAD).Trim()
    if ($actualCommit -ne $expectedCommit) {
        throw "CommonLibVR at '$submoduleRoot' is at $actualCommit but the repo pins $expectedCommit. Sync the submodule before building."
    }

    return $submoduleRoot
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

$configureArgs = @("f", "-y", "-p", "windows", "-a", "x64", "-m", $Mode)
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
