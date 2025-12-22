#ifndef _EQUALIZER_H_
#define _EQUALIZER_H_

typedef enum {
    GENRE_FLAT = 0,
    GENRE_ROCK,
    GENRE_POP,
    GENRE_ACUSTIC,
    GENRE_BASS_BOOSTED,
    GENRE_JAZZ
} Genre_t;

#include <arm_math.h>

/*!
 * @brief Initializes the filter and the genres array for equalization
 */
void initEqualizer(void);

/*!
 * @brief Changes the filter configuration to the next genre of the array
 * @return A char pointer to a string that contains the name of the genre to display
 */
void setGenre(Genre_t genre_id);

/*!
 * @brief Applies the IIR biquad filtering of 4 bands according to th settings
 *
 * @param pSrc: signal array passed by reference, array with data to filter
 * @param pDst: pointer to the array after processing
 * @param blockSize: array's size of pSrc and pDst
 */
void blockEqualizer(const float32_t * pSrc, float32_t * pDst, uint32_t 	blockSize);

void eq_preset_to_str(Genre_t genre, char *str);

#endif // _EQUALIZER_H_