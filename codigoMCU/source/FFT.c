/**
 * @file FFT.c
 * @brief FFT-based spectrum analysis module implementation.
 *
 * This file contains the implementation of an FFT-based spectrum analyzer
 * used to extract an 8-band spectral representation from PCM audio samples.
 * The implementation is optimized for Cortex-M4F microcontrollers using
 * the CMSIS-DSP library (arm_rfft_fast_f32).
 *
 * The processing pipeline implemented in this file includes:
 * - DC removal via mean subtraction
 * - Hann windowing
 * - Real FFT computation using CMSIS-DSP
 * - Magnitude-squared spectrum calculation
 * - Integration over 8 fixed frequency bands
 * - Logarithmic (dB) scaling and normalization for visualization
 *
 * This module is intended to be used in real-time audio visualization
 * applications such as VU meters or spectrum displays.
 *
 * @author   Grupo 3
 *           - Ezequiel Díaz Guzmán
 *           - José Iván Hertter
 *           - Cristian Damián Meichtry
 *           - Lucía Inés Ruiz
 */

#include "FFT.h"
#include <math.h>
#include <string.h>

#include "arm_math.h"	// CMSIS-DSP


#if (FFT_N < 8)
#error "FFT_N too small"
#endif

#if ((FFT_N & (FFT_N - 1)) != 0)
#error "FFT_N must be a power of 2"
#endif


static uint16_t g_bin_edges[9];
static float    g_window[FFT_N];

static arm_rfft_fast_instance_f32 g_rfft;
static uint8_t  g_init_done = 0;

static float g_time[FFT_N];          // real time-domain input
static float g_freq[FFT_N];          // packed RFFT output
static float g_mag2[FFT_N/2 + 1];    // magnitude^2 for bins 0..N/2


/**
 * @brief Compute FFT bin indices corresponding to fixed band edge frequencies.
 *
 * Converts a set of fixed frequency edges (in Hz) into FFT bin indices given the
 * sampling frequency and FFT length. The edges are clamped to the valid range
 * (skipping DC and limiting to Nyquist) and forced to be strictly increasing.
 *
 * Band edges (Hz): 60, 120, 250, 500, 1000, 2000, 4000, 8000, 16000
 *
 * @param[in]  Fs         Sampling frequency in Hz.
 * @param[in]  N          FFT length (must match FFT_N).
 * @param[out] bin_edges  Output array of 9 bin edges (indices in [1 .. N/2]).
 */
static void compute_bin_edges(uint32_t Fs, uint32_t N, uint16_t bin_edges[9]) {
    const float edges_hz[9] = {60,120,250,500,1000,2000,4000,8000,16000};

    for (int i = 0; i < 9; i++) {
        float f = edges_hz[i];
        uint32_t k = (uint32_t)( (f * (float)N / (float)Fs) + 0.5f ); // round
        if (k < 1) k = 1;               // skip DC
        if (k > N/2) k = N/2;           // clamp to Nyquist
        bin_edges[i] = (uint16_t)k;
    }

    // Make sure bin edges are strictly increasing
    for (int i = 1; i < 9; i++) {
        if (bin_edges[i] <= bin_edges[i-1]) {
            bin_edges[i] = bin_edges[i-1] + 1;
            if (bin_edges[i] > N/2) bin_edges[i] = N/2;
        }
    }
}


/**
 * @brief Initialize the FFT module.
 *
 * Performs one-time initialization of:
 * - Hann window coefficients (length FFT_N)
 * - CMSIS-DSP real FFT instance (arm_rfft_fast_f32)
 * - Fixed bin edges used for the 8-band spectrum integration
 *
 * This function must be called once before calling ::FFT_ComputeBands().
 */
void FFT_Init(void)
{
    // Hann window (one-time cost; fine to keep cosf here)
	const float two_pi = 2.0f * (float)M_PI;
	const float denom  = (float)(FFT_N - 1u);

	for (uint32_t n = 0; n < FFT_N; n++) {
	        g_window[n] = 0.5f - 0.5f * cosf(two_pi * (float)n / denom);
	}

    // Init CMSIS real FFT
    arm_rfft_fast_init_f32(&g_rfft, FFT_N);

    compute_bin_edges(AUDIO_FS_HZ, FFT_N, g_bin_edges);

    g_init_done = 1;
}


/**
 * @brief Compute 8 normalized spectrum band levels from PCM audio.
 *
 * Takes a block of PCM samples, removes DC (mean subtraction), applies a Hann
 * window, computes a real FFT using CMSIS-DSP, and integrates the resulting
 * magnitude-squared spectrum into 8 fixed frequency bands. Each band is then
 * mapped to a normalized [0..1] level using a dB scale and a simple noise gate.
 *
 * Output bands correspond to the 8 intervals defined by the 9 band edges:
 * [60..120], [120..250], [250..500], [500..1000], [1000..2000],
 * [2000..4000], [4000..8000], [8000..16000] (Hz), subject to Nyquist clamping.
 *
 * @param[in]  pcm        Pointer to PCM int16 samples.
 * @param[in]  n          Number of samples available in @p pcm.
 * @param[in]  fs_hz      Sampling frequency in Hz (currently unused; edges use AUDIO_FS_HZ).
 * @param[out] out_bands  Output array of 8 normalized band levels in [0..1].
 */
void FFT_ComputeBands(const int16_t *pcm, size_t n, uint32_t fs_hz, float out_bands[8])
{
    (void)fs_hz;

    if (!pcm || n < FFT_N) {
        for (int i = 0; i < 8; i++) out_bands[i] = 0.0f;
        return;
    }

    int32_t sum = 0;
    for (uint32_t i = 0; i < FFT_N; i++){
    	sum += pcm[i];
    }
    float mean = (float)sum / (float)FFT_N;

    const float inv_q15 = 1.0f / 32768.0f;

    for (uint32_t i = 0; i < FFT_N; i++) {
            float s = ((float)pcm[i] - mean) * inv_q15;
            g_time[i] = s * g_window[i];
    }

    arm_rfft_fast_f32(&g_rfft, g_time, g_freq, 0);

    g_mag2[0] = g_freq[0] * g_freq[0];
    g_mag2[FFT_N/2] = g_freq[1] * g_freq[1];

    for (uint32_t k = 1; k < FFT_N/2; k++) {
        float re = g_freq[2u * k];
        float im = g_freq[2u * k + 1u];
        g_mag2[k] = re * re + im * im;
    }

    float bands[8] = {0};

    for (int b = 0; b < 8; b++) {
        uint32_t k0 = g_bin_edges[b];
        uint32_t k1 = g_bin_edges[b + 1];
        if (k1 <= k0) k1 = k0 + 1;
        if (k0 < 1) k0 = 1;
        if (k1 > FFT_N/2) k1 = FFT_N/2;

        if (k1 > k0 + 2) { k0 += 1; k1 -= 1; }

        float acc = 0.0f;
        for (uint32_t k = k0; k < k1; k++) acc += g_mag2[k];
        bands[b] = acc;
    }

    const float eps = 1e-20f;
    const float db_min = -50.0f;
    const float db_max = 0.0f;

    const float inv_db_span = 1.0f / (db_max - db_min);
    const float inv_N2 = 1.0f / ((float)FFT_N * (float)FFT_N);

    for (int b = 0; b < 8; b++) {
            float p_norm = bands[b] * inv_N2;
            float db = 10.0f * log10f(p_norm + eps);
            float e = (db - db_min) * inv_db_span;
            if (e < 0.0f) e = 0.0f;
            if (e > 1.0f) e = 1.0f;
            if (e < 0.12f) e = 0.0f;
            out_bands[b] = e;
    }
}

