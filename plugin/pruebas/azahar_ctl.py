"""Pilotar Azahar (Windows) para probar la edición definitiva: arrancar, pulsar botones, capturas, memoria.

    python plugin/pruebas/azahar_ctl.py iniciar
    python plugin/pruebas/azahar_ctl.py pulsar A [ms]          # A B X Y L R START SELECT ARRIBA ABAJO IZQ DER
    python plugin/pruebas/azahar_ctl.py secuencia "A:120 espera:800 ABAJO A"
    python plugin/pruebas/azahar_ctl.py captura salida.png [abajo]
    python plugin/pruebas/azahar_ctl.py leer 0x08000000 64
    python plugin/pruebas/azahar_ctl.py cerrar

Rutas por variables de entorno: AZAHAR_DIR (carpeta con azahar.exe y scripting/AzaharRPC.py) e IE123_ROM.
Las teclas son las de fábrica de Azahar (A=A, B=S, X=Z, Y=X, cruceta T/G/F/H, L=Q, R=W, Start=M,
Select=N). Hace falta el servidor RPC activado (enable_rpc_server=true en qt-config.ini).
"""
from __future__ import annotations

import contextlib
import ctypes
import os
import subprocess
import sys
import time
from ctypes import wintypes
from pathlib import Path

AZAHAR_DIR = Path(os.environ.get("AZAHAR_DIR", ""))
ROM = os.environ.get("IE123_ROM", "")

TECLAS = {
    "A": 0x41, "B": 0x53, "X": 0x5A, "Y": 0x58, "L": 0x51, "R": 0x57, "START": 0x4D, "SELECT": 0x4E,
    "ARRIBA": 0x54, "ABAJO": 0x47, "IZQ": 0x46, "DER": 0x48,
    # Stick (círculo): flechas del teclado.
    "S_ARRIBA": 0x26, "S_ABAJO": 0x28, "S_IZQ": 0x25, "S_DER": 0x27,
}

user32 = ctypes.WinDLL("user32", use_last_error=True)
KEYEVENTF_KEYUP = 0x0002
KEYEVENTF_SCANCODE = 0x0008


TITULO = 0x00040000000BB800


def _rpc(seleccionar: bool = True):
    sys.path.insert(0, str(AZAHAR_DIR / "scripting"))
    from AzaharRPC import AzaharRPC
    rpc = AzaharRPC(timeout=3)
    if seleccionar:
        # Sin proceso elegido, el servidor avisa de que las lecturas pueden no ser del juego.
        for pid, (titulo, _nombre) in (rpc.process_list() or {}).items():
            if titulo == TITULO:
                rpc.set_process(pid)
                break
    return rpc


def _ventana() -> int:
    encontrada = []

    @ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    def cada(hwnd, _):
        largo = user32.GetWindowTextLengthW(hwnd)
        if largo and user32.IsWindowVisible(hwnd):
            buf = ctypes.create_unicode_buffer(largo + 1)
            user32.GetWindowTextW(hwnd, buf, largo + 1)
            if buf.value.startswith("Azahar"):
                encontrada.append(hwnd)
        return True

    user32.EnumWindows(cada, 0)
    if not encontrada:
        raise SystemExit("no encuentro la ventana de Azahar")
    return encontrada[0]


def _foco() -> None:
    hwnd = _ventana()
    user32.ShowWindow(hwnd, 9)
    user32.SetForegroundWindow(hwnd)
    time.sleep(0.15)


WM_KEYDOWN, WM_KEYUP = 0x0100, 0x0101


def pulsar(boton: str, ms: int = 120) -> None:
    """Manda la tecla a la ventana de Azahar con mensajes: no toca el foco ni el resto del escritorio."""
    vk = TECLAS[boton.upper()]
    scan = user32.MapVirtualKeyW(vk, 0)
    extendida = (1 << 24) if vk in (0x25, 0x26, 0x27, 0x28) else 0  # flechas
    hwnd = _ventana()
    user32.PostMessageW(hwnd, WM_KEYDOWN, vk, 1 | (scan << 16) | extendida)
    time.sleep(ms / 1000)
    user32.PostMessageW(hwnd, WM_KEYUP, vk, 1 | (scan << 16) | extendida | (0xC0 << 24))


def secuencia(texto: str) -> None:
    """Pasos separados por espacios: BOTON, BOTON:ms o espera:ms."""
    for paso in texto.split():
        nombre, _, ms = paso.partition(":")
        if nombre.lower() == "espera":
            time.sleep(int(ms) / 1000)
        else:
            pulsar(nombre, int(ms) if ms else 120)
            time.sleep(0.25)


def captura(salida: str, abajo: bool = False) -> None:
    _rpc().save_screenshot(salida, secondary_window=abajo)
    print(salida)


def iniciar() -> None:
    subprocess.Popen([str(AZAHAR_DIR / "azahar.exe"), ROM], cwd=AZAHAR_DIR)
    for _ in range(60):
        time.sleep(1)
        with contextlib.suppress(Exception):  # aún arrancando
            if _rpc().get_process() is not None:
                print("Azahar responde por RPC")
                return
    raise SystemExit("Azahar no responde por RPC")


def cerrar() -> None:
    subprocess.run(["taskkill", "/IM", "azahar.exe", "/F"], capture_output=True, check=False)


def main(argv: list[str]) -> None:
    orden, *resto = argv
    if orden == "iniciar":
        iniciar()
    elif orden == "pulsar":
        pulsar(resto[0], int(resto[1]) if len(resto) > 1 else 120)
    elif orden == "secuencia":
        secuencia(resto[0])
    elif orden == "captura":
        captura(resto[0], len(resto) > 1 and resto[1] == "abajo")
    elif orden == "leer":
        datos = _rpc().read_memory(int(resto[0], 0), int(resto[1], 0))
        print(datos.hex(" "))
    elif orden == "cerrar":
        cerrar()
    else:
        raise SystemExit(__doc__)


if __name__ == "__main__":
    main(sys.argv[1:])
