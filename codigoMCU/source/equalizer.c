#include "equalizer.h"
#include <arm_math.h>
#include "drivers/gpio.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define BANDS_QUANT 4
static uint8_t bands_gain[BANDS_QUANT];

#define MAX_LENGTH_NAME 10

typedef struct {
    char name[MAX_LENGTH_NAME];
    uint8_t equalization[BANDS_QUANT];
} MusicalGenre_t;

#define GENRES_QUANT 6
MusicalGenre_t genres_array[GENRES_QUANT];

static float32_t Bf[4] = {100, 100, 100, 100};// Bandwidth [Hz]
static float32_t GB[4] = {9, 9, 9, 9}; // Bandwidth gain (level at which the bandwidth is measured) [dB]
static float32_t G0[4] = {0, 0, 0, 0}; // Reference gain @ DC [dB]
static float32_t G[4] = {10, 10, 10, 10}; // Boost/cut gain [dB]
static float32_t f0[4] = {200, 500, 1000, 1500}; // Center freqency [Hz]

static float32_t fs = 22050;

static float32_t pState[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static float32_t pCoeffs [BANDS_QUANT * 5];
static arm_biquad_casd_df1_inst_f32 Sequ;

void initEqualizer(){

    MusicalGenre_t Flat = {"Flat", {0,0,0,0}};
    MusicalGenre_t Rock = {"Rock", {2,1,0,3}};
    MusicalGenre_t Pop = {"Pop", {1,2,2,4}};
    MusicalGenre_t Acustic = {"Acus", {3,2,2,1}};
    MusicalGenre_t BassBoosted = {"Bass", {4,2,0,1}};
    MusicalGenre_t Jazz = {"Jazz", {1,4,1,1}};

    genres_array[0] = Flat;
    genres_array[1] = Rock;
    genres_array[2] = Pop;
    genres_array[3] = Acustic;
    genres_array[4] = BassBoosted;
    genres_array[5] = Jazz;

    for(uint16_t i = 0; i < BANDS_QUANT; i++){
        if(G[i] != 0){

            float32_t beta = tan(Bf[i]/2 * PI / (fs / 2)) * sqrt(abs( pow(pow(10, GB[i]/20), 2) - pow(pow(10, G0[i]/20), 2) )) /
                             sqrt(abs( pow(pow(10, G[i]/20), 2) - pow(pow(10, GB[i]/20), 2) ));

            pCoeffs[i*5] = (pow(10, G0[i]/20) + pow(10, G[i]/20)*beta) / (1+beta); // b0
            pCoeffs[i*5 + 1] = -2*pow(10, G0[i]/20)*cos(f0[i]*PI/(fs/2)) / (1+beta); // b1
            pCoeffs[i*5 + 2] = (pow(10, G0[i]/20) - pow(10, G[i]/20)*beta) / (1+beta); // b2

            pCoeffs[i*5 + 3] = -(-2*cos(f0[i]*PI/(fs/2))/(1+beta));
            pCoeffs[i*5 + 4] = -(1-beta)/(1+beta);
        }
        else{
            pCoeffs[i*5] = 1;
            pCoeffs[i*5 + 1] = 0;
            pCoeffs[i*5 + 2] = 0;

            pCoeffs[i*5 + 3] = 0;
            pCoeffs[i*5 + 4] = 0;
        }
    }
    arm_biquad_cascade_df1_init_f32(&Sequ, 4, pCoeffs, pState);
}

void setUpFilter(float32_t nuevaGananciadB, uint8_t nroBanda){

    G[nroBanda] = nuevaGananciadB;
    GB[nroBanda] = 0.9*nuevaGananciadB;
    if(nuevaGananciadB != 0){
        float32_t beta = tan(Bf[nroBanda]/2 * PI / (fs / 2)) *
                         sqrt(fabs( pow(pow(10, GB[nroBanda]/20), 2) - pow(pow(10, G0[nroBanda]/20), 2) )) /
                         sqrt(fabs( pow(pow(10, G[nroBanda]/20), 2) - pow(pow(10, GB[nroBanda]/20), 2) ));
        
        pCoeffs[nroBanda*5] = (pow(10, G0[nroBanda]/20) + pow(10, G[nroBanda]/20)*beta) / (1+beta); // b0
        pCoeffs[nroBanda*5 + 1] = -2*pow(10, G0[nroBanda]/20)*cos(f0[nroBanda]*PI/(fs/2)) / (1+beta); // b1
        pCoeffs[nroBanda*5 + 2] = (pow(10, G0[nroBanda]/20) - pow(10, G[nroBanda]/20)*beta) / (1+beta); // b2

        pCoeffs[nroBanda*5 + 3] = -(-2*cos(f0[nroBanda]*PI/(fs/2))/(1+beta));
        pCoeffs[nroBanda*5 + 4] = -(1-beta)/(1+beta);
    }
    else{ 
        pCoeffs[nroBanda*5] = 1;
        pCoeffs[nroBanda*5 + 1] = 0;
        pCoeffs[nroBanda*5 + 2] = 0;

        pCoeffs[nroBanda*5 + 3] = 0;
        pCoeffs[nroBanda*5 + 4] = 0;
    }
    arm_biquad_cascade_df1_init_f32(&Sequ, 4, pCoeffs, pState);
}

void setGenre(Genre_t genre_id)
{
    for(uint8_t i=0; i<BANDS_QUANT; i++)
    {
        setUpFilter(genres_array[genre_id].equalization[i] - 4 ,i);
    }
}

void blockEqualizer(const float32_t * pSrc, float32_t * pDst, uint32_t 	blockSize){
    arm_biquad_cascade_df1_f32(&Sequ, pSrc, pDst, blockSize);
}

void eq_preset_to_str(Genre_t genre, char *str) 
{
    if (genre < GENRES_QUANT) {
        strcpy(str, genres_array[genre].name);
    } else {
        strcpy(str, "Unknown");
    }
}