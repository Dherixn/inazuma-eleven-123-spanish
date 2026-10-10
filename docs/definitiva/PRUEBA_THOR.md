# Prueba larga en la AYN Thor (o cualquier Azahar)

Guía para jugar la edición definitiva de principio a fin y apuntar lo que falle. Cada paso dice qué hacer y qué
debería verse. Si algo no cuadra, apunta el paso, el juego, el momento de la historia y, si puedes, una captura.

## 0. Instalar

1. En el PC: `powershell -ExecutionPolicy Bypass -File tools/definitiva_thor.ps1` con la Thor conectada por USB
   (depuración USB activada y Azahar abierto una vez para que cree su carpeta). O copia a mano `load` y `sdmc`
   del paquete dentro de la carpeta de usuario de Azahar.
2. Azahar → Ajustes → Sistema → «3GX Plugin Loader» activado. El juego, sin la actualización 1.4.
3. Abre el juego. En el menú del recopilatorio, **Select** abre el menú de la edición definitiva.
   - Debería verse: carpetas Continuidad, Fichajes especiales, Ayudas y Partidas.

## 1. Inazuma Eleven (IE1)

1. Juega con normalidad. Prueba las Ayudas: encuentros («Ninguno» y «Más»), velocidad al andar, EXP ×2,
   Pasión y Amistad ×2, PE y PT siempre llenos.
   - Debería verse: los cambios al momento o al volver al mapa (lo dice cada ayuda).
2. Guarda a menudo. «Partidas» copia los archivos a la SD.

## 2. Del IE1 al IE2

1. Empieza el IE2 (Tormenta de Fuego o Ventisca Eterna) y guarda una vez.
2. Menú del recopilatorio → Select → Continuidad → «Traer mi equipo del IE1 a…» → modo (Completa, Equilibrada o
   Solo fichajes).
   - Debería verse: el texto «Capítulo siguiente» y «N llegarán con la historia, N van a la reserva y N mejoran».
3. Sal al título SIN GUARDAR y pulsa Continuar.
   - Debería verse: los del Raimon que ya estaban, con tu nivel del IE1; los que la historia no une, en la
     página 2 (reserva). «Fichajes especiales → Grupo y reserva» los pasa al grupo.
4. Prueba «Nivel de los rivales» (Ayudas; del IE2 en adelante): cuenta desde el siguiente partido, y en un
   duelo se nota enseguida en los atributos del rival. Con un equipo traído, «A tu nivel» evita paseos.
5. Juega. Cuando la historia una a alguien que traías (por ejemplo, Axel), debería llegar con su nivel del IE1
   en unos segundos (hace falta «Memoria del equipo» activada, que viene de fábrica).

## 3. Aiden Froste (IE2)

1. Cuando Shawn aprenda «Aullido de lobo»: Fichajes especiales → Aiden Froste.
   - Debería verse: Aiden con el retrato de Atsuya, en el grupo si cabe, con el nivel de Shawn.

## 4. Del IE2 al IE3 (Rayo Celeste o Fuego Explosivo)

1. Empieza el IE3, juega hasta poder guardar (prólogo de Paolo) y guarda.
2. Continuidad → «Traer mi equipo del IE2 a…».
3. Sal al título SIN GUARDAR y pulsa Continuar.
   - En el prólogo solo juega Paolo: los tuyos esperan en la reserva o llegan con la historia.
4. Tras el prólogo, Mark está solo un rato con el nivel de la historia (el plugin necesita 4 jugadores para
   actuar). En Balón Bazar la historia une a Nathan, Kevin y Jack.
   - Debería verse: Mark y ellos con el nivel que traían del IE2.
5. Tras el primer partido del IE3, mira `/Inazuma123Definitiva/equipo_*.txt`: solo debe haber jugadores
   tuyos (si aparece un rival, apúntalo: en el IE1 y el IE2 ya se evita). Y prueba ahí «Nivel de los rivales»
   (por ejemplo «Muy duros»): comprobado en un partido del IE2; en el IE3 la tabla está en memoria igual, pero
   falta verlo en un partido (si los rivales no cambian, apúntalo).
6. **Momento clave: la selección de Inazuma Japón.** Apunta quién queda en el grupo y en la reserva, y si alguien
   sale repetido o con nivel 1 cuando traía más. Los que la historia descarta (Kevin, Max, David…) se pueden
   traer después con «Fichajes especiales → Recuperar jugadores».
7. Tras el partido contra Corea, Shawn vuelve: debería volver con su nivel y sus técnicas.

## 5. Del IE3 a La amenaza del Ogro

1. Empieza el Ogro (prólogo de Canon), guarda y usa «Traer mi equipo del IE3 a La amenaza del Ogro».
   - Debería verse lo mismo que en el IE3: la historia une a los tuyos y llegan con lo suyo.

## 6. Extras

- **Contenido de StreetPass y Wi-Fi** (Fichajes especiales): el juego se cierra; al abrirlo, el menú del
  recopilatorio da 27 avisos y «Has desbloqueado todos los personajes y objetos». Comprueba, en su capítulo, que
  esos jugadores y objetos aparecen.
- **Fichaje libre** y **Supertécnicas a la carta**: busca por nombre y prueba en la partida.
- **Crónica de la leyenda** (Continuidad): tus temporadas y horas.
- Hay secretos. Pista: el abuelo de Mark era muy aficionado a los videojuegos de los ochenta.

## Qué apuntar si algo falla

- Juego y versión, momento de la historia y paso de esta guía.
- Qué esperabas y qué salió (captura si se puede).
- Si fue tras un traspaso: el modo elegido.
- Los archivos de `/Inazuma123Definitiva/` de la SD (partidas copiadas y `equipo_*.txt`) ayudan mucho.
