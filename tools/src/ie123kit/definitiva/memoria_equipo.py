"""Memoria del equipo de una partida: lo que llegó a tener cada personaje y quién llega con la historia.

Gemelo de ``plugin/Sources/Nucleo/MemoriaEquipo.cpp`` (mismo texto y mismas reglas; lo comprueban
``plugin/pruebas/memoria_pc.cpp`` y ``tools/tests/unidad/test_definitiva_memoria.py`` con los mismos casos).

Por qué hace falta (medido en Azahar con eventos de prueba, ver ``docs/definitiva/HALLAZGOS_3DS.md``):

- La historia une jugadores con la instrucción 16446 (alta). Con alguien que ya está en la partida no hace
  nada: ni lo duplica ni le baja el nivel, pero tampoco lo pasa de la reserva al grupo.
- La 16447 (baja) borra el registro: si la historia lo vuelve a unir, llega nuevo, con el nivel de la historia.

Por eso el traspaso no crea a quien la historia va a unir (se quedaría en la reserva y el grupo, corto): lo
apunta aquí como pendiente y el plugin, al verlo aparecer en la plantilla, le devuelve nivel, EXP y técnicas.
Igual con quien la historia borra y vuelve a unir con menos nivel del que tuvo.

Va por la clave del personaje (la del nombre) y no por número: uno puede tener varias fichas en un juego.
Una por archivo de partida, ``equipo_<identificador>.txt``: el identificador está en la cabecera (+0x20) y no
cambia desde que se crea el archivo. Para no mezclar una partida nueva sobre el mismo archivo con la anterior se
guarda el tiempo de juego (info + 0x0C, en fotogramas): si baja, es otra partida.

Texto, una línea por personaje, «clave=t1:n1,t2,...;n=número;nv=nivel;xp=EXP;p=1» (``:n`` es el nivel de la
técnica, si no es 0; ``p=1``, pendiente), detrás de «#partida=XXXXXXXX» y «#tiempo=N».
"""
from __future__ import annotations

from dataclasses import dataclass, field

from ie123kit.definitiva.partida import JugadorPartida

MAX_POR_JUGADOR = 16


@dataclass
class Recuerdo:
    numero: int = 0
    nivel: int = 0
    exp: int = 0
    # Llega con la historia (traspaso): al aparecer se le devuelve todo, aunque la historia lo traiga con más nivel.
    pendiente: bool = False
    # (técnica, nivel de la técnica), en el orden en que las aprendió.
    tecnicas: list[tuple[int, int]] = field(default_factory=list)


def _poner_tecnica(tecnicas: list[tuple[int, int]], tecnica: int, nivel: int) -> bool:
    for k, (t, n) in enumerate(tecnicas):
        if t == tecnica:
            if nivel > n:
                tecnicas[k] = (t, nivel)
                return True
            return False
    if len(tecnicas) >= MAX_POR_JUGADOR:
        del tecnicas[0]  # se olvida la más antigua
    tecnicas.append((tecnica, nivel))
    return True


class MemoriaEquipo:
    def __init__(self, partida: int = 0, tiempo: int = 0):
        self.partida = partida
        self.tiempo = tiempo
        self.recuerdos: dict[int, Recuerdo] = {}

    def apuntar(self, clave: int, numero: int, nivel: int, exp: int, tecnicas, niveles=None) -> bool:
        """Lo que tiene ahora un jugador de la plantilla (técnica 0 = hueco libre). Se queda el nivel más alto
        que ha tenido y deja de estar pendiente. True si cambia algo que haya que guardar."""
        r = self.recuerdos.setdefault(clave, Recuerdo())
        cambio = False
        for k, t in enumerate(tecnicas):
            if t:
                cambio |= _poner_tecnica(r.tecnicas, t, niveles[k] if niveles else 0)
        if numero and numero != r.numero:
            r.numero, cambio = numero, True
        if (nivel, exp) > (r.nivel, r.exp):
            r.nivel, r.exp, cambio = nivel, exp, True
        if r.pendiente:
            r.pendiente, cambio = False, True
        if not r.tecnicas and not r.numero:
            del self.recuerdos[clave]
        return cambio

    def esperar(self, clave: int, recuerdo: Recuerdo) -> None:
        """Llega con la historia (traspaso): sustituye lo que hubiera de ese personaje."""
        self.recuerdos[clave] = Recuerdo(recuerdo.numero, recuerdo.nivel, recuerdo.exp, True,
                                         list(recuerdo.tecnicas[-MAX_POR_JUGADOR:]))

    def perdidas(self, clave: int, tecnicas) -> list[int]:
        """Las que recuerda y no tiene ahora, en el orden en que las aprendió."""
        r = self.recuerdos.get(clave)
        return [t for t, _ in r.tecnicas if t not in tecnicas] if r else []

    def serializar(self) -> str:
        lineas = [f"#partida={self.partida:08X}", f"#tiempo={self.tiempo}"]
        for clave in sorted(self.recuerdos):
            r = self.recuerdos[clave]
            linea = f"{clave}=" + ",".join(f"{t}:{n}" if n else str(t) for t, n in r.tecnicas)
            if r.numero:
                linea += f";n={r.numero};nv={r.nivel};xp={r.exp}"
            if r.pendiente:
                linea += ";p=1"
            lineas.append(linea)
        return "\n".join(lineas) + "\n"

    @classmethod
    def cargar(cls, texto: str) -> MemoriaEquipo:
        """Tolerante: se salta lo que no entiende (líneas antiguas «clave=t1,t2» incluidas)."""
        m = cls()
        for linea in texto.split("\n"):
            if linea.startswith("#"):
                nombre, _, valor = linea[1:].partition("=")
                if nombre == "partida":
                    m.partida = _entero(valor, 16, 0xFFFFFFFF)
                elif nombre == "tiempo":
                    m.tiempo = _entero(valor, 10, 0xFFFFFFFF)
                continue
            texto_clave, igual, resto = linea.partition("=")
            clave = _entero(texto_clave, 10, 0xFFFFFFFF) if igual else 0
            if not clave:
                continue
            lista, *campos = resto.split(";")
            r = Recuerdo()
            for elemento in lista.split(","):
                t, dos, n = elemento.partition(":")
                tecnica = _entero(t, 10, 0xFFFF)
                if tecnica and len(r.tecnicas) < MAX_POR_JUGADOR:
                    r.tecnicas.append((tecnica, _entero(n, 10, 0xFF) if dos else 0))
            for campo in campos:
                nombre, _, valor = campo.partition("=")
                if nombre == "n":
                    r.numero = _entero(valor, 10, 0xFFFF)
                elif nombre == "nv":
                    r.nivel = _entero(valor, 10, 99)
                elif nombre == "xp":
                    r.exp = _entero(valor, 10, 0xFFFFFFFF)
                elif nombre == "p":
                    r.pendiente = valor == "1"
            if r.tecnicas or r.numero:
                m.recuerdos[clave] = r
        return m


def _entero(texto: str, base: int, maximo: int) -> int:
    """Como ``Entero`` del C++: solo dígitos de esa base; vacío, basura o fuera de rango = 0."""
    digitos = "0123456789abcdefABCDEF" if base == 16 else "0123456789"
    if not texto or len(texto) > 10 or any(c not in digitos for c in texto):
        return 0
    valor = int(texto, base)
    return valor if valor <= maximo else 0


def restaurar(j: JugadorPartida, r: Recuerdo) -> JugadorPartida | None:
    """Lo que hay que devolver a un jugador que acaba de aparecer: si estaba pendiente o la historia lo ha vuelto a
    crear con menos nivel del que tuvo, el nivel y la EXP (si eran más) y las técnicas recordadas que no tiene, en
    sus huecos libres (o su nivel de técnica, si era mayor). None si no hay nada que cambiar."""
    if not (r.pendiente or r.nivel > j.nivel):
        return None
    tecnicas, niveles = list(j.tecnicas), list(j.niveles_tecnica)
    cambio = False
    nivel, exp = j.nivel, j.exp
    if (r.nivel, r.exp) > (j.nivel, j.exp):
        nivel, exp, cambio = r.nivel, r.exp, True
    for t, n in r.tecnicas:
        if t in tecnicas:
            k = tecnicas.index(t)
            if n > niveles[k]:
                niveles[k], cambio = n, True
        elif 0 in tecnicas:
            k = tecnicas.index(0)
            tecnicas[k], niveles[k], cambio = t, n, True
    if not cambio:
        return None
    return j.con(nivel=nivel, exp=exp, tecnicas=tuple(tecnicas), niveles_tecnica=tuple(niveles))
