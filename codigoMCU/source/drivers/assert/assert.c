/* app_assert.c - provide minimal __assert_func to satisfy drivers */
#include <stdint.h>

void __assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
    /* Puedes hacer debugging aquí: colocar un breakpoint, o reiniciar, o enviar por UART. */
    (void)file; (void)line; (void)func; (void)failedexpr;
    /* loop infinito para atraparlo con el debugger */
    while (1) { __asm__("bkpt #0"); }
}
