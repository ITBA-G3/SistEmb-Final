/**
 * @file     Visualizer.c
 * @brief    LED matrix audio visualizer implementation.
 *
 * This file contains the implementation of the LED matrix visualizer:
 * - Mapping of normalized band energies to bar heights
 * - Color gradient generation (green → yellow → red)
 * - Rendering of vertical bar graphs on the LED matrix
 * - Synthetic test frame generation for visualization testing
 *
 * @author   Grupo 3
 *           - Ezequiel Díaz Guzmán
 *           - José Iván Hertter
 *           - Cristian Damián Meichtry
 *           - Lucía Inés Ruiz
 */

#include "Visualizer.h"
#include <math.h>

/**
 * @brief Returns a color for a given bar "level" within a column.
 *
 * Produces a vertical gradient:
 * - Lower levels: green -> yellow
 * - Upper levels: yellow -> red
 *
 * @param level Current level index (0 = bottom).
 * @param max_level Total number of levels (LEDM_MAX_HEIGHT).
 * @return RGB color corresponding to the given level.
 */
static LEDM_color_t color_for_level(int level, int max_level) {
    float t = (float)level / (float)(max_level - 1);   // 0..1

    LEDM_color_t c;

    if (t < 0.5f) {
        // de verde a amarillo
        float u = t / 0.5f;     
        c.r = (uint8_t)(u * 255);
        c.g = 255;
        c.b = 0;
    } else {
        // de amarillo a rojo
        float u = (t - 0.5f) / 0.5f; 
        c.r = 255;
        c.g = (uint8_t)((1.0f - u) * 255); 
        c.b = 0;
    }

    return c;
}

/**
 * @brief Converts normalized energy (0.0 to 1.0) into a bar height in pixels.
 *
 * Clamps energy to [0,1] and maps to [0, LEDM_MAX_HEIGHT] with rounding
 * to help low energies still produce visible bars.
 *
 * @param e Normalized energy value (expected range 0.0 to 1.0).
 * @return Bar height in pixels (0..LEDM_MAX_HEIGHT).
 */
static int energy_to_height(float e) {
    if (e < 0.0f) e = 0.0f;
    if (e > 1.0f) e = 1.0f;
    // redondeo hacia arriba para que se vea algo con energías chicas
    int h = (int)(e * (float)LEDM_MAX_HEIGHT + 0.5f);
    if (h > LEDM_MAX_HEIGHT) h = LEDM_MAX_HEIGHT;
    return h;
}

/**
 * @brief Draws an 8-band vertical bar visualization onto an LED matrix.
 *
 * For each band (x = 0..7), computes a height from normalized energy and
 * fills pixels from bottom to top with a green->yellow->red gradient.
 * Pixels above the height are cleared (set to black).
 *
 * @param band_energy Array of 8 normalized energy values (each ideally 0.0..1.0).
 * @param matrix Pointer to the LED matrix instance to draw into.
 */
void Visualizer_DrawBars(const float band_energy[8], LEDM_t* matrix) {
	for (int x = 0; x < 8; x++) {
        int height = energy_to_height(band_energy[x]); // 0..8

        for (int y = 0; y < 8; y++) {
            if (y < height) {
            	LEDM_SetPixel(matrix, 7-y, x, color_for_level(y, LEDM_MAX_HEIGHT));	// de abajo hacia arriba
            }
            else {
                LEDM_SetPixel(matrix, 7-y, x, (LEDM_color_t){0,0,0});
            }
        }
    }
}
