/*
 * FFT.c
 *
 *  Created on: 15 Dec 2025
 *      Author: lucia
 */


#include "FFT.h"
#include <math.h>
#include <string.h>


static uint16_t g_bin_edges[9];

typedef struct { float re, im; } cplx_t;

static float g_window[FFT_N];
static uint16_t g_bitrev[FFT_N];
static cplx_t g_twiddle[FFT_N / 2];

static inline cplx_t c_add(cplx_t a, cplx_t b){ return (cplx_t){a.re+b.re, a.im+b.im}; }
static inline cplx_t c_sub(cplx_t a, cplx_t b){ return (cplx_t){a.re-b.re, a.im-b.im}; }
static inline cplx_t c_mul(cplx_t a, cplx_t b){
    return (cplx_t){a.re*b.re - a.im*b.im, a.re*b.im + a.im*b.re};
}

static uint16_t bit_reverse(uint16_t x, uint16_t bits)
{
    uint16_t r = 0;
    for (uint16_t i = 0; i < bits; i++) {
        r = (r << 1) | (x & 1u);
        x >>= 1;
    }
    return r;
}

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


void FFT_Init(void)
{

    // Hann window
    for (uint32_t n = 0; n < FFT_N; n++) {
        g_window[n] = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * (float)n / (float)(FFT_N - 1u));
    }

    // bit reversal table
    uint16_t bits = 0;
    while ((1u << bits) < FFT_N) bits++;
    for (uint32_t i = 0; i < FFT_N; i++) {
        g_bitrev[i] = bit_reverse((uint16_t)i, bits);
    }

    // twiddles W_N^k = exp(-j 2pi k / N)
    for (uint32_t k = 0; k < FFT_N/2; k++) {
        float ang = -2.0f * (float)M_PI * (float)k / (float)FFT_N;
        g_twiddle[k].re = cosf(ang);
        g_twiddle[k].im = sinf(ang);
    }

    compute_bin_edges(AUDIO_FS_HZ, FFT_N, g_bin_edges);
}

static void fft_inplace(cplx_t *x)
{
    // bit-reversal reorder
    for (uint32_t i = 0; i < FFT_N; i++) {
        uint32_t j = g_bitrev[i];
        if (j > i) {
            cplx_t tmp = x[i];
            x[i] = x[j];
            x[j] = tmp;
        }
    }

    // iterative radix-2 Cooley-Tukey
    for (uint32_t len = 2; len <= FFT_N; len <<= 1) {
        uint32_t half = len >> 1;
        uint32_t step = FFT_N / len;

        for (uint32_t i = 0; i < FFT_N; i += len) {
            for (uint32_t k = 0; k < half; k++) {
                cplx_t t = c_mul(g_twiddle[k * step], x[i + k + half]);
                cplx_t u = x[i + k];
                x[i + k]        = c_add(u, t);
                x[i + k + half] = c_sub(u, t);
            }
        }
    }
}

void FFT_ComputeBands(const int16_t *pcm, size_t n, uint32_t fs_hz, float out_bands[8])
{
    (void)fs_hz;

    if (!pcm || n < FFT_N) {
        for (int i = 0; i < 8; i++) out_bands[i] = 0.0f;
        return;
    }

    // Build complex input with window, normalize int16 to [-1..1]
    static cplx_t X[FFT_N];

    // Substract mean before windowing
	// Build complex input with window, normalize int16 to [-1..1]
	// Subtract mean before windowing
	int32_t sum = 0;
	for (uint32_t i = 0; i < FFT_N; i++) sum += pcm[i];
	float mean = (float)sum / (float)FFT_N;

	for (uint32_t i = 0; i < FFT_N; i++) {
		float s = ((float)pcm[i] - mean) / 32768.0f;
		X[i].re = s * g_window[i];
		X[i].im = 0.0f;
	}


    fft_inplace(X);

    // Magnitude^2 for bins 0..N/2
    static float mag2[FFT_N/2 + 1];
    for (uint32_t k = 0; k <= FFT_N/2; k++) {
        mag2[k] = X[k].re * X[k].re + X[k].im * X[k].im;
    }

    // Integrate fixed (static) bin bands
    float bands[8] = {0};
    for (int b = 0; b < 8; b++) {
        uint32_t k0 = g_bin_edges[b];
        uint32_t k1 = g_bin_edges[b + 1];
        if (k1 <= k0) k1 = k0 + 1;
        if (k0 < 1) k0 = 1;                 // skip DC
        if (k1 > FFT_N/2) k1 = FFT_N/2;      // clamp to Nyquist

        if (k1 > k0 + 2) { k0 += 1; k1 -= 1; }  // guard band

        float acc = 0.0f;
        for (uint32_t k = k0; k < k1; k++) acc += mag2[k];

        bands[b] = acc / (float)(k1 - k0);
    }

    const float eps = 1e-20f;

    // TODO: db scale in dBFS check scale change to dbV? maybe.
    const float db_min = -50.0f;

    // 1*sinf(f) --> db = -17.09
    const float db_max = 0.0f;

    for (int b = 0; b < 8; b++) {

        // normalize power so amplitude matters
        float p_norm = bands[b] / ((float)FFT_N * (float)FFT_N);

        float db = 10.0f * log10f(p_norm + eps);

        float e = (db - db_min) / (db_max - db_min);
        if (e < 0.0f) e = 0.0f;
        if (e > 1.0f) e = 1.0f;

        if (e < 0.12f) e = 0.0f;
        out_bands[b] = e;
    }

}
