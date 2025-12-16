/*
 * Visualizer.h
 *
 *  Created on: 2 Dec 2025
 *      Author: lucia
 */

#ifndef VISUALIZER_H_
#define VISUALIZER_H_

#include "LEDmatrix.h"

void Visualizer_UpdateFrame(LEDM_t* matrix);
void Visualizer_DrawBars(const float band_energy[8], LEDM_t* matrix);

#endif /* VISUALIZER_H_ */
