/***************************************************************************/ /**
   @file     Spectrogram_test.c
   @brief
   @author   Grupo 3
  ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "LEDmatrix.h"
#include "drivers/PIT.h"
#include "Visualizer.h"
#include "FFT.h"
#include <math.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/
static LEDM_t *matrix;
static bool start_new_frame;

static void make_test_pcm(int16_t *pcm, uint32_t fs_hz);
static void PIT_cb(void);

/*******************************************************************************
 *******************************************************************************
						GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void App_Init(void)
{
//	LEDMATRIX TEST
	matrix = LEDM_Init(8, 8);
	PIT_Init(PIT_0, 10);		// PIT 0 para controlar FPS y refrescar matrix de leds,
	PIT_SetCallback(PIT_cb, PIT_0);
	FFT_Init();
}

void App_Run(void)
{
// LED MATRIX TEST
    if (!matrix) {
        while (1);
    }

    bool ok;
    static int16_t frame[FFT_N];
    static float bands[8];
    int nonzero=0;


    LEDM_SetBrightness(matrix, 8);

    if(start_new_frame){
    	start_new_frame = 0;

    	make_test_pcm(frame, AUDIO_FS_HZ);

    	FFT_ComputeBands(frame, FFT_N, AUDIO_FS_HZ, bands);

    	Visualizer_DrawBars(bands, matrix);
    	ok = LEDM_Show(matrix);
    	if(ok){
			while(LEDM_TransferInProgress());
		}
		LEDM_Clear(matrix);
    }
}


/*******************************************************************************
 *******************************************************************************
						LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void make_test_pcm(int16_t *pcm, uint32_t fs_hz)
{
    static float ph1 = 0.0f;
    static float ph2 = 0.0f;

    /* Pick one low-band tone + one mid/high-band tone.
       Examples for Fs=48k, N=1024 (bin spacing 46.875 Hz):
       - 281.25 Hz  (k=6)  -> band 250–500
       - 3000.0 Hz  (k=64) -> band 2000–4000
    */
    const float f1 = 281.25f;
    const float f2 = 3000.0f;

    const float inc1 = 2.0f * (float)M_PI * f1 / (float)fs_hz;
    const float inc2 = 2.0f * (float)M_PI * f2 / (float)fs_hz;

    /* Keep headroom: sum of two sines can reach 2.0.
       Using 0.35 + 0.35 keeps peak <= 0.70, no clipping.
     */
    const float a1 = 0.35f;
    const float a2 = 0.35f;

    for (uint32_t i = 0; i < FFT_N; i++) {
        float s = a1 * sinf(ph1) + a2 * sinf(ph2);

        // Optional hard clip (should never trigger with the amplitudes above)
        if (s >  1.0f) s =  1.0f;
        if (s < -1.0f) s = -1.0f;

        pcm[i] = (int16_t)(s * 32767.0f);

        ph1 += inc1; if (ph1 >= 2.0f*(float)M_PI) ph1 -= 2.0f*(float)M_PI;
        ph2 += inc2; if (ph2 >= 2.0f*(float)M_PI) ph2 -= 2.0f*(float)M_PI;
    }
}

static void PIT_cb(void){
	start_new_frame = true;
}


/*******************************************************************************
 ******************************************************************************/
