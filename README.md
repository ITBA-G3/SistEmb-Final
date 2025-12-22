# SistEmb-Final

**Trabajo Práctico Final — Sistemas Embebidos (25.27)**  
Instituto Tecnológico de Buenos Aires (ITBA)

Repositorio del proyecto final de la materia **Sistemas Embebidos**, que implementa un **reproductor de audio embebido** sobre una MCU, con manejo de periféricos reales, multitarea con RTOS e interfaz de usuario.

## Descripción general

El proyecto consiste en el desarrollo de un **reproductor de música embebido**, capaz de leer archivos desde una **tarjeta SD**, reproducir audio y permitir la interacción del usuario mediante **encoder rotatorio, botones y display I2C**.

La lógica principal está organizada como una **máquina de estados**, ejecutada dentro de una *task* del sistema operativo en tiempo real.

##  Funcionalidades principales

- Lectura de archivos de audio desde **SD Card**
- Reproducción de audio mediante periféricos dedicados
- Control de reproducción:
  - Play
  - Pause
  - Cambio de canción
- Interfaz de usuario:
  - Encoder rotatorio para navegación
  - Botones de control
  - Display I2C para mostrar estado e información
- Arquitectura multitarea basada en **RTOS (µC/OS)**

##  Máquina de estados

La *task* principal implementa una **FSM (Finite State Machine)** con tres estados:

- **MENU**
  - Navegación entre canciones mediante el encoder
  - Selección de canción
- **PLAY**
  - Reproducción activa
  - Cambio de canción sin salir del estado
  - Opción de pausar o volver al menú
- **PAUSE**
  - Reproducción pausada
  - Reanudar reproducción
  - Volver al menú

## Autores

- Diaz Guzmán, Ezequiel.
- Hertter, José Iván.
- Ruíz, Lucía Inés.
- Meichtry, Cristian Damián.
