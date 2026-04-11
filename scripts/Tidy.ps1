[CmdletBinding()]
param(
    [ValidateSet("debug", "releasedbg")]
    [string]$Mode = "debug",

    [string[]]$Path = @("src", "tests"),

    [switch]$Fix,

    [switch]$Quiet
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildScript = Join-Path $PSScriptRoot "Build.ps1"
$enterDevShell = Join-Path $PSScriptRoot "Enter-DevShell.ps1"
$commonLibOverrides = @(
    @{
        SourcePath = Join-Path $repoRoot "lib/commonlibsse-ng/include/RE/B/BSTPointerAndFlags.h"
        RelativePath = "RE/B/BSTPointerAndFlags.h"
        KnownText = "a_rhs.storage.address = 0;"
        FixedText = "a_rhs._storage.address = 0;"
    },
    @{
        SourcePath = Join-Path $repoRoot "lib/commonlibsse-ng/include/RE/G/GHashSetBase.h"
        RelativePath = "RE/G/GHashSetBase.h"
        KnownText = "for (const_iterator it = a_src.Begin(); it != a_src.End(); ++it) {"
        FixedText = "for (const_iterator it = a_src.begin(); it != a_src.end(); ++it) {"
    }
)

function New-TemporaryOverrideIncludeRoot
{
    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("ConsoleLink-clang-tidy-" + [System.Guid]::NewGuid().ToString("N"))
    $overrideIncludeRoot = Join-Path $tempRoot "include"

    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    foreach ($override in $commonLibOverrides) {
        $sourcePath = $override.SourcePath
        if (-not (Test-Path -LiteralPath $sourcePath)) {
            throw "Missing vendored CommonLib header at '$sourcePath'."
        }

        $headerContent = Get-Content -LiteralPath $sourcePath -Raw
        if ($headerContent.Contains($override.KnownText)) {
            $headerContent = $headerContent.Replace($override.KnownText, $override.FixedText)
        } elseif (-not $headerContent.Contains($override.FixedText)) {
            throw "Unexpected CommonLib header contents in '$sourcePath'. Review the clang-tidy override logic."
        }

        $overrideHeaderPath = Join-Path $overrideIncludeRoot $override.RelativePath
        New-Item -ItemType Directory -Path (Split-Path -Parent $overrideHeaderPath) -Force | Out-Null
        [System.IO.File]::WriteAllText($overrideHeaderPath, $headerContent, $utf8NoBom)
    }

    return @{
        Root = $tempRoot
        IncludeRoot = $overrideIncludeRoot
    }
}

if (-not (Get-Command clang-tidy -ErrorAction SilentlyContinue)) {
    & $enterDevShell -Quiet
}

$temporaryOverride = New-TemporaryOverrideIncludeRoot
$overrideIncludeRoot = $temporaryOverride.IncludeRoot

$includeTests = $false
foreach ($relativePath in $Path) {
    if ($relativePath -match '(^|[\\/])tests($|[\\/])') {
        $includeTests = $true
        break
    }
}

& $buildScript -Mode $Mode -IncludeTests:$includeTests -ConfigureOnly -Quiet

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
        "--header-filter=.*[\\\\/](src|tests)[\\\\/].*",
        "--extra-arg-before=--driver-mode=cl",
        "--extra-arg-before=/I$overrideIncludeRoot",
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
    if ($temporaryOverride -and (Test-Path -LiteralPath $temporaryOverride.Root)) {
        Remove-Item -LiteralPath $temporaryOverride.Root -Recurse -Force
    }
}
