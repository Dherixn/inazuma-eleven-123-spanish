# Edición definitiva — lo medido en las ROM NDS

Medido el 2026-10-09 sobre las ROM NDS en castellano (IE1 `YEES`, IE2 Tormenta de Fuego `BEES`, IE2
Ventisca Eterna `BEBS`) y sobre el IE3 *La amenaza del Ogro* de DS (`BOEJ`, japonesa con traducción fan
a nombres europeos). El recopilatorio de 3DS es un port de estos juegos (`inazumaN/data_iz/`), así que
casi todo debería valer allí. **Todo lo que dependa del 3DS está marcado como «a comprobar en 3DS».**

Extracción: `python tools/nds_unpack.py <rom.nds> work/<juego>/fuentes/nds_es` (Norma 2: `work/` no
se sube).

## 1. Numeración de jugadores: la llave de la continuidad

| Juego | Fichas `unitbase.dat` | Tamaño | `unitstat.dat` | Tamaño |
|---|---|---|---|---|
| IE1 | 2400 | 96 B | 2400 | 80 B |
| IE2 | 2400 | 96 B | 2400 | 80 B |
| IE3 | 2582 | 104 B | 2582 | 72 B, **ofuscado** |

- **IE1 e IE2 comparten numeración.** 2252 de las 2400 fichas tienen el mismo nombre en el mismo
  índice. Las demás son huecos que el IE1 reservaba para el IE2 (conservan el nombre japonés en la ROM
  española) o pequeños cambios de nombre («T. Fielding» → «Tammy Fielding»).
- **El IE3 renumera.** Solo 33 índices coinciden. Para pasar un jugador del IE2 al IE3 hace falta la
  tabla de equivalencias.
- **La cara se conserva entre juegos** (campo `cara` del bloque de identidad). Con nombre + cara se
  emparejan 1629 de los 1745 jugadores jugables del IE2; la cara sola resuelve 69 más, casi todos
  alienígenas de Alius cuyo nombre real aparece en el IE3 («Grent» → «Grant Cook»). En total,
  **1701 de 1745 jugadores del IE2 tienen ficha en el IE3**. Los 44 restantes son NPC (matones,
  hombres de negro), los invitados de *El profesor Layton* y los «O'» de relleno.

Herramienta: `python -m ie123kit.definitiva identidades --ie1 … --ie2 … --ie3 … -o …` (ver
`tools/src/ie123kit/definitiva/`). Tests: `tools/tests/unidad/test_definitiva_identidades.py` y, con
`work/`, `tools/tests/requiere_rom/test_definitiva_identidades_nds.py`.

## 2. Ficha de identidad (`unitbase.dat`)

Nombre completo en +0 (32 B), nombre corto en +32 (16 B). El bloque de identidad empieza en +64 (IE1,
IE2) o +76 (IE3). En el IE3 empieza 4 B más tarde y le faltan 4 B de ceros a mitad, así que posición y
elemento se desplazan 8 B, no 12.

| Campo | IE1/IE2 | IE3 | Valores |
|---|---|---|---|
| cara (u16) | +66 | +78 | se conserva entre juegos |
| gráficos (4 × u16) | +66 | +78 | cara, retrato y cuerpo; aún sin separar |
| posición (nibble alto) | +85 | +93 | 2 PO, 4 DF, 6 CC, 8 DL |
| elemento | +90 | +98 | 1 viento, 2 bosque, 3 fuego, 4 montaña (deducido) |

## 3. Ficha de crecimiento (`unitstat.dat`)

Igual en los tres juegos salvo el tamaño y la ofuscación del IE3. Esta es **la misma transformación
que `item.dat`** (`ie3/comun/items.py`): XOR 0xAD, rotación de 2 bits e intercambio de extremos en
grupos de 3, 5, 7 y 2.

| Offset | Campo |
|---|---|
| +0 / +2 / +4 | PE base (u16), PE máximo (u16), curva (u8) |
| +8 / +10 / +12 | PT base, PT máximo, curva |
| +16 + 4k (k = 0…6) | atributo k: base (u8), máximo (u8), curva (u8). Orden habitual: Tiro, Físico, Control, Defensa, Rapidez, Aguante, Valor (a confirmar en pantalla) |
| +44 + 4k (k = 0…3) | técnica k (u16) y nivel en que la aprende (u16 en IE1/IE2; u8 + u8 en IE3) |

**Nivel 100 = no se aprende subiendo de nivel** (la da la historia o un manual): Mark aprende la Mano
Celestial en el IE1 por la historia y en el IE2 la trae desde el nivel 1.

Las fichas de NPC llevan PE máximo 1 y todos los atributos a 1 (`Crecimiento.jugable`).

## 4. Aiden Froste (Atsuya Fubuki)

- En el **IE2** existe la ficha 1082 «Aiden Froste», pero es su **versión de niño** para los recuerdos
  (`吹雪アツヤ（幼少）`), con estadísticas de NPC. Junto a ella está la 1081, Shawn de niño.
- **Shawn Froste tiene tres fichas jugables**: 1505 (defensa, «Paisaje helado»), **1513 (delantero,
  la personalidad de Atsuya: Ventisca Eterna, Entrada Huracán, Regate Aurora, Chut Congelante)** y
  1514 (despierto). La 1513 tiene su propia cara (0x0921), distinta de la 1505 (0x0919).
- En el **IE3**, las mismas tres van a 1161, 1169 y 1170, más una 2374 con otra cara (0x0F65).

**Consecuencia:** un Aiden jugable es sobre todo trabajo de datos. Basta una ficha libre con su nombre,
la cara y el cuerpo de la 1513 (o un retrato nuevo con su pelo), sus técnicas y una forma de
ficharlo. Falta decidir en qué hueco va y cómo se ficha (ver `ScoutData.dat` y `JinmyakuData.dat`).

## 5. Versiones: Fuego y Ventisca son la misma ROM

Las dos ROM del IE2 tienen 1309 archivos y **solo 43 difieren**: `INAZUMA.INI`, `ROM_TIME.TXT`, cuatro
vídeos (`op00`, `a2y01`–`a2y03`) y las imágenes del final (`pic2d/ending`). **Todos los datos de juego
son idénticos**: los ficheros propios de cada versión van en las dos copias con sufijo `_F` o `_B`
(`BinderData_F/_B`, `PracticeDataF/B`, `PracticeGame0F…4F/B`). La versión la elige `IZ_TYPE` en
`INAZUMA.INI` (0 Fuego, 1 Ventisca).

Dentro de esas parejas, lo que cambia es mínimo:

| Fichero | Bytes distintos | Qué cambia |
|---|---|---|
| `BinderData_F/_B.dat` (catálogo de equipos del archivador) | 18 | la entrada del equipo 0x2F **Prominence** (Fuego) frente al 0x30 **Polvo de Diamantes** (Ventisca) |
| `PracticeDataF/B.dat` (104 partidos de entrenamiento de 18 B) | 2 | los rivales de los partidos 65 (0x15F/0x16C) y 82 (0x49/0x4A) |
| `PracticeGame0F…4F/B.dat` | 0 | idénticos |

El resto de diferencias entre versiones (qué capitán se ficha, contra quién se juega) tiene que estar en
los scripts de evento, que son los mismos en las dos copias y preguntan por la versión.

**Consecuencia:** juntar las versiones no exige traer contenido de fuera. Hace falta que el juego cargue
las dos listas en vez de una, o fundir las `_B` dentro de las `_F`. A comprobar en 3DS: cómo pasa la
recopilación la versión elegida en el menú a `ina_main2.cro` (si lee `IZ_TYPE` o la fija ella).

El IE3 trae además los ficheros del Ogre Premium Link (`premium_item.dat`, `premiumcondition.dat` y
sus variantes `_hurri`) y torneos (`Tournament*.dat`).

## 6. `INAZUMA.INI`: ajustes de juego en texto plano

Cada juego lleva un `data_iz/INAZUMA.INI` en Shift-JIS con parámetros comentados. Los que sirven al mod:

| Clave | Valor | Qué hace | Uso en la edición definitiva |
|---|---|---|---|
| `RPG_ENCOUNT_TIME` | 60 | intervalo de la comprobación de encuentros aleatorios | menos encuentros |
| `RPG_MOVESPEED` | 320 | velocidad al andar | andar más rápido |
| `CPU_USE_SPECIAL_PERCENT` | 40 | probabilidad de que la CPU use supertécnica | dificultad |
| `SPECIAL_SKILL_SPEND_TP` | 1 | las técnicas gastan PT (0 = no) | opción «PT infinitos» |
| `ITEM_SELL_PERCENT` | 50 | precio de venta de objetos | economía |
| `IZ_TYPE` | 0/1 | versión | juntar versiones |
| `RPG_SCRIPT_NO` | 91010000 / 21010000 / 31010000 | script de arranque de cada juego | enlazar historias |
| `SEARCH_LV191`…`200` (IE3) | 40…100 | probabilidad de los minipartidos de fichaje | fichar más fácil |
| `HEATUP_REMAIN_MAX` (IE3) | 420 | duración del estado «ardiente» | — |

Los valores se leen al arrancar, así que cambiarlos por LayeredFS fija la opción para toda la partida.
Para poder activarlos y desactivarlos en el juego hace falta tocar en memoria los valores ya leídos
(plugin) o que el juego vuelva a leer el fichero. A comprobar en 3DS: que cada `inazumaN/data_iz/` lleve
su `INAZUMA.INI` y que se siga leyendo.

## 7. Curvas de experiencia (`lvup_exp.dat`)

16 curvas × 98 valores u32: la EXP acumulada para llegar a los niveles 2…99 (la curva 0 empieza 100,
300, 600, 1000…; la 15 está vacía). **Son idénticas en los tres juegos**, así que la EXP de un jugador
se puede traspasar tal cual y conserva su nivel. Escalarlas es la forma de datos de «menos farmeo»
(a comprobar si la partida guarda el nivel o lo recalcula desde la EXP).

## 8. Otros ficheros de lógica a estudiar

`ScoutData.dat`, `ScoutSpCode.dat`, `HeadhuntData.dat` (fichajes), `JinmyakuData.dat`/`JinmyakuLV.dat`
(mapa de contactos), `lvup_command.dat`, `cmd_cond.dat` (requisitos de
técnicas, probablemente las combinadas), `shop.dat` (tiendas), `TokkunData.dat` (entrenamientos),
`TradeData.dat` (intercambios). Además, el IE2 incluye en `logic01/` los equipos, estadísticas y
partidos del IE1 (`team.pkb`, `unitstat.dat`, `games.dat`, `fieldinf.dat`, `PracticeData*`), con el
mismo tamaño que en el IE1.
