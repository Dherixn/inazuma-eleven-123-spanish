<#
Instala la edición definitiva en una consola Android (AYN Thor) con Azahar, por adb.

    powershell -ExecutionPolicy Bypass -File tools/definitiva_thor.ps1 [-Destino /storage/emulated/0/Azahar]

Copia «load» y «sdmc» del paquete (python -m ie123kit.definitiva paquete ...) dentro de la carpeta de
usuario de Azahar del dispositivo, fusionando con lo que haya. No toca ROMs ni partidas. Antes, en la
consola: depuración USB activada y la carpeta de usuario de Azahar ya elegida (se elige al abrirlo).
#>
param(
    [string]$Paquete = "work/shared/definitiva/Inazuma123Definitiva",
    [string]$Destino = "/storage/emulated/0/Azahar",
    [string]$Adb = "$env:LOCALAPPDATA\Android\Sdk\platform-tools\adb.exe"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $Adb)) { throw "No encuentro adb en $Adb (usa -Adb)." }
foreach ($sub in "load", "sdmc") {
    if (-not (Test-Path (Join-Path $Paquete $sub))) { throw "Al paquete le falta «$sub»: monta antes el paquete." }
}

$dispositivos = & $Adb devices | Select-Object -Skip 1 | Where-Object { $_ -match "\tdevice$" }
if (-not $dispositivos) { throw "No hay ningún dispositivo conectado por adb (¿depuración USB activada?)." }

$hay = & $Adb shell "[ -d '$Destino' ] && echo si"
if ($hay -ne "si") {
    Write-Host "No existe $Destino en el dispositivo. Carpetas candidatas:"
    & $Adb shell "ls -d /storage/emulated/0/*zahar* /storage/emulated/0/*itra* /storage/emulated/0/Android/data/*zahar* /storage/emulated/0/Android/data/*citra* 2>/dev/null"
    throw "Indica la carpeta de usuario de Azahar con -Destino."
}

foreach ($sub in "load", "sdmc") {
    Write-Host "Copiando $sub a $Destino ..."
    & $Adb push (Join-Path $Paquete $sub) "$Destino/"
    if ($LASTEXITCODE -ne 0) { throw "Falló la copia de $sub." }
}

$plugin = & $Adb shell "ls '$Destino/sdmc/luma/plugins/00040000000BB800/Definitiva.3gx' 2>/dev/null"
$fa = & $Adb shell "ls -l '$Destino/load/mods/00040000000BB800/romfs/archive.fa' 2>/dev/null"
Write-Host ""
Write-Host "Plugin: $plugin"
Write-Host "Mod:    $fa"
Write-Host ""
Write-Host "Último paso, en la consola: Azahar → Ajustes → Sistema → «3GX Plugin Loader» activado."
Write-Host "En el menú del recopilatorio, Select abre el menú de la edición definitiva."
