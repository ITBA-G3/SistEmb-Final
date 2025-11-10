# SistEmb-Final
## Trabajo Práctico de la materia 25.27 - Sistemas Embebidos

Documentar todos los drivers $\to$ en caso de usar driver externos hacer un resumen de comon funciona internamente.

## Drivers necesarios

### Lectura SD

> Sulli

### Decodificacion de archivo extraido de SD

> Lucha

Puedde ser que necesitemos una decodificacion previa.
Sacar señal de audio, metadata, etc.
Recursos: 
- Wrapper para simplificar: https://github.com/Lefucjusz/Helix-MP3-Decoder
- Helix a secas (igual al subido a campus por dani): https://github.com/liuduanfei/helix?tab=readme-ov-file
- Appnote para entender helix: https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ApplicationNotes/ApplicationNotes/01367A.pdf
- Referencia "Embedded Audio Processing": https://www.analog.com/media/en/dsp-documentation/embedded-media-processing/embedded-media-processing-chapter5.pdf



### Display 

> to be determined (a determinar, en idioma español)

- Driver para poder usar la matriz de leds.

### I2S & reproduccion de audio

> Eze

- I2C pero mejorado para transmitir audio.
- Necesitamos un módulo.

### Driver user input (control de audio)

> Cris

- Ya está implementado el driver del encoder $\to$ revisarlo de todas formas.
- Si es necesario implementar un driver o interrupcion para botones.

- ver que onda el vumetro

## FSM

Usar la implemetación de dany para la fsm.

## información "adicional"
Dejé en la carpeta de drivers el del encoder por si lo llegamos a usar como interfaz de usuario, para cambiar volúmen y navegar entre archivos (también podríamos usar botones).
También, hay dos carpetas de drivers de display, porque el salteño tiene el display LCD de dos líneas de caracteres, y yo tengo una pantallita oled de 1 pulgada, podemos usar la que más les guste, ambas son I2C.

- Investigar sobre ecualización (cómo hacerla en la kinetis, si aplicamos filtros digitales o hacemos otra cosa).