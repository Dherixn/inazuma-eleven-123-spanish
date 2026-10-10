# Hallazgos en la recopilación de 3DS (CTR-P-AETJ)

Medido sobre la ROM japonesa con la traducción montada, en Azahar (lectura y escritura de memoria por
RPC) y desensamblando los módulos (`cro/ina_main1.cro`, `ina_main2.cro`, `ina_main3ogre.cro`). Nada de
esto copia datos del juego al repositorio: son posiciones, tamaños y comportamientos.

## Partidas

- Archivos (de `code.bin`): `ina1_save.dat`, `ina2F_save.dat`, `ina2B_save.dat`, `ina3S_save.dat`,
  `ina3B_save.dat`, `ina3O_save.dat` y `Menu.sav` (24 B; el byte 6 es el último juego elegido: 0 IE1,
  1 Fuego, 2 Ventisca, 3 Rayo Celeste, 4 Fuego Explosivo, 5 Ogro).
- 64 KB. Cabecera de 0x40 (CRC32 en 0x00 de 0x04..0x3F). IE1: tres espacios (datos 0x54C0); IE2 e IE3: uno
  (0x7DC0 / 0x5940). Cada espacio: 0x80 de información (CRC de la información en +0x40, de los datos en
  +0x44, marca 0x19150A04 en +0x48, tiempo en fotogramas a 60 por segundo en +0x50, episodio en +0x54,
  nivel de equipo en +0x55, jugadores en +0x57) y los datos.
- Jugadores: IE1 0x60 B desde 0x79C (máx. 32), IE2 0x70 B desde 0x1268, IE3 0x6C B desde 0x11B0. EXP en
  +0, número en +4 (el de `unitno.dat`, no el índice de ficha), PE y PT actuales en +0x34, técnicas en
  +0x38 (u8 en el IE1, u16 en IE2/IE3), nivel en +0x3E (IE1) o +0x4A.
- El juego recalcula estadísticas y máximos de PE/PT a partir del nivel.
- Sitio en el equipo (IE2 e IE3, en la ficha): dorsal en +0x57 y sitio en la formación en +0x58 (los dos a
  0 en la reserva: el juego los limpia al guardar), órdenes en +0x5B (0x10 y, encima, capitán y
  lanzadores: Mark 0x1F en el IE2, 0x15 en el IE3). IE2: puesto entre los cuatro de cabecera (los que
  acompañan por el mapa y juegan los 4 contra 4) en +0x5D, y su lista de números en 0x98. IE3: +0x4B es el
  equipo (1 en los naturales; 0x36 el Orfeo de Paolo Bianchi, el del prólogo).
- **Grupo** (los 16 que juegan; el resto es la reserva, página 2 de Equipo): en el IE2, una lista de 16
  números (u16) 0x40 B detrás de los 100 registros (0x3E68); el byte 0x56 de la información es su resumen.
  Al guardar, el juego reordena los registros (grupo primero). En el IE2 y el IE3, el menú Equipo no deja
  pasar a nadie de la reserva al grupo (al menos al principio).
- Grupo del IE3: lista de 16 números 0x5E B detrás de los registros (0x3C3E), resumen en 0x56. Con la lista
  vacía (tras el prólogo), el grupo son los primeros registros. Al entrar alguien, el juego le da dorsal y deja
  el sitio a 0. En el prólogo el menú pinta como mucho a 7 (los primeros de la lista); el resto sale en la
  reserva. Se localizó al revés: la lista sí está en memoria; metiendo a alguien ahí y guardando desde el
  juego, el archivo la recoge.
- El grupo del IE3 solo pinta fichas nativas de jugador con recursos de cara: una ficha nueva o reciclada (aun
  copiando la de Shawn entera) sale en la reserva sin icono y no en el grupo, y el retrato de Atsuya (0x51B)
  no existe en el IE3. Por eso Aiden toma allí la ficha 2337 (Shawn en modo delantero), que la historia no
  ficha.
- Prólogo del IE3: juegas con Paolo Bianchi (número 3170) y luego la historia lo quita y deja a Mark solo en
  el grupo. Si se ha traspasado antes, los traspasados siguen (Mark con su nivel, el resto en la reserva).

## Tablas

- `unitbase` (IE1/IE2 96 B, IE3 104 B), `unitstat` (64 / 80 / 72 B; el del IE3, ofuscado como `item.dat`),
  `unitno` (u16 por ficha, en orden estricto, las libres al final con 0), `lvup_exp` (98 u32 por curva).
- Bloque de identidad (IE1/IE2 en +64, IE3 en +76): curva de EXP, colegio, caras (+2, +4, +10), modelo
  (+8), posición (IE2 +21), elemento (IE2 +26) e índice de la descripción en `unitbase.STR` (u16 × 32,
  último campo de la ficha).
- Las caras de Shawn (0x919, 0x921, 0x922) van atadas a su cambio de forma (`unitchange.dat`): cualquier
  ficha con ellas sale como Shawn defensa. Aiden usa la cara 0x51B (el retrato de Atsuya), que existe en
  IE2 e IE3.
- Técnicas: IE1 `command.dat` de 24 B (nombre en +16, × 32 en `command.STR`), IE2 de 28 B (+20), IE3 de
  36 B (+24). Los números de técnica coinciden en IE2 e IE3. Los nombres de técnica van en letras normales
  en los tres (los de jugador del IE1/IE2, no).
- Shawn tiene tres fichas jugables en el IE3 (2329 DF, 2337 y 2338 DL): por eso la memoria de técnicas va
  por personaje (la clave del nombre) y no por número.
- Títulos de equipo del IE2 (`rpgtitle.STR`): la caja tiene 14 casillas.
- **Nivel de los rivales: `logic/games.dat`**, un registro de 48 B por partido (historia, duelos y encuentros
  por el mapa; en los encuentros, el registro es el número del equipo): equipo rival (u16, el número de la
  cabecera de `team.pkb`, +32) en +0, 1 en +2 si lleva nivel y **el nivel en +3**. Cada jugador del rival suma
  su ajuste de `team.pkb` (byte +4 de su hueco de 8 B; de 0 a 40). El juego tiene la tabla entera en el heap
  mientras se juega (no en el título ni en el mapa del mundo) y la vuelve a cargar del archivo al cambiar de
  zona, a veces en otro sitio. Medido en Azahar con un duelo del IE2 (registro 35, nivel 1): con el +3 a 77
  sus cuatro rivales salen a nivel 77 y sus atributos pasan de 4-8 a 23-79; con la ayuda «a tu nivel», a 17.
  Así que la dificultad no estaba en `team.pkb` (#11): ver `Nucleo/Rivales.hpp` y `RivalesEnJuego.hpp`.
- Descripciones del IE2: la traducción junta letras de dos en dos con glifos propios (tabla generada fuera
  del repositorio); en ancho completo caben 18 por línea. Las del IE3 van en ancho completo.

## INAZUMA.INI

- El juego lo deja en memoria como una tabla de entradas de 0x40 B (clave y valor en texto), al principio
  del heap lineal, siempre en el mismo sitio para cada juego (0x14015280 en el IE2, 0x14018E80 en el IE3)
  pero distinto entre juegos. Primera clave: `FARCLIP_LENGTH`. `GetIniInt` busca la clave y hace `atoi` en
  cada llamada.
- `RPG_ENCOUNT_TIME`: se relee al reiniciar el contador de encuentros (contador = valor × 30).
- `RPG_MOVESPEED`: al entrar en el modo campo.
- `CPU_USE_SPECIAL_PERCENT`, `ITEM_SELL_PERCENT`, `OFFSIDE_EFFECTIVE`: una vez, al cargar el juego, justo
  después del INI (con varias cargas de archivo en medio: un hilo que mira cada milisegundo llega antes).
- `IZ_TYPE` (0 Fuego / Rayo Celeste, 1 Ventisca / Fuego Explosivo): lo único que cambia entre los INI de
  cada versión; el código lo consulta en cada uso: `rpgencountf/b.dat`, `BinderData_F/B`, `PracticeData
  F/B` y, en el IE2, la carpeta `data_iz_blizzard` (scripts de eventos). La partida es la del juego elegido.
- `SPECIAL_SKILL_SPEND_TP` y `FUSEI_ITEM_NO` no los lee ningún módulo.

## Scripts de evento

- `inazumaN/data_iz/script/eve.pkh` (índice «PackNum»: entradas de 12 B desde 0x30, `id, offset, tamaño`,
  y un centinela 0xFFFFFFFF) + `eve.pkb` (cada evento, un SSD comprimido en LZ10). El IE2 tiene además
  `data_iz_blizzard/script` (Ventisca); Fuego Explosivo usa los de Rayo Celeste.
- Instrucción 16446: une un jugador al equipo (`número, nivel, dorsal, ?, ?`; en el prólogo del IE3,
  `[3170, 30, 10, 0, 1]`). 16447: lo quita. 12376/12378/12381/12383/12399 llevan el número del personaje en
  las escenas.
- Varios personajes tienen varias fichas jugables; la historia solo ficha una (IE3: Jude 30, no 1097; Shawn
  2338; Caleb 3005; Nathan 2 y Kevin 11, no sus 24xx). El traspaso usa la que ficha la historia.
- Lo que hacen 16446 y 16447, medido en Azahar cambiando las bajas del arranque del IE3 (evento 31010000) por
  altas y bajas de prueba:
  - Un alta de alguien que ya está no hace nada: no lo duplica, no le cambia el nivel (Paolo a 50 sigue a 50
    tras el alta de la historia a nivel 30) y **no lo pasa de la reserva al grupo**, aunque haya hueco.
  - Un alta de alguien nuevo lo crea con el nivel del alta y entra en el grupo si cabe (16); si no, a la
    reserva.
  - Una baja **borra el registro**: si la historia lo vuelve a unir, llega nuevo (Kevin a 40, baja y alta a 7:
    queda a 7).
  - Una partida nueva del IE3 trae a Raimon (1 a 12, nivel 1), y las bajas del arranque los quitan.
  - Consecuencia para el traspaso: quien la historia va a unir no se crea (se quedaría en la reserva y el
    grupo, corto o vacío: tras el prólogo del IE3 la historia quita a Paolo y Héctor y une a Mark). Se apunta
    en la memoria del equipo y el plugin le devuelve lo suyo cuando la historia lo crea.
- Saltos: un argumento de tipo 4 es el identificador de la instrucción de destino (0x6001/0x6002 y otras). En
  0x301D (texto) el de tipo 4 es un índice de texto. Insertar instrucciones obliga a desplazar los destinos
  (#107 de la traducción); `python -m ie123kit.definitiva saltos` lo comprueba contra el japonés.
- Bajas de la historia del IE3: en la selección (32011850) Kevin, Max, David, Marvin, Shadow e Isaac; tras
  Corea (32041600) Shawn y Jordan (Shawn vuelve después); en 32070100, Tod.

## StreetPass y contenido de Wi-Fi

- `Menu.sav` (24 B, el menú de la recopilación): +6 último juego elegido; **+8 u32, encuentros por StreetPass**;
  +12, +16 y +20 (u32), cuántos avisos de desbloqueo ha dado ya el menú de IE1, IE2 e IE3 (9, 9 y 8 con todo).
- Al abrir el juego, el menú compara el contador con los umbrales y avisa uno a uno: «N o más encuentros por
  StreetPass: Inazuma Eleven. Has desbloqueado sus personajes. Desde el capítulo X podrás conseguirlos en tu
  partida». El IE1 empieza en 3 (capítulo 3) y acaba en 43 («personajes y objetos, tras acabar la historia»);
  luego el IE2 (ambas versiones) y el IE3 (todas las versiones). Con 45: los 27 avisos y «Has desbloqueado todos
  los personajes y objetos». Sin conexión, el contador no pasa de 0 en un emulador.
- El menú guarda su copia de `Menu.sav` al elegir un juego: tras cambiar el archivo hay que cerrar el juego.
- Los avisos son texturas de `menu/extra.arc` (`ie99_extra_pas_open_mes_b01..b03`). El panel dorado del final
  (`b02`) se dibuja con dos trozos; el pequeño conservaba el arranque de un carácter japonés que salía como una
  «t» suelta: la edición definitiva lo tapa (`definitiva/texturas_menu.py`).
- Lo que desbloquea en el IE3 incluye `premium_item.dat` (19 entradas de 36 B: objeto, 1 y hasta 8 pares
  número de jugador, nivel): manuales de The Earth, Gungnir, Procyon Net, Rock Wall Dam, Neo Galaxy, Drill
  Smasher, Triple Dash y Death Zone 2, uniformes Gemini, Epsilon, Genesis y Caos, y billetes de Ura-Zeus y
  Genesis. Sus nombres siguen en japonés en `item.dat` (registros de 44 B, desofuscados uno a uno con
  `fichas.desofuscar`; nombre de 29 B al principio; en +30 la técnica de los manuales; en +42 el índice ×32 de
  la descripción en `item.STR`). La edición definitiva los pone en español (`definitiva/objetos.py`) con las
  descripciones nuevas al final de `item.STR`: el juego las lee (probado en Azahar). Con ellos, todos los
  que seguían en japonés (30 en el IE3 y 33 en el Ogro: también llaves, «pruebas», botas de marca y manuales
  sueltos). Solo quedan los que no se usan: las cinco «システム予約», «ゴーストロック大(仮)» y el manual 413, de una
  técnica sin nombre ni en japonés.

## Identificar una partida

- Cabecera del archivo, +0x20: un identificador de 32 bits que no cambia desde que se crea el archivo (ni con
  una partida nueva encima: se probó). Distinto en cada archivo (Rayo Celeste, Fuego Explosivo y Ogro tienen
  el suyo); el IE1 lleva sus tres espacios en un archivo, con uno solo.
- Info del espacio, +0x0C (archivo 0x50 en el espacio 1): tiempo de juego en fotogramas a 60 por segundo.
  Una partida nueva sobre el mismo archivo lo hace bajar.
- En la RAM, el juego tiene la cabecera (0x40 B, con su suma bien) en el heap (IE3: 0x087EFA18 en una
  sesión); detrás hay otra cosa, así que el tiempo de juego se lee del archivo.
- Los registros más allá del recuento no llegan a la RAM: no sirven para guardar datos propios en la partida.
- Pasión y Amistad: dos u32 en +0x4D4 del archivo (IE2 e IE3). En la RAM del IE3 están en una copia de los
  datos de la partida en el heap con la misma disposición que el archivo (en una sesión, 0x088092D0: Pasión en
  +0x4D4 y el identificador de la partida en +0xBA8, que es como se encuentra). Es la copia viva: es la única.

## Aviso de «datos ilegítimos»

Al continuar una partida del IE3 sale una pantalla que avisa de que, si se detectan datos ilegítimos, «se
activará un programa que te perjudicará en los enfrentamientos». Es un aviso fijo, no una detección: sale
igual con una partida sin tocar y la pantalla (`CMainTitleScreenCaution`) está en los tres módulos, IE1
incluido. Ningún módulo exporta símbolos de comprobación de partidas y `FUSEI_ITEM_NO` no lo lee nadie; el
«resultado no válido» de los scripts de partido es del modo de conexión. Queda por vigilar en partidas
largas por si hubiera algo sin nombre.

## Plantilla en memoria

- Su dirección cambia entre sesiones (la traducción cambia el tamaño de lo que se carga antes): se busca
  por su forma. IE1 cada 0x188 (PE/PT máximos en +0xAC), IE2 cada 0xFC (+0x70), IE3 y Ogro cada 0x6C con
  los máximos aparte (inicio + 0xCA6C + i × 0x98). Los actuales, en el registro (+0x34).
- El partido gasta los PT del propio registro (no hay copia).
- Pasión y Amistad: IE1 plantilla − 0xCE4, IE2 plantilla − 0xF0C; en el IE3 van en otro bloque.
- Dentro del heap hay código de los CRO en solo lectura: hay que leer bloque a bloque (`svcQueryMemory`).
- Técnicas en el registro de memoria como en la partida: +0x38 (u8 en el IE1; u16 en IE2/IE3 con sus
  niveles en +0x44). Comprobado en los tres juegos con los nombres de `command.dat`.
- Para buscarla, la EXP tiene que estar entre la de su nivel y la de dos niveles más; una vez encontrada,
  la cuenta no mira el tope (un jugador con EXP de más no deja sin ayudas a los de detrás).
- **En los partidos, el IE1 y el IE2 ponen detrás de tu plantilla, en la misma tabla, las fichas de los
  rivales** (y de quien juega contigo sin ser del equipo, como Nathan en el primer duelo del IE1), y ahí se
  quedan después. Antes del arreglo, el plugin las contaba como tuyas: la memoria del equipo apuntaba a los
  rivales y los «devolvía» a su nivel más alto, PE y PT llenos también para ellos, y «a tu nivel» los metía
  en la media. IE2: la ficha de partido es la misma que la de la plantilla (cada 0xFC) y la de los rivales
  lleva un 1 en +0x56 (medido: 18 tuyos y detrás 4 rivales del duelo del Raimon). IE1: sin marca, pero cada
  ficha lleva su puesto en +0x7A y el número de jugadores de tu plantilla va 0x20 bytes antes de la tabla
  (medido: 6, los de la partida, con Nathan y seis rivales en los puestos 6 a 12). IE3 y Ogro: la plantilla
  es la copia de la partida (cada 0x6C), donde el juego no pone rivales.

## Contenido de conexión (por estudiar)

`inazuma3/data_iz/logic/premiumcondition.dat` (condiciones por número de jugador, con equipos como «SP
Fixers»), `premium_item.dat`, `item_pw_adhoc.dat`, `MLPass.bin`, `phone_password.txt` y `ScoutSpCode.dat`
(códigos de fichaje por contraseña).
