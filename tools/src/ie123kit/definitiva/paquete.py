"""Paquete listo para copiar: la carpeta de usuario de Azahar (Windows, Android, AYN Thor) con el mod y el plugin.

    python -m ie123kit.definitiva paquete --mod RUTA/load/mods/00040000000BB800 --plugin Definitiva-release.3gx \
        -o dist/Inazuma123Definitiva

``--mod`` es la carpeta del mod de la traducción (la que tiene ``romfs/archive.fa``, los CRO y demás). Se copia
entera, su archive.fa se cambia por el de la edición definitiva (``construir``) y el plugin va a
``sdmc/luma/plugins/00040000000BB800/``. El resultado lleva datos del juego: es para uso propio y no se sube
(Norma 2).
"""
from __future__ import annotations

import shutil
from pathlib import Path

from ie123kit.definitiva import construir
from ie123kit.nucleo.contenedores.fa import FaArchive

TITULO = "00040000000BB800"

LEEME = """Inazuma Eleven 1·2·3 — Edición definitiva
=========================================

Lleva dentro la traducción al español (luishidalgoa/inazuma-eleven-123-spanish) y la edición definitiva
encima. Hace falta la ROM japonesa del recopilatorio (CTR-P-AETJ, {titulo}); no va incluida. Es para el
juego SIN actualizar (1.0): si instalaste la actualización 1.4 en Azahar, desinstálala (con ella, los menús
saldrían en japonés).

AZAHAR (Windows, Android, AYN Thor)
-----------------------------------
1. Copia «load» y «sdmc» de este paquete dentro de la carpeta de usuario de Azahar, fusionando con lo que
   haya. En Android es la carpeta que elegiste al abrir Azahar por primera vez.
2. Ajustes → Sistema → «3GX Plugin Loader»: actívalo.
3. Abre el juego. En el menú del recopilatorio, pulsa Select para el menú de la edición definitiva.

3DS CON LUMA3DS (13 o más moderno)
----------------------------------
- «load/mods/{titulo}/romfs» va en la SD como «/luma/titles/{titulo}/romfs».
- «sdmc/luma/plugins/{titulo}/Definitiva.3gx» va como «/luma/plugins/{titulo}/Definitiva.3gx».
- En el menú de Rosalina (L + Abajo + Select): «Plugin Loader» activado; «Game Patching» activado.

CÓMO SE JUEGA DE PRINCIPIO A FIN
--------------------------------
1. Juega el Inazuma Eleven (IE1).
2. Empieza el IE2 (Tormenta de Fuego o Ventisca Eterna) y guarda una vez. Vuelve al menú del recopilatorio,
   pulsa Select y en «Continuidad» elige «Traer mi equipo del IE1…». Sal al título SIN GUARDAR y pulsa
   Continuar.
3. Igual del IE2 al IE3 (Rayo Celeste o Fuego Explosivo) y del IE3 a La amenaza del Ogro.
Quien ya está en la partida nueva mejora. A quien la historia va a unir más adelante no se le crea: llega
con la historia y el plugin le devuelve su nivel y sus técnicas en cuanto aparece (deja activada «Memoria
del equipo» en Ayudas). El resto espera en la reserva: «Grupo y reserva» los pasa al grupo.

QUÉ TRAE
--------
- Continuidad: tu equipo pasa del IE1 al IE2, del IE2 al IE3 y del IE3 a La amenaza del Ogro, con su
  nivel y sus técnicas; «Crónica de la leyenda» con tus tres temporadas.
- Memoria del equipo: nadie pierde lo suyo por la historia. Quien se va y vuelve, vuelve como se fue;
  «Recuperar jugadores» trae de vuelta a quien la historia no vuelve a traer.
- Fichajes especiales: Aiden Froste (cuando Shawn aprende «Aullido de lobo»), «Fichaje libre» (cualquier
  jugador, también los de Wi-Fi o StreetPass), «Supertécnicas a la carta» (cualquier técnica a cualquier
  jugador), «Grupo y reserva» y «Contenido de StreetPass y Wi-Fi»
  (desbloquea lo que en la consola pedía 45 encuentros por StreetPass).
- Ayudas opcionales (de fábrica, como el original salvo la memoria del equipo): encuentros, velocidad al
  andar, supertécnicas del rival, penaltis de la CPU, precio de venta, fuera de juego, versión (exclusivos
  de las dos versiones en una partida), PE y PT siempre llenos, experiencia x2/x3, Pasión y Amistad x2/x3.
- Copias de seguridad de las partidas antes de cada cambio (en la SD, /Inazuma123Definitiva/partidas).
- Y algún secreto: dicen que el abuelo Daisuke dejó un cuaderno...
"""


def montar(mod: Path, plugin: Path, salida: Path) -> list[str]:
    """Monta el paquete en ``salida``; devuelve un resumen. ``salida`` no debe existir (no se pisa nada)."""
    romfs = mod / "romfs"
    base = romfs / "archive.fa"
    if not base.is_file():
        raise FileNotFoundError(f"no está {base}: --mod debe ser la carpeta del mod de la traducción")
    if not plugin.is_file():
        raise FileNotFoundError(plugin)
    if salida.exists():
        raise FileExistsError(f"{salida} ya existe")
    # La capa primero: si los datos no valen (p. ej. ya llevan la edición definitiva), no se crea nada.
    cambios, resumen = construir.capa_definitiva(FaArchive(str(base)))
    destino_mod = salida / "load" / "mods" / TITULO
    try:
        shutil.copytree(mod, destino_mod,
                        ignore=lambda carpeta, nombres: ["archive.fa"] if Path(carpeta) == romfs else [])
        info = construir.construir_archive(base, destino_mod / "romfs" / "archive.fa", cambios)
        resumen.append(f"archive.fa: {info['cambios']} archivos cambiados ({info['tamano']} B)")
        destino_plugin = salida / "sdmc" / "luma" / "plugins" / TITULO
        destino_plugin.mkdir(parents=True)
        shutil.copyfile(plugin, destino_plugin / "Definitiva.3gx")
        (salida / "LEEME.txt").write_text(LEEME.format(titulo=TITULO), encoding="utf-8")
    except BaseException:
        shutil.rmtree(salida, ignore_errors=True)  # nada a medias: se puede volver a lanzar tal cual
        raise
    resumen.append(f"paquete en {salida}")
    return resumen
