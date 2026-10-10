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

## Tablas

- `unitbase` (IE1/IE2 96 B, IE3 104 B), `unitstat` (64 / 80 / 72 B; el del IE3, ofuscado como `item.dat`),
  `unitno` (u16 por ficha, en orden estricto, las libres al final con 0), `lvup_exp` (98 u32 por curva).
- Bloque de identidad (IE1/IE2 en +64, IE3 en +76): curva de EXP, colegio, caras (+2, +4, +10), modelo
  (+8), posición (IE2 +21), elemento (IE2 +26) e índice de la descripción en `unitbase.STR` (u16 × 32,
  último campo de la ficha).
- Las caras de Shawn (0x919, 0x921, 0x922) van atadas a su cambio de forma (`unitchange.dat`): cualquier
  ficha con ellas sale como Shawn defensa. Aiden usa la cara 0x51B (el retrato de Atsuya), que existe en
  IE2 e IE3.
- Técnicas: IE2 `command.dat` de 28 B (nombre en +20, × 32 en `command.STR`); IE3 de 36 B (nombre en +24).
  Los números de técnica coinciden en IE2 e IE3.
- Títulos de equipo del IE2 (`rpgtitle.STR`): la caja tiene 14 casillas.
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

## Plantilla en memoria

- Su dirección cambia entre sesiones (la traducción cambia el tamaño de lo que se carga antes): se busca
  por su forma. IE1 cada 0x188 (PE/PT máximos en +0xAC), IE2 cada 0xFC (+0x70), IE3 y Ogro cada 0x6C con
  los máximos aparte (inicio + 0xCA6C + i × 0x98). Los actuales, en el registro (+0x34).
- El partido gasta los PT del propio registro (no hay copia).
- Pasión y Amistad: IE1 plantilla − 0xCE4, IE2 plantilla − 0xF0C; en el IE3 van en otro bloque.
- Dentro del heap hay código de los CRO en solo lectura: hay que leer bloque a bloque (`svcQueryMemory`).

## Contenido de conexión (por estudiar)

`inazuma3/data_iz/logic/premiumcondition.dat` (condiciones por número de jugador, con equipos como «SP
Fixers»), `premium_item.dat`, `item_pw_adhoc.dat`, `MLPass.bin`, `phone_password.txt` y `ScoutSpCode.dat`
(códigos de fichaje por contraseña).
