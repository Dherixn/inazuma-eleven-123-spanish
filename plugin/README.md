# Plugin de la edición definitiva

Plugin 3GX (CTRPluginFramework) que corre junto a *Inazuma Eleven 1·2·3* (`00040000000BB800`) y
añade lo que los datos solos no pueden: un menú de opciones dentro del juego y el traspaso de partida
entre juegos. Ver [`docs/definitiva/README.md`](../docs/definitiva/README.md).

## Qué hace hoy (v0.1)

- **Partidas → Ver archivos de partida**: lista los archivos del save archive del juego.
- **Partidas → Copiar partidas a la SD**: los copia a `/Inazuma123Definitiva/partidas/<fecha>/`.

Todavía no escribe en la memoria del juego. Las ayudas (#7) y el traspaso (#2) se activarán cuando sus
direcciones estén comprobadas con la ROM.

## Núcleo del traspaso

`Sources/Nucleo/` no depende de la 3DS: es el gemelo en C++ de `ie123kit.definitiva.partida` y
`.traspaso`, y se compila también en el PC. `Equivalencias.inc` se genera con
`python -m ie123kit.definitiva cabecera …` (hoy desde las ROM NDS; se regenerará con el 3DS, #3).

`python plugin/pruebas/comparar.py` genera partidas sintéticas de los tres juegos, hace el traspaso con
los dos y exige el mismo informe y los mismos bytes. Necesita la imagen Docker de `compilar.ps1`.

## Compilar

Solo hace falta Docker:

```powershell
pwsh ./plugin/compilar.ps1          # la primera vez construye la imagen definitiva-3gx
pwsh ./plugin/compilar.ps1 -Limpiar
```

La imagen parte de `devkitpro/devkitarm` y añade `libctrpf` y `3gxtool` desde el repositorio de paquetes
de ThePixellizerOSS. Resultado: `plugin/Definitiva-release.3gx` (git lo ignora).

Notas de la compilación:

- El paquete `libctrpf` no trae el `types.h` que sus cabeceras incluyen, y el de la libctru actual ya no
  define `NORETURN`. Por eso `Includes/types.h` es copia del de CTRPluginFramework.
- `Makefile` y `3gx.ld` vienen del TestPlugin de CTRPluginFramework. Su licencia está en
  `LICENSE-CTRPF.txt` y obliga a publicar el código junto a cualquier binario que se reparta.
- `Compatibility: Any` en `Definitiva.plgInfo`: con `Console`, Azahar lo rechaza, y con `Citra`, Luma.

## Instalar

| Dónde | Ruta del `.3gx` | Activar |
|---|---|---|
| 3DS con Luma3DS ≥ 13 | `SD:/luma/plugins/00040000000BB800/Definitiva.3gx` | Rosalina (L + ↓ + Select) → «Plugin Loader: Enabled» |
| Azahar | `<usuario>/sdmc/luma/plugins/00040000000BB800/Definitiva.3gx` | Configuración → Sistema → «Enable 3GX plugin loader» |

El menú del plugin se abre con **Select** durante el juego.
