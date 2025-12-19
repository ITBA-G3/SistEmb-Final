/**
 * @file     Visualizer.h
 * @brief    LED matrix audio visualizer interface.
 *
 * This file declares the public interface for the LED matrix visualizer:
 * - Functions to draw bar-based visualizations
 * - Test-frame generation for development and debugging
 *
 * @author   Grupo 3
 *           - Ezequiel Díaz Guzmán
 *           - José Iván Hertter
 *           - Cristian Damián Meichtry
 *           - Lucía Inés Ruiz
 */

#ifndef VISUALIZER_H_
#define VISUALIZER_H_

#include "LEDmatrix.h"

/**
 * @brief Generates and renders a test visualization frame.
 *
 * Produces synthetic band-energy data (phase-shifted sine waves) and
 * draws the corresponding bar visualization on the LED matrix.
 * Intended for testing the visualizer without real FFT input.
 *
 * @param matrix Pointer to the LED matrix instance to render into.
 */
void Visualizer_UpdateFrame(LEDM_t* matrix);

/**
 * @brief Renders an 8-band vertical bar visualization on the LED matrix.
 *
 * Each band represents a normalized energy value and is displayed as a
 * bottom-up bar with a green–yellow–red color gradient.
 *
 * @param band_energy Array of 8 normalized energy values (0.0..1.0).
 * @param matrix Pointer to the LED matrix instance to render into.
 */
void Visualizer_DrawBars(const float band_energy[8], LEDM_t* matrix);

#endif /* VISUALIZER_H_ */
