<#
.SYNOPSIS
  Runs the SOCKETS_LOW_LATENCY sideband latency test and optionally captures a WPR (ETW) CPU trace.
.EXAMPLE
  .\scripts\profile_sideband_latency.ps1                 # just measure
  .\scripts\profile_sideband_latency.ps1 -Trace          # measure + WPR CPU trace (run elevated)
  .\scripts\profile_sideband_latency.ps1 -Target 10.198.65.94   # use an already-running remote server
#>
param(
    [string]$BinDir,
    [string]$Config = (Join-Path $PSScriptRoot '..\test_config_sideband_latency.json'),
    [string]$OutDir = (Join-Path $PSScriptRoot '..\build\profile'),
    [string]$Target,
    [switch]$Trace
)
$ErrorActionPreference = 'Stop'

if (-not $BinDir) {
    $BinDir = @('..\build\Release', '..\build\bin\Release') | ForEach-Object { Join-Path $PSScriptRoot $_ } |
        Where-Object { Test-Path (Join-Path $_ 'perftest_client.exe') } | Select-Object -First 1
    if (-not $BinDir) { throw 'perftest_client.exe not found under build\Release; build Release or pass -BinDir.' }
}
$BinDir = (Resolve-Path $BinDir).Path
$Config = (Resolve-Path $Config).Path
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$OutDir = (Resolve-Path $OutDir).Path

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Warning 'Not elevated: processes fall back from REALTIME to HIGH priority class, and WPR cannot run.'
    if ($Trace) { throw '-Trace requires an elevated PowerShell.' }
}

function Wait-ServerLog([string]$Path, [string]$Pattern, [int]$TimeoutSec = 30) {
    # Don't probe the sideband port: grpc-sideband treats any connection as a client and hangs on early close.
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        if ((Test-Path $Path) -and (Select-String -Path $Path -Pattern $Pattern -Quiet)) { return }
        Start-Sleep -Milliseconds 200
    }
    throw "Timed out waiting for '$Pattern' in $Path"
}

$serverLog = Join-Path $OutDir 'server.log'
$server = $null
if (-not $Target) {
    $server = Start-Process -FilePath (Join-Path $BinDir 'perftest_server.exe') -WorkingDirectory $OutDir `
        -RedirectStandardOutput $serverLog -RedirectStandardError (Join-Path $OutDir 'server.err.log') `
        -PassThru -WindowStyle Hidden
}
try {
    if ($server) { Wait-ServerLog $serverLog 'Listening for sideband sockets' }
    if ($Trace) {
        wpr.exe -cancel 2>$null | Out-Null
        wpr.exe -start CPU -start Network -filemode
    }

    Push-Location $OutDir
    try {
        $clientArgs = @('--config', $Config)
        if ($Target) { $clientArgs += @('--target', $Target) }
        & (Join-Path $BinDir 'perftest_client.exe') @clientArgs
    }
    finally {
        Pop-Location
    }

    if ($Trace) {
        $etl = Join-Path $OutDir 'sideband_latency.etl'
        wpr.exe -stop $etl
        Write-Host "Trace: $etl  (open in Windows Performance Analyzer; symbol path: $BinDir)"
    }
}
finally {
    if ($server) { Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue }
}

Write-Host "Per-iteration latencies (us): $(Join-Path $OutDir 'SidebandLatency.txt')"
