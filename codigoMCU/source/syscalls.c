#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

// Reemplazá esto por tu propia función para enviar un caracter por UART
extern void UART_PutChar(char c);

int _write(int file, char *ptr, int len) {
//    for (int i = 0; i < len; i++) {
//        UART_PutChar(ptr[i]);  // Usá tu driver UART aquí
//    }
    return 0;
}

int _read(int file, char *ptr, int len) {
    // No implementado, devolver 0
    return 0;
}

int _close(int file) {
    return -1;
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _lseek(int file, int ptr, int dir) {
    return 0;
}

int _isatty(int file) {
    return 1;
}

int _kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}

int _getpid(void) {
    return 1;
}

void _exit(int status) {
    while (1);  // No hay OS, así que quedamos en loop
}
