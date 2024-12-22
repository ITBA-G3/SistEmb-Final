# SistEmb-Final
Trabajo Práctico de la materia 25.27 - Sistemas Embebidos

# Cosas para hacer
- buscar la forma de leer una microSD (en el user manual de la freedom están detalladas las señales, pero el uso va a depender de cómo está formateada la memoria).
- Investigar sobre modos de bajo consumo en kinetis (más que nada cómo setearlos y cómo salir de ellos).
- Ver cómo reproducir audio (en la consigna recomiendan I2S, pero la placa decodificadora sale $30k).
- Hacer drivers para los displays que usemos (hay hechas, si se complica hacer usamos las que existen).
- Investigar sobre ecualización (cómo hacerla en la kinetis, si aplicamos filtros digitales o hacemos otra cosa).
- Teniendo en cuenta la información "adicional", hay que hacer "bien" el driver de I2C, porque el nuestro no admite errores, si ocurre un error, no lo detectamos ni hacemos nada.

# información "adicional"
Dejé en la carpeta de drivers el del encoder por si lo llegamos a usar como interfaz de usuario, para cambiar volúmen y navegar entre archivos (también podríamos usar botones).
También, hay dos carpetas de drivers de display, porque el salteño tiene el display LCD de dos líneas de caracteres, y yo tengo una pantallita oled de 1 pulgada, podemos usar la que más les guste, ambas son I2C.

# Por ahora es todo lo que se me ocurre, gracias por su atención