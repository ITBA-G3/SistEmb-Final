# SistEmb-Final
## Trabajo Práctico de la materia 25.27 - Sistemas Embebidos

Documentar todos los drivers $\to$ en caso de usar driver externos hacer un resumen de comon funciona internamente.

## Drivers necesarios

### Lectura SD

> Sulli
#### Actualizaciones de funcionamiento.
- La FRDM cuenta con un pin para detectar si hay una tarjeta puesta. Tiene un comportamiento errático, según el datasheet oficial NXP, se recomienda una R de pull-down externa porque la función pull-down interna de GPIO "es muy fuerte" y no permite transiciones. Lo que se observa es que sin pull-down el switch sólo pasa de bajo a alto una vez, y queda así hasta que reinicies el programa. Así que si sacás la SD, el programa va a seguir detectando que la tiene puesta.

### Decodificacion de archivo extraido de SD

> Lucha

Puedde ser que necesitemos una decodificacion previa.
Sacar señal de audio, metadata, etc.

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
