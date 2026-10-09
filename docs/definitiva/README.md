# Edición definitiva de Inazuma Eleven 1·2·3

Mod que va **encima de la traducción** y convierte la recopilación en un solo juego, del primer partido
del IE1 a la final del IE3:

1. **Continuidad**: los jugadores que fichas, las técnicas que aprenden y lo que consigues en un juego
   siguen contigo en el siguiente, con sentido dentro de la historia.
2. **Versiones juntas**: Tormenta de Fuego y Ventisca Eterna en una; Rayo Celeste, Fuego Explosivo y La
   amenaza del Ogro en una.
3. **Lo que pide la comunidad**: menos farmeo, fichar más fácil, contenido de Wi-Fi recuperado, más
   técnicas, personajes que faltaban (Aiden Froste), dificultad a elegir. Las ayudas, **siempre
   opcionales**.

## Principios

- **No se toca lo que la traducción ya ha cerrado.** El bloqueo tipográfico v20 y la Norma 2 (nada de
  ROM ni datos extraídos en git) rigen igual aquí. El mod se monta sobre los ficheros ya traducidos,
  nunca sobre los japoneses, para no pisar la traducción.
- **Las ayudas se eligen, no se imponen.** PT/PE al máximo, multiplicador de EXP, menos encuentros…
  Todo se activa y desactiva. Si no se puede dentro del juego, se elige al instalar.
- **Heredar con sentido.** Lo que pasa de un juego a otro respeta la historia: el IE2 empieza justo
  después del IE1 y el IE3 meses después, con el Inazuma Japón seleccionado de todo el país.
- **Primero datos, luego código.** Lo que se pueda hacer cambiando tablas y ficheros va por LayeredFS. El
  código (plugin) solo entra cuando los datos no bastan.

## Arquitectura

| Pieza | Qué es | Qué hace |
|---|---|---|
| Capa de datos | ficheros de la RomFS sustituidos por LayeredFS (Luma3DS y Azahar) | versiones juntas, Aiden, técnicas, tiendas, fichajes, curvas de EXP, `INAZUMA.INI` |
| Plugin | código que corre junto al juego (plugin 3GX; viabilidad en estudio) | menú de opciones dentro del juego y traspaso de partida entre juegos |
| Herramientas | `tools/src/ie123kit/definitiva/` | leer y escribir las tablas, tabla de equivalencias de jugadores, construir la capa |

## Continuidad: diseño

### Qué se hereda

| Del juego anterior | Cómo llega | Por qué |
|---|---|---|
| Jugadores fichados | fichados en el siguiente (el IE1 y el IE2 comparten numeración; para el IE3 se usa la tabla de equivalencias) | es lo que más pesa en tu partida |
| Técnicas aprendidas | siguen aprendidas si el jugador puede tenerlas en el juego nuevo | — |
| EXP y nivel | según el modo de herencia elegido (abajo). Las curvas de EXP son idénticas en los tres juegos, así que la EXP se traspasa tal cual | — |
| Amistad, objetos raros, dinero | en estudio | depende del formato de partida |
| Medallas del juego anterior | desbloqueos en el siguiente (manual de técnica, jugador secreto, equipo de leyendas) | da motivo para completar cada juego |

### Modos de herencia (opción del jugador)

- **Completa**: nivel y técnicas tal cual. Para quien quiere su equipo exacto.
- **Equilibrada** (recomendada): llega todo, pero el nivel se ajusta al punto de la historia en que
  empieza el juego nuevo, para que el principio no sea un paseo.
- **Solo fichajes**: llegan los jugadores al nivel con que el juego nuevo los traería.

### Enlace de historias

El IE2 empieza con el ataque de la Academia Alius justo después de ganar el Fútbol Frontera, y el IE3 con
la selección del Inazuma Japón. Al empezar partida nueva en el IE2 o el IE3, si hay una partida terminada
del juego anterior, el juego ofrece continuarla. Cada juego arranca con el script de `RPG_SCRIPT_NO`
(`INAZUMA.INI`), lo que abre la puerta a un prólogo de enlace.

## Estado

- [x] Fork y rama `definitiva/continuidad`.
- [x] Fichas de jugador de los tres juegos leídas, incluida la ofuscación del IE3 (`definitiva.fichas`).
- [x] Tabla de equivalencias IE1/IE2/IE3: 1701 de los 1745 jugables del IE2 tienen ficha en el IE3
  (`definitiva.identidades`).
- [x] Mapa de diferencias entre Fuego y Ventisca y ajustes de `INAZUMA.INI`
  ([HALLAZGOS_NDS.md](HALLAZGOS_NDS.md)).
- [x] Peticiones de la comunidad ordenadas y enlazadas al plan ([PETICIONES.md](PETICIONES.md)).
- [x] Formato de partida del IE2 y del IE3 de la recopilación (cabecera, dos CRC32, fichas de jugador) y
  motor de traspaso con sus tres modos (`definitiva.partida`, `definitiva.traspaso`). Probado con
  partidas sintéticas; **falta probarlo con partidas reales**.
- [x] Mecanismo elegido: **plugin 3GX** (CTRPluginFramework). Lo cargan Luma3DS ≥ 13 y Azahar. Su código
  vive en memoria propia, así que no choca con el límite de los CRO. Ver [`plugin/`](../../plugin/README.md).
- [x] Plugin v0.1 compilado: menú propio y copia de las partidas a la SD. El núcleo del traspaso en C++
  da los mismos bytes que el de Python (120 casos).
- [ ] **Bloqueo:** ROM japonesa de la recopilación (CTR-P-AETJ, title ID `00040000000BB800`) en esta
  máquina, para repetir las medidas en 3DS, ver los archivos de partida y probar en Azahar.
- [ ] Ficha de jugador de la partida del IE1 (distinta de la del IE2 y el IE3) y nombres de los archivos de
  partida de cada juego.
- [ ] Campo de la curva de EXP de cada jugador (para el modo equilibrado): saldrá de los pares EXP-nivel de
  una partida real.
- [ ] Capa de datos: versiones juntas, Aiden, `INAZUMA.INI`, curvas de EXP, dificultad.

Detalle medido: [HALLAZGOS_NDS.md](HALLAZGOS_NDS.md).
