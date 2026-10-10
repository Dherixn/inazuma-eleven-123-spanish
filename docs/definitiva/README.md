# Edición definitiva de Inazuma Eleven 1·2·3

Mod que va **encima de la traducción** y convierte la recopilación en un solo juego, del primer partido
del IE1 al último del Ogro:

1. **Continuidad**: el equipo que formas en un juego pasa al siguiente con su nivel y sus técnicas, y las
   historias se enlazan (IE1 → IE2 → IE3 → La amenaza del Ogro).
2. **Versiones juntas**: con una sola partida se juegan los exclusivos de Fuego y Ventisca, o de Rayo
   Celeste y Fuego Explosivo.
3. **Lo que pide la comunidad**: menos farmeo, Aiden Froste jugable, dificultad a elegir y más, con las
   ayudas **siempre opcionales**.

## Qué trae

| Menú del plugin | Qué hace |
|---|---|
| Continuidad | Traer el equipo del IE1 a Tormenta de Fuego o Ventisca, del IE2 a Rayo Celeste, Fuego Explosivo u Ogro y del IE3 a Ogro. Modos: completa (cada uno con su nivel), equilibrada (nivel máximo) y solo fichajes (todos al mismo nivel). La EXP se recalcula con la curva del juego nuevo y las técnicas con su tabla de aprendizaje. Quien ya está en la partida mejora; a quien la historia del juego nuevo va a unir no se le crea ficha: llega con la historia y el plugin le devuelve su nivel, su EXP y sus técnicas al aparecer (la historia no pasa a nadie de la reserva al grupo); el resto, a la reserva. Copia de seguridad en la SD antes de escribir. Texto «Capítulo siguiente» que enlaza las historias. |
| Continuidad → Crónica de la leyenda | Las partidas de los tres juegos vistas como temporadas: episodio, nivel del equipo, jugadores, horas y total. |
| Fichajes especiales | Aiden Froste: en el IE2, cuando Shawn aprende «Aullido de lobo» (cuando deja de esconderse tras su hermano); en IE3 y Ogro, con Shawn en el equipo. Llega con el nivel de Shawn y pasa al IE3 con la Continuidad. «Fichaje libre» (ayuda, con aviso): busca por nombre a cualquier jugador jugable, también los de Wi-Fi o StreetPass, y lo ficha con el nivel que elijas. Si cabe, el fichado entra en el grupo (página 1 de Equipo). «Grupo y reserva»: pasa a cualquiera de la reserva al grupo (el menú del juego no deja al principio del IE2 y del IE3). «Recuperar jugadores»: vuelven, con su nivel y sus técnicas, los que la historia se ha llevado (sus bajas borran al jugador: en el IE3, Kevin, Max y David al no entrar en la selección). |
| Ayudas | Encuentros (normal, menos, muy pocos, ninguno, más), velocidad al andar, supertécnicas del rival (dificultad), penaltis de la CPU (IE3 y Ogro), precio de venta, fuera de juego, versión (exclusivos de la otra), PE y PT siempre llenos (también dentro del partido), experiencia x2/x3. De fábrica, todo como el original salvo la memoria del equipo; se guardan en la SD. «Memoria del equipo» (activada de fábrica: es la continuidad) recuerda lo que llega a tener cada jugador de la partida, en `equipo_<identificador>.txt`: quien llega con la historia tras un traspaso recupera lo suyo, y quien la historia se lleva y vuelve a traer vuelve como se fue (#13: Shawn tras el partido contra Corea). «Recuperar técnicas perdidas» devuelve técnicas a mano. |
| Partidas | Ver los archivos de partida y copiarlos a la SD. |
| (secreto) | Dicen que el abuelo Daisuke dejó un cuaderno... |

Capa de datos (archivos de `archive.fa`, por LayeredFS):

- Aiden Froste jugable en IE2, IE3 y Ogro, con su descripción y sus técnicas. En el IE2 tiene el retrato de
  Atsuya; en IE3 y Ogro ocupa la ficha 2337 (Shawn en modo delantero, con su cara: el retrato de Atsuya no existe
  allí) y juega en Nuevo Alpino y Los Impostores.
- Títulos de equipo del IE2 que no cabían en su caja (#14).

## Instalar

Hace falta la ROM japonesa del recopilatorio (CTR-P-AETJ, `00040000000BB800`) y la traducción ya
montada. Desde la raíz del repositorio:

```
powershell -ExecutionPolicy Bypass -File plugin/compilar.ps1    # plugin/Definitiva-release.3gx
python -m ie123kit.definitiva paquete --mod RUTA/load/mods/00040000000BB800 \
    --plugin plugin/Definitiva-release.3gx -o work/shared/definitiva/Inazuma123Definitiva
```

El paquete es la carpeta de usuario de Azahar (`load/` y `sdmc/`) con un `LEEME.txt`; lleva datos del
juego y no se sube (Norma 2). Para la AYN Thor (Azahar para Android), con la consola conectada por USB:

```
powershell -ExecutionPolicy Bypass -File tools/definitiva_thor.ps1 -Destino /storage/emulated/0/Azahar
```

y en Azahar: Ajustes → Sistema → «3GX Plugin Loader» activado. En una 3DS con Luma3DS ≥ 13, el `romfs` va
en `/luma/titles/00040000000BB800/` y el plugin en `/luma/plugins/00040000000BB800/` (ver el `LEEME.txt`).

## Principios

- **No se toca lo que la traducción ya ha cerrado.** Bloqueo tipográfico v20 y Norma 2 igual que en el
  resto del repositorio. El mod se monta sobre los archivos ya traducidos, nunca sobre los japoneses.
- **Las ayudas se eligen, no se imponen.** Todo se activa y desactiva cuando quieras.
- **Nada del juego en el repositorio ni en el plugin.** El plugin lee las tablas (`unitbase`, `unitstat`,
  `lvup_exp`, `unitno`) del propio `archive.fa` al ejecutarse.
- **Gemelos C++ y Python.** El traspaso, el fichaje y las tablas existen en los dos y se comparan
  (`plugin/pruebas/comparar.py`, `comparar_tablas.py`).

## Arquitectura

| Pieza | Dónde | Qué hace |
|---|---|---|
| Plugin 3GX | `plugin/` (CTRPluginFramework) | menús, traspaso y fichajes sobre los archivos de partida, ayudas sobre la memoria (`INAZUMA.INI` en memoria y la plantilla), secretos |
| Capa de datos | `tools/src/ie123kit/definitiva/` (`construir`, `aiden`, `titulos`) | `archive.fa` de la edición definitiva sobre el traducido |
| Herramientas | `python -m ie123kit.definitiva` | `traspasar`, `construir`, `paquete`, `identidades`, `saltos` |
| Pruebas | `tools/tests/unidad/test_definitiva_*.py`, `plugin/pruebas/` | partidas y tablas sintéticas; en local, con la ROM y Azahar (`azahar_ctl.py`) |

## Probado en Azahar

- Traspasos IE1 → IE2 → IE3: el plugin y la CLI dan partidas idénticas byte a byte y el juego las carga
  con nombres, niveles y estadísticas correctos.
- Aiden fichado en el IE2 desde el plugin y traspasado al IE3: en los dos sale con su retrato, su
  descripción y sus técnicas.
- Ayudas: la versión cambia el título y el contenido (Fuego con «Ventisca» carga la partida de Fuego con
  lo de Ventisca); los parámetros que se leen al cargar el juego llegan a tiempo; EXP x2 y PE/PT llenos
  sobre la plantilla, con una supertécnica en el primer duelo del IE2 (los PT bajan y vuelven al máximo).
- Títulos de equipo, crónica, cuaderno secreto.
- Aiden en un partido de la historia del IE2 (el primero contra los alienígenas): anda con el grupo, entra
  en duelos con su retrato y sale en el resultado.
- Fichaje libre (Byron Love, Tyler Murdock) y su entrada en el grupo; memoria y recuperación de técnicas en
  IE1 e IE2; el prólogo del IE3 con traspasados (siguen en la partida y la Mano celestial traspasada
  funciona).
- Qué hacen las altas y bajas de la historia, con eventos de prueba (HALLAZGOS_3DS.md). Traspaso IE2 → Rayo
  Celeste con aplazados: 10 llegan con la historia, 8 a la reserva, y la memoria que escribe el plugin es
  igual a la de la CLI. Mark creado por la historia a nivel 5 vuelve a su nivel 25 con su EXP en cuanto
  aparece; «Recuperar jugadores» trae a Kevin con su nivel y sus técnicas.
- Saltos de los eventos de la traducción 3.0 contra el japonés (`python -m ie123kit.definitiva saltos`): unos
  768 000 en IE1, IE2 (Fuego y Ventisca), IE3 y Ogro, todos a su sitio. El atasco del sorteo del FFI (#107 de la
  traducción) ya no está en la base.

## Pendiente

- Partida larga de prueba: pasar por los tres juegos de principio a fin en la AYN Thor.
- Convocatoria de Inazuma Japón con traspasados, en una partida de verdad (con eventos de prueba ya se ha
  visto lo que hacen las altas y las bajas).
- Objetos y condiciones del contenido de Wi-Fi o StreetPass (#8, #10); los jugadores ya se pueden fichar.
- Revisar con el equipo de la traducción los títulos acortados (#14).

Hallazgos de 3DS: [HALLAZGOS_3DS.md](HALLAZGOS_3DS.md). De las ROM de DS: [HALLAZGOS_NDS.md](HALLAZGOS_NDS.md).
Peticiones de la comunidad: [PETICIONES.md](PETICIONES.md).
