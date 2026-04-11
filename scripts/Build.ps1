[CmdletBinding()]
param(
    [ValidateSet("debug", "releasedbg")]
    [string]$Mode = "debug",

    [switch]$IncludeTests,

    [switch]$ConfigureOnly,

    [switch]$Reconfigure,

    [switch]$Quiet
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$enterDevShell = Join-Path $PSScriptRoot "Enter-DevShell.ps1"

function Resolve-CommonLibRoot {
    return Join-Path $repoRoot "lib/commonlibsse-ng"
}

function Get-CommonLibSetupCommand {
    return "git submodule update --init --recursive"
}

function Assert-CommonLibReady {
    $commonLibRoot = Resolve-CommonLibRoot
    $setupCommand = Get-CommonLibSetupCommand
    if (-not (Test-Path -LiteralPath $commonLibRoot)) {
        throw "CommonLibSSE-NG is missing at '$commonLibRoot'. This repo expects that directory to be populated by Git submodules. Run '$setupCommand' from the repo root."
    }

    $commonLibXmake = Join-Path $commonLibRoot "xmake.lua"
    if (-not (Test-Path -LiteralPath $commonLibXmake)) {
        throw "CommonLibSSE-NG at '$commonLibRoot' is incomplete. Expected xmake project at '$commonLibXmake'. Re-run '$setupCommand' from the repo root."
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

Push-Location $repoRoot
try {
    if (-not $Quiet) {
        Write-Host "Configuring xmake:" -ForegroundColor Cyan
        Write-Host "  mode:        $Mode"
        if ($sdkVersion) {
            Write-Host "  sdk:         $sdkVersion"
        }
        Write-Host "  tests:       $($IncludeTests.IsPresent)"
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
