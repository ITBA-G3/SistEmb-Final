#include "Visualizer.h"
#include <math.h>

static LEDM_color_t color_for_level(int level, int max_level) {
    // level: 0 abajo, max_level-1 arriba
    float t = (float)level / (float)(max_level - 1);   // 0..1

    LEDM_color_t c;

    if (t < 0.5f) {
        // de verde a amarillo
        float u = t / 0.5f;     // 0..1
        c.r = (uint8_t)(u * 255);   // 0 -> 255
        c.g = 255;
        c.b = 0;
    } else {
        // de amarillo a rojo
        float u = (t - 0.5f) / 0.5f; // 0..1
        c.r = 255;
        c.g = (uint8_t)((1.0f - u) * 255); // 255 -> 0
        c.b = 0;
    }

    return c;
}


static int energy_to_height(float e) {
    if (e < 0.0f) e = 0.0f;
    if (e > 1.0f) e = 1.0f;
    // redondeo hacia arriba para que se vea algo con energías chicas
    int h = (int)(e * (float)LEDM_MAX_HEIGHT + 0.5f);
    if (h > LEDM_MAX_HEIGHT) h = LEDM_MAX_HEIGHT;
    return h;   // 0..8
}


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

void Visualizer_UpdateFrame(LEDM_t* matrix) {
    static float t = 0.0f;
    float bands[8];

    for (int i = 0; i < 8; i++) {
    	// sinuisodales defasadas para test
        float phase = t + i * 0.7f;
        float val = 0.5f + 0.5f * sinf(phase);
        bands[i] = val;
    }

    t += 0.25f; // velocidad del movimiento

    Visualizer_DrawBars(bands, matrix);
}
