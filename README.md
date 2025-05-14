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

### Display 

> TBD

- Driver para poder usar la matriz de leds.
- Hacer el vúmetro.

### I2S & reproduccion de audio

> Eze

- I2C pero mejorado para transmitir audio.
- Necesitamos un módulo.

### Driver user input (control de audio)

> Cris

- Ya está implementado el driver del encoder $\to$ revisarlo de todas formas.
- Si es necesario implementar un driver o interrupcion para botones.

## FSM

Usar la implemetación de dany para la fsm.

## información "adicional"
Dejé en la carpeta de drivers el del encoder por si lo llegamos a usar como interfaz de usuario, para cambiar volúmen y navegar entre archivos (también podríamos usar botones).
También, hay dos carpetas de drivers de display, porque el salteño tiene el display LCD de dos líneas de caracteres, y yo tengo una pantallita oled de 1 pulgada, podemos usar la que más les guste, ambas son I2C.

- Investigar sobre ecualización (cómo hacerla en la kinetis, si aplicamos filtros digitales o hacemos otra cosa).