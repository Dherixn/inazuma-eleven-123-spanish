"""Edición definitiva: los tres juegos de la recopilación como una sola partida.

Capa del mod que va encima de la traducción. Une IE1, IE2 e IE3 con continuidad (los
jugadores, técnicas y fichajes de un juego pasan al siguiente), junta las versiones y
añade lo que más pide la comunidad. Ver ``docs/definitiva/``.

Esta capa trabaja con los tres juegos a la vez, así que no es un juego: puede leer de
``nucleo`` y de los ``comun`` de cada juego, pero ningún juego la importa a ella.
Importar sus módulos no tiene efectos.
"""
