param(
  [string[]]$Env = @("ttgo-t8-v171", "beetle-c3-location"),
  [string]$Image = "iotglove-builder",
  [string]$CacheVolume = "iotglove-platformio-cache",
  [switch]$ForwardHas2Token,
  [switch]$BuildImage
)

$ErrorActionPreference = "Stop"

$repo = (Get-Location).Path
$Env = @($Env | ForEach-Object { $_ -split "," } | ForEach-Object { $_.Trim() } | Where-Object { $_ })

if ($BuildImage) {
  docker build -t $Image .
}

$pioCommand = "pio run"
foreach ($name in $Env) {
  $pioCommand += " -e $name"
}

$shouldForwardHas2Token = $ForwardHas2Token.IsPresent
if ($shouldForwardHas2Token -and -not $env:HAS2_LIB_TOKEN) {
  throw "HAS2_LIB_TOKEN is not set. Remove -ForwardHas2Token or set the environment variable before running this script."
}

$innerLines = @(
  'set -eu',
  'git config --global --add safe.directory /work'
)

if ($shouldForwardHas2Token) {
  $innerLines += 'git config --global url."https://x-access-token:${HAS2_LIB_TOKEN}@github.com/Fuzzyline-HAS2/".insteadOf "https://github.com/Fuzzyline-HAS2/"'
}

$innerLines += $pioCommand
$inner = $innerLines -join "`n"

$dockerArgs = @(
  "run",
  "--rm",
  "-v",
  "${repo}:/work",
  "-v",
  "${CacheVolume}:/opt/platformio"
)

if ($shouldForwardHas2Token) {
  $dockerArgs += @("-e", "HAS2_LIB_TOKEN")
}

$dockerArgs += @(
  $Image,
  "/bin/sh",
  "-lc",
  $inner
)

Write-Host "Using PlatformIO cache volume: $CacheVolume"
Write-Host "Building env(s): $($Env -join ', ')"
if ($shouldForwardHas2Token) {
  Write-Host "Forwarding HAS2_LIB_TOKEN into the container"
} else {
  Write-Host "HAS2_LIB_TOKEN is not forwarded. Use -ForwardHas2Token only when private dependency fetch is required."
}
& docker @dockerArgs
