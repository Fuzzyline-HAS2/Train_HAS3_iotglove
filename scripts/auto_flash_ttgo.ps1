[CmdletBinding()]
param(
  # Our TTGO T8 v1.7.1 boards ship a WCH CH9102 USB-UART bridge (VID 1A86 / PID 55D4).
  # Override for a CP2104 board (VID_10C4.*PID_EA60) or a CH340 (VID_1A86.*PID_7523).
  [string]$VidPid   = 'VID_1A86.*PID_55D4',
  [string]$Chip     = 'esp32',
  [int]$Baud        = 921600,
  # Defaults to the ttgo-t8-v171 factory image. Override to flash a different build.
  [string]$Firmware,
  [int]$PollMs      = 1500
)

# Native tools (py/esptool) write to stderr on normal "module missing" probes; under
# 'Stop' PowerShell turns that stderr into a terminating error and aborts before we can
# react. We drive control flow off $LASTEXITCODE instead, so keep this on 'Continue'.
$ErrorActionPreference = 'Continue'

# scripts/ -> repo root
$root = Split-Path -Parent $PSScriptRoot
if (-not $Firmware) {
  $Firmware = Join-Path $root 'dist\wired-upload\ttgo-t8-v171\ttgo-factory.bin'
}

# Prefer the py launcher, fall back to python on PATH.
$py = if (Get-Command py -ErrorAction SilentlyContinue) { 'py' } else { 'python' }

if (-not (Test-Path $Firmware)) {
  Write-Host "[ERROR] Firmware not found:" -ForegroundColor Red
  Write-Host "        $Firmware"
  Write-Host "Build it first, e.g.:"
  Write-Host "  powershell scripts\pio_docker_build.ps1 -Env ttgo-t8-v171"
  Write-Host "then ensure ttgo-factory.bin exists under dist\wired-upload\ttgo-t8-v171\."
  Read-Host 'Press Enter to exit'
  exit 1
}

# Make sure esptool is available; install on first run if missing.
& $py -m esptool version *> $null
if ($LASTEXITCODE -ne 0) {
  Write-Host '[setup] esptool not found - installing via pip...' -ForegroundColor Yellow
  & $py -m pip install --quiet --disable-pip-version-check "esptool>=5"
  if ($LASTEXITCODE -ne 0) {
    Write-Host '[ERROR] Failed to install esptool. Run:  ' -NoNewline -ForegroundColor Red
    Write-Host "$py -m pip install esptool"
    Read-Host 'Press Enter to exit'
    exit 1
  }
}

function Get-TtgoPort {
  $dev = Get-CimInstance Win32_PnPEntity |
    Where-Object { $_.DeviceID -match $VidPid -and $_.Name -match '\(COM\d+\)' } |
    Select-Object -First 1
  if ($dev) {
    return 'COM' + ([regex]'\(COM(\d+)\)').Match($dev.Name).Groups[1].Value
  }
  return $null
}

Write-Host "==================================================="
Write-Host " TTGO T8 v1.7.1 auto-flasher (updated_IoTglove)"
Write-Host "==================================================="
Write-Host " Watching for : $VidPid"
Write-Host " Firmware     : $Firmware"
Write-Host " Chip / Baud  : $Chip / $Baud"
Write-Host " Plug in a TTGO to flash it. Ctrl+C to stop."
Write-Host ""

$last = $null
while ($true) {
  $port = Get-TtgoPort
  if ($port) {
    if ($port -ne $last) {
      Write-Host "[detected] TTGO on $port - flashing..." -ForegroundColor Cyan
      & $py -m esptool --chip $Chip --port $port --baud $Baud --before default-reset --after hard-reset write-flash -z 0x0 "$Firmware"
      if ($LASTEXITCODE -eq 0) {
        Write-Host "[OK] $port flashed. Unplug this board, plug the next one.`n" -ForegroundColor Green
        $last = $port
      } else {
        Write-Host "[FAIL] flash failed on $port (exit $LASTEXITCODE). Will retry shortly.`n" -ForegroundColor Red
        $last = $null
        Start-Sleep -Seconds 2
      }
    }
  } else {
    # Board unplugged -> arm for the next insertion.
    $last = $null
  }
  Start-Sleep -Milliseconds $PollMs
}
