<#
.SYNOPSIS
  Compila el plugin 3GX de la edición definitiva con Docker (sin instalar devkitPro).
.DESCRIPTION
  Construye la imagen definitiva-3gx (devkitARM + CTRPluginFramework + 3gxtool) la primera vez
  y compila plugin/ dentro de ella. Deja plugin/Definitiva-release.3gx.
.EXAMPLE
  pwsh ./plugin/compilar.ps1
  pwsh ./plugin/compilar.ps1 -Limpiar
#>
[CmdletBinding()]
param([switch]$Limpiar)
$ErrorActionPreference = "Stop"
$aqui = $PSScriptRoot
$imagen = "definitiva-3gx"

if (-not (docker images -q $imagen)) {
  Write-Host "==> Construyendo la imagen $imagen"
  docker build -t $imagen $aqui
  if ($LASTEXITCODE) { throw "docker build ha fallado" }
}
$orden = if ($Limpiar) { "make clean && make" } else { "make" }
docker run --rm -v "${aqui}:/plugin" $imagen bash -lc $orden
if ($LASTEXITCODE) { throw "la compilación ha fallado" }
Write-Host "OK: $aqui\Definitiva-release.3gx"
