<#
.SYNOPSIS
    Writes PLL register values from a TICS Pro hex export to the device over UART.

.DESCRIPTION
    Reads a register list exported by TICS Pro ("R<n><TAB>0x<value>" per line, e.g.
    HexRegisterValues.txt) and sends each register to the device one by one, in file
    order, using the firmware's "wr" CLI command (see MX_USART_Init / MainTask.cpp).

.PARAMETER Port
    COM port name, e.g. COM5. If omitted, the script lists available ports and exits.

.PARAMETER Path
    Path to the hex values file. Defaults to HexRegisterValues.txt next to this script.

.PARAMETER BaudRate
    Serial baud rate. Must match MX_USART_Init (currently 9600).

.EXAMPLE
    .\WriteRegisters.ps1 -Port COM5
    .\WriteRegisters.ps1 -Port COM5 -Path .\MyRegisters.txt
#>
param(
    [string]$Port,
    [string]$Path = (Join-Path $PSScriptRoot "HexRegisterValues.txt"),
    [int]$BaudRate = 9600,
    [int]$TimeoutMs = 2000
)

if (-not $Port) {
    Write-Host "Usage: .\WriteRegisters.ps1 -Port <COMx> [-Path <file>] [-BaudRate 9600]"
    Write-Host "Available ports:"
    [System.IO.Ports.SerialPort]::GetPortNames() | ForEach-Object { Write-Host "  $_" }
    exit 1
}

if (-not (Test-Path $Path)) {
    Write-Error "File not found: $Path"
    exit 1
}

function Read-Response {
    param([System.IO.Ports.SerialPort]$SerialPort)

    $sb = [System.Text.StringBuilder]::new()
    $deadline = (Get-Date).AddMilliseconds($SerialPort.ReadTimeout)
    while ((Get-Date) -lt $deadline) {
        if ($SerialPort.BytesToRead -gt 0) {
            [void]$sb.Append($SerialPort.ReadExisting())
            if ($sb.ToString() -match '>\s*$') {
                break
            }
            $deadline = (Get-Date).AddMilliseconds($SerialPort.ReadTimeout)
        } else {
            Start-Sleep -Milliseconds 10
        }
    }
    return $sb.ToString()
}

$portName = $Port
if ($Port -match '^COM(\d+)$' -and [int]$Matches[1] -ge 10) {
    $portName = "\\.\$Port"
}

$serial = New-Object System.IO.Ports.SerialPort $portName, $BaudRate, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
$serial.NewLine = "`n"
$serial.ReadTimeout = $TimeoutMs
$serial.WriteTimeout = $TimeoutMs

try {
    $serial.Open()
} catch {
    Write-Error "Failed to open $Port : $_"
    exit 1
}

try {
    # Drain the startup banner / stale prompt, if any.
    Start-Sleep -Milliseconds 200
    if ($serial.BytesToRead -gt 0) { [void]$serial.ReadExisting() }

    $count = 0
    $errors = 0
    foreach ($line in Get-Content -Path $Path) {
        $trimmed = $line.Trim()
        if ([string]::IsNullOrWhiteSpace($trimmed)) { continue }

        $parts = $trimmed -split '\s+'
        if ($parts.Count -lt 2) {
            Write-Warning "Skipping malformed line: '$line'"
            continue
        }

        $register = $parts[0]
        $value = $parts[1]
        $command = "wr $register $value"

        $serial.WriteLine($command)
        $response = Read-Response -SerialPort $serial
        $count++

        if ($response -match 'Invalid') {
            Write-Host "-> $command  FAILED: $($response.Trim())" -ForegroundColor Red
            $errors++
        } else {
            $reply = ($response -replace "wr $register $value", '') -replace '[\r\n>]+', ' '
            Write-Host "-> $command  $($reply.Trim())"
        }
    }

    Write-Host "Done. Wrote $count register(s), $errors error(s)."

    Write-Host "-> enable"
    $serial.WriteLine("enable")
    $enableResponse = Read-Response -SerialPort $serial
    Write-Host "  $(($enableResponse -replace '[\r\n>]+', ' ').Trim())"

    $lockState = "UNKNOWN"
    $deadline = (Get-Date).AddSeconds(5)
    while ((Get-Date) -lt $deadline) {
        $serial.WriteLine("lock")
        $lockResponse = Read-Response -SerialPort $serial
        if ($lockResponse -match 'LockDetect:\s*(\w+)') {
            $lockState = $Matches[1]
            if ($lockState -eq 'LOCKED') { break }
        }
        Start-Sleep -Milliseconds 200
    }

    if ($lockState -eq 'LOCKED') {
        Write-Host "Lock state: LOCKED" -ForegroundColor Green
    } else {
        Write-Host "Lock state: $lockState (no LOCKED within 5s)" -ForegroundColor Yellow
    }
} finally {
    $serial.Close()
}
