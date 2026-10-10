# Secretos de la edición definitiva

**Destripes.** Esto es para quien mantenga el proyecto: qué hay escondido, dónde está y cómo probarlo.
Para los jugadores, el README solo dice «dicen que el abuelo Daisuke dejó un cuaderno...».

Todo vive en el plugin (`plugin/Sources/Secretos.cpp`, salvo donde se indica) y se recuerda en la SD
(`/Inazuma123Definitiva/secretos.txt`).

## El Cuaderno del abuelo

- **Cómo se encuentra:** con el juego en marcha, el código Konami con la cruceta: ↑ ↑ ↓ ↓ ← → ← → B A. Sale
  «¡Has despertado el Cuaderno del abuelo!» y aparece la carpeta en el menú del plugin (Select). Si ya estaba
  despierto, el código enseña una frase del abuelo.
- **Páginas:** el fútbol, tres temporadas (la continuidad), los hermanos del norte (pista de cómo se ficha a
  Aiden) y la última (anuncia al Ogro); y los créditos.
- **Escribir en el cuaderno** (las palabras no distinguen mayúsculas ni signos):

| Palabra | Qué pasa |
|---|---|
| RAIMON | Una vez por SD, con una partida del IE1 o del IE2 cargada: +500 de Pasión y +50 de Amistad («sobre del club»). |
| CANON | Canon Evans, el bisnieto de Mark, llega del futuro a la partida que elijas (IE2, IE3 u Ogro) con el nivel de Mark, tras una carta «con matasellos de dentro de ochenta años» (`Fichajes.cpp`, `FicharACanon`). |
| LEYENDA | La última hoja del cuaderno: un mensaje firmado por Daisuke y Mark. La Crónica lo anuncia al completar las cuatro sagas. |
| AIDEN, ATSUYA | Una nota de Aiden a su hermano. |
| MANOCELESTIAL, GODHAND | El dibujo de la Mano celestial. |
| DAISUKE, ABUELO | Un mensaje del abuelo. |
| OGRO, OGRE | Una página rasgada sobre el futuro. |
| LUIS | Las gracias al traductor. |
| INAZUMA, INAZUMAELEVEN | Una frase del abuelo. |

## La leyenda completa

«Crónica de la leyenda» (menú Continuidad, `Cronica.cpp`) suma las horas de todas tus partidas. Con las cuatro
sagas empezadas (IE1, IE2 en cualquier versión, IE3 en cualquier versión y Ogro) cierra con «LA LEYENDA
COMPLETA... el juego más largo de la historia de Inazuma Eleven» y te manda a escribir «LEYENDA» en el cuaderno.

## Fechas

Unos segundos después de arrancar, si es 22 de agosto (salida del primer Inazuma Eleven, 2008), 1 de enero o
25 de diciembre, sale un mensaje de la fecha.

## De regalo

Tras el prólogo del IE3 la historia da de baja a Paolo Bianchi y a Héctor Helio (y a Canon tras el del Ogro).
Si jugaste con la memoria del equipo activa, «Recuperar jugadores» los puede traer a tu equipo.
