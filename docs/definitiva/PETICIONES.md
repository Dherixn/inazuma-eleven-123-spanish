# Qué pide la comunidad y cómo lo cubre la edición definitiva

Investigación del 2026-10-09 en Reddit (índice de r/inazumaeleven en archive.org, unos 34 500 hilos),
InazumaLife, GBAtemp, la biblioteca IE MODS LIBRARY (inazuma-eleven.fr/mods), los wikis inglés y
japonés y la prensa. Las fuentes de cada dato están en los enlaces. Orden: de más a menos repetido.

| # | Petición | Respaldo | Qué haremos | Cómo | Incidencia | Estado (10-oct) |
|---|---|---|---|---|---|---|
| 1 | **Contenido de descargas y Wi-Fi perdido.** En el recopilatorio, lo que en DS iba por Wi-Fi se desbloquea con **45 encuentros StreetPass**, imposibles en emulador | ~30 hilos de Reddit, InazumaLife | Desbloquearlo jugando, sin StreetPass | contador de StreetPass en la partida (plugin) o umbral en datos | #8, #10 | **Hecho**: «Contenido de StreetPass y Wi-Fi» pone a 45 el contador de `Menu.sav` y el juego lo desbloquea todo; los 15 objetos del IE3 y el Ogro, en español |
| 2 | **Farmeo y fichajes difíciles** (350 pachangas para Nakata, huir de encuentros cuesta amistad, tope de 100 jugadores) | 17 hilos sobre NoFarmForMe, prensa | Ayudas opcionales: EXP ×2/×3, fichajes más baratos, `SEARCH_LV*` | datos + menú del plugin | #7 | En buena parte: EXP ×2/×3, Pasión y Amistad ×2/×3, PE y PT llenos, encuentros, «Fichaje libre» (cualquier jugador, al nivel que elijas) y «Supertécnicas a la carta» (como en Tempête de Feu+, todas las técnicas al alcance) |
| 3 | **Encuentros aleatorios** («frecuencia criminal»; el accesorio que los evita llega al final) | 33 comentarios en un hilo, Game Informer, Nintendo Life | Frecuencia elegible: normal, la mitad o ninguno | `RPG_ENCOUNT_TIME` + menú | #7 | **Hecho**: ayuda «Encuentros aleatorios» (normal, menos, muy pocos, ninguno, más) |
| 4 | **Dificultad demasiado baja** | 6 hilos; los mods más vistos son de dificultad (IE Souls 3, 13 314 visitas) | Selector: normal, difícil, extremo | niveles de `team.pkb` y `CPU_USE_SPECIAL_PERCENT` | #11 | Hecho: «Nivel de los rivales» (más fáciles, más duros, muy duros y «a tu nivel», sobre `games.dat` en memoria; medido en Azahar), supertécnicas del rival y penaltis de la CPU |
| 5 | **Exclusivos de versión** (IE2: Prominence y Diamond Dust, ~100 fichajes por versión; IE3: ~98 por versión y 10 manuales) | 5 fuentes; piden un mod como el de Yo-kai Watch 2 | Una sola versión con todo | listas `_F`/`_B` y ramas de los scripts | #4, #5 | **Hecho**: ayuda «Versión» (`IZ_TYPE`): con una partida, los exclusivos de las dos |
| 6 | **Un solo juego con sistemas comunes** («esperaba que el 1·2·3 fuera un único juego») | 4 hilos | Es el objetivo: continuidad y enlace de historias | traspaso + plugin | #2, #9 | **Hecho**: continuidad IE1 → IE2 → IE3 → Ogro con la memoria del equipo, textos puente y crónica |
| 7 | **Equilibrio** (inflación de PT en IE3, Edgar con afinidad Bosque, la Espada Meteoro de Hiroto) | 3 fuentes, Shin Ogre | Revisión de técnicas y afinidades | `command.dat`, `unitstat.dat` | #12 | Pendiente |
| 8 | **Pérdidas irreversibles** (en IE3 Fubuki y Midorikawa vuelven tras Corea sin sus técnicas de manual) | wiki japonés | Que no se pierdan | scripts de evento / plugin | #13 | **Hecho**: memoria del equipo (vuelven como se fueron) y «Recuperar jugadores» |
| 9 | **Personajes que faltan**: Aiden Froste, Kazemaru del Dark Emperors, Nosaka, Haizaki | mods de Atsuya en IE2, Great Road of Heroes | Aiden primero; el resto después | fichas libres + gráficos | #6 | Aiden **hecho** (IE2, IE3 y Ogro); los demás, con «Fichaje libre» si tienen ficha jugable |

## Lo que ya existe y nos sirve

- **Super-Link (IE2 → IE3) y Ogre Premium Link funcionan dentro del recopilatorio con un solo cartucho**
  (wiki japonés, atwiki inazuma11-3 p. 70). El IE3 **ya lee la partida del IE2**: es el mejor punto de
  partida para la continuidad completa (#2). Super-Link da 47 fichajes premium (entre ellos **Fubuki en
  forma Atsuya**, con Fubuki y Someoka a nivel 50 o más) y 13 equipos.
- **Tempête de Feu+** (IE2, Darthris) ya hizo una «edición definitiva» del IE2 en DS: exclusivos y Wi-Fi
  conseguibles jugando, técnicas del IE3, rutas hasta el nivel 99 y desplazamiento más rápido. Confirma que
  todo eso es posible con datos.
- **IE Souls** demuestra que las dificultades elegibles son lo que más éxito tiene.
- **Contraseñas**: la web oficial publica las de 8 jugadores del IE2 y unas 24 del IE3. Además están en
  `ScoutSpCode.dat` de cada juego (8 por juego).

## Ideas para que apetezca jugar del 1 al 3

- **Heredar tu equipo** del IE1 al IE2 y al IE3 (continuidad, #2).
- **Medallas de veterano**: lo que consigues en un juego desbloquea en el siguiente el contenido que antes
  pedía Wi-Fi o StreetPass (#10).
- **Aiden Froste jugable** desde el IE2, cuando Shawn se reconcilia con su hermano (#6).
- **Ninguna pérdida irreversible**: tus técnicas de manual no se pierden por guion (#13).
- **Dificultad a elegir** desde el principio y para toda la partida (#11).

## Fuentes principales

- Reddit: hilo de encuentros aleatorios https://www.reddit.com/r/inazumaeleven/comments/172iozj/ ;
  «1 unique game» https://www.reddit.com/r/inazumaeleven/comments/j6loqz/ ; mod sin exclusividad
  https://www.reddit.com/r/inazumaeleven/comments/1mcp3xf/
- Biblioteca de mods: https://inazuma-eleven.fr/mods/api/mods?limit=24&offset=0 · Shin Ogre:
  https://inazuma-eleven.fr/mods/api/mods/inazuma-eleven-shin-ogre · Tempête de Feu+:
  https://inazuma-eleven.fr/mods/api/mods/tempete-de-feu · IE Souls: https://gbatemp.net/threads/inazuma-eleven-souls.681091/
- Recopilatorio: https://w.atwiki.jp/inazuma11-3/pages/70.html · contraseñas oficiales:
  https://www.inazuma.jp/inazuma123/popup_rendou.html
- Super-Link y Premium Link: https://inazuma-eleven.fandom.com/wiki/Super-Link
- Exclusivos: https://inazuma-eleven.fandom.com/wiki/Player_Binder_(Inazuma_Eleven_3)
- Atsuya: https://inazuma-eleven.fandom.com/wiki/Chara_Change
