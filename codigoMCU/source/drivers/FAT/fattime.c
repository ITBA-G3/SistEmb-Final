#include "ff.h"

/*
 * Formato FatFs:
 * bit31:25  Year from 1980 (0..127)
 * bit24:21  Month (1..12)
 * bit20:16  Day (1..31)
 * bit15:11  Hour (0..23)
 * bit10:5   Minute (0..59)
 * bit4:0    Second / 2 (0..29)
 */
DWORD get_fattime(void)
{
    return ((DWORD)(2025 - 1980) << 25) |  // Año 2024
           ((DWORD)12  << 21) |             // Enero
           ((DWORD)20  << 16) |             // Día 1
           ((DWORD)15  << 11) |             // Hora 00
           ((DWORD)23  << 5)  |             // Minuto 00
           ((DWORD)03  >> 1);               // Segundo 00
}
