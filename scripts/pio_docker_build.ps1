param(
  [string[]]$Env = @("ttgo-t8-v171", "beetle-c3-location"),
  [string]$Image = "iotglove-builder",
  [string]$CacheVolume = "iotglove-platformio-cache",
  [switch]$BuildImage
)

$ErrorActionPreference = "Stop"

$repo = (Get-Location).Path

if ($BuildImage) {
  docker build -t $Image .
}

$pioCommand = "pio run"
foreach ($name in $Env) {
  $pioCommand += " -e $name"
}

$innerLines = @(
  'set -eu',
  'git config --global --add safe.directory /work',
  'if [ -n "${HAS2_LIB_TOKEN:-}" ]; then',
  '  git config --global url."https://x-access-token:${HAS2_LIB_TOKEN}@github.com/Fuzzyline-HAS2/".insteadOf "https://github.com/Fuzzyline-HAS2/"',
  'fi',
  $pioCommand
)
$inner = $innerLines -join "`n"

$dockerArgs = @(
  "run",
  "--rm",
  "-v",
  "${repo}:/work",
  "-v",
  "${CacheVolume}:/opt/platformio"
)

if ($env:HAS2_LIB_TOKEN) {
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
& docker @dockerArgs
