/***************************************************************************/ /**
   @file     main.c
   @brief    MP3 Player main (SDHC minimal access - polling)
   @author   Grupo 3 (corregido)
  ******************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "MK64F12.h"
#include "pin_mux.h"
#include "gpio.h"
#include "board.h"
#include "drivers/SDHC/sdhc.h"

//File System
#include "drivers/FAT/ff.h"

//MP3
#include "mp3dec.h"
#include "mp3common.h"

#include "fsl_clock.h"


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS
 ******************************************************************************/
#define MP3_INBUF_SIZE   4096
#define MP3_OUT_SAMPLES  (1152 * 2)
//#define BUS_CLK_HZ CLOCK_GetFreq(kCLOCK_BusClk)
#define BUS_CLK_HZ 50000000


/*******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/
void mp3_decode_file(const char *path);
void process_pcm(int16_t *pcm, int samples, int rate, int ch);
static inline void dac0_init(void);
static inline uint16_t pcm16_to_dac12(int16_t x);

/*******************************************************************************
 * GLOBAL DATA
 ******************************************************************************/

static FATFS fs;

static uint8_t  mp3_inbuf[MP3_INBUF_SIZE];
static int16_t  pcm_out[MP3_OUT_SAMPLES];



////////////////PRUEBA: EMPROLIJAR
//#define AUDIO_RING_SZ 8192   // potencia de 2 (recomendado)
#define AUDIO_RING_SZ 16384

static volatile uint16_t audio_ring[AUDIO_RING_SZ];
static volatile uint32_t wr_idx = 0;
static volatile uint32_t rd_idx = 0;

static inline uint32_t ring_count(void)
{
    return (wr_idx - rd_idx) & (AUDIO_RING_SZ - 1);
}

static inline uint32_t ring_space(void)
{
    return (AUDIO_RING_SZ - 1) - ring_count();
}

static inline void ring_push(uint16_t v)
{
    audio_ring[wr_idx & (AUDIO_RING_SZ - 1)] = v;
    wr_idx++;
}

static inline bool ring_pop(uint16_t *out)
{
    if (wr_idx == rd_idx) return false;
    *out = audio_ring[rd_idx & (AUDIO_RING_SZ - 1)];
    rd_idx++;
    return true;
}
//////////////////// TODO

//#define BUS_CLK_HZ 100000000u

static inline void pit_init(uint32_t sample_rate_hz)
{
    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
    PIT->MCR = 0; // enable PIT

    uint32_t ldval = (BUS_CLK_HZ / sample_rate_hz) - 1u;
    PIT->CHANNEL[0].LDVAL = ldval;
    PIT->CHANNEL[0].TCTRL = PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK;

    NVIC_EnableIRQ(PIT0_IRQn);
}

void PIT0_IRQHandler(void)
{
    PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK; // clear

    uint16_t s;
    if (!ring_pop(&s)) {
        s = 0x0800; // underrun => silencio (midscale)
    }

    DAC0->DAT[0].DATL = (uint8_t)(s & 0xFF);
    DAC0->DAT[0].DATH = (uint8_t)((s >> 8) & 0x0F);
}

//////////////////// TODO

void list_dir(const char *path)
{
    DIR dir;
    FILINFO fno;
    FRESULT fr;

    fr = f_opendir(&dir, path);
    if (fr != FR_OK) {
//        PRINTF("f_opendir(%s) err=%d\r\n", path, fr);
        return;
    }

    for (;;)
    {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK) {
//            PRINTF("f_readdir err=%d\r\n", fr);
            break;
        }

        if (fno.fname[0] == 0) {   // fin del directorio
            break;
        }

        if (fno.fattrib & AM_DIR) {
//            PRINTF("[DIR ] %s\r\n", fno.fname);
        } else {
//            PRINTF("[FILE] %s  (%lu bytes)\r\n", fno.fname, (unsigned long)fno.fsize);
        }
    }

    f_closedir(&dir);
}

void App_Init(void)
{
	// SDHC driver initialization
	sdhc_enable_clocks_and_pins();
	sdhc_reset(SDHC_RESET_CMD);
	sdhc_reset(SDHC_RESET_DATA);
	dac0_init();
	gpioMode(PORTNUM2PIN(PB,2), OUTPUT);
	__enable_irq();
}

/*******************************************************************************
 * App_Run - loop principal (vacío para este PoC)
 ******************************************************************************/
void App_Run(void)
{
    FATFS fs;
    FIL fil;
    FRESULT fr;
    UINT br;

    /* 1) Montar FS */
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        // PRINTF("f_mount error=%d\r\n", fr);
        while (1) {}
    }
    list_dir("0:/");

    /* 2) Abrir MP3 */
    fr = f_open(&fil, "0:/TALKTO~1.MP3", FA_READ);
    if (fr != FR_OK) {
        // PRINTF("f_open error=%d\r\n", fr);
        while (1) {}
    }

    /* 3) Init audio HW */
    dac0_init();

    /* 4) Init Helix */
    HMP3Decoder hMP3 = MP3InitDecoder();
    if (!hMP3) {
        while (1) {}
    }

    MP3FrameInfo fi;
    int pit_started = 0;

    uint8_t *readPtr = mp3_inbuf;
    int bytesLeft = 0;

    /* 5) Loop de decode */
    while (1)
    {
    	gpioToggle(PORTNUM2PIN(PB,2));
        /* Compactar lo no consumido al inicio del buffer */
        if (bytesLeft > 0) {
            memmove(mp3_inbuf, readPtr, (size_t)bytesLeft);
        }
        readPtr = mp3_inbuf;

        /* Leer más bytes del archivo */
        fr = f_read(&fil, mp3_inbuf + bytesLeft, MP3_INBUF_SIZE - bytesLeft, &br);
        if (fr != FR_OK) {
            while (1) {}
        }

        if (br == 0 && bytesLeft == 0) {
            break;  // EOF y nada pendiente
        }

        bytesLeft += (int)br;

        /* Decodificar frames mientras haya data */
        while (bytesLeft > 0)
        {
            int off = MP3FindSyncWord(readPtr, bytesLeft);
            if (off < 0) {
                /* No hay sync: pedir más data */
                break;
            }

            readPtr += off;
            bytesLeft -= off;

            int err = MP3Decode(hMP3, &readPtr, &bytesLeft, pcm_out, 0);
            if (err != ERR_MP3_NONE) {
                /* Frame inválido: avanzar 1 byte y reintentar */
                if (bytesLeft > 0) {
                    readPtr++;
                    bytesLeft--;
                }
                continue;
            }

            MP3GetLastFrameInfo(hMP3, &fi);

            /* 6) Arrancar PIT con la sample rate real del MP3 (una vez) */
            if (!pit_started) {
                pit_init((uint32_t)fi.samprate);  // típicamente 44100 o 48000
                pit_started = 1;
            }

            /* 7) Encolar PCM (esto bloquea si el ring se llena) */
            process_pcm(pcm_out, fi.outputSamps, fi.samprate, fi.nChans);
        }

        /* Si EOF pero queda algo en buffer, seguimos intentando decodificar */
        if (br == 0) {
            if (bytesLeft == 0) break;
        }
    }

    /* 8) Cleanup (opcional) */
    f_close(&fil);
    MP3FreeDecoder(hMP3);

    /* Dejar sonando hasta vaciar buffer (o quedarte en loop) */
    while (1) {
        __WFI();
    }
}



/*******************************************************************************
 * LOCAL FUNCTION DEFINITIONS
 ******************************************************************************/
void mp3_decode_file(const char *path)
{
    FIL fil;
    FRESULT fr;
    UINT br;

    HMP3Decoder hMP3 = MP3InitDecoder();
    MP3FrameInfo frameInfo;

    fr = f_open(&fil, path, FA_READ);
    if (fr != FR_OK) while (1) {}

    int bytesLeft = 0;
    uint8_t *readPtr = mp3_inbuf;

    while (1)
    {
        /* Compactar buffer */
        if (bytesLeft > 0)
            memmove(mp3_inbuf, readPtr, bytesLeft);

        readPtr = mp3_inbuf;

        fr = f_read(&fil,
                     mp3_inbuf + bytesLeft,
                     MP3_INBUF_SIZE - bytesLeft,
                     &br);
        if (fr != FR_OK) break;
        if (br == 0 && bytesLeft == 0) break; // EOF

        bytesLeft += br;

        while (bytesLeft > 0)
        {
            int offset = MP3FindSyncWord(readPtr, bytesLeft);
            if (offset < 0)
                break;

            readPtr += offset;
            bytesLeft -= offset;

            int err = MP3Decode(hMP3,
                                &readPtr,
                                &bytesLeft,
                                pcm_out,
                                0);
            if (err != ERR_MP3_NONE)
            {
                /* frame inválido → saltar un byte */
                readPtr++;
                bytesLeft--;
                continue;
            }

            MP3GetLastFrameInfo(hMP3, &frameInfo);

            /* ACÁ TENÉS EL AUDIO DECODIFICADO */
            /* frameInfo.outputSamps samples en pcm_out[] */
            /* frameInfo.samprate, frameInfo.nChans */

            process_pcm(pcm_out,
                        frameInfo.outputSamps,
                        frameInfo.samprate,
                        frameInfo.nChans);
        }
    }

    f_close(&fil);
    MP3FreeDecoder(hMP3);
}

void process_pcm(int16_t *pcm, int total_samps, int rate, int nChans)
{
    (void)rate; // por ahora asumimos que el PIT ya está configurado a ese rate

    if (nChans == 1)
    {
        for (int i = 0; i < total_samps; i++)
        {
            uint16_t v = pcm16_to_dac12(pcm[i]);

            while (ring_space() == 0) {
                __WFI(); // esperar a que la ISR consuma
            }
            ring_push(v);
        }
    }
    else if (nChans == 2)
    {
        // total_samps incluye ambos canales (interleaved L,R,L,R,...)
        for (int i = 0; i < total_samps; i += 2)
        {
            int32_t L = pcm[i];
            int32_t R = pcm[i + 1];
            int16_t mono = (int16_t)((L + R) / 2);

            uint16_t v = pcm16_to_dac12(mono);

            while (ring_space() == 0) {
                __WFI();
            }
            ring_push(v);
        }
    }
}


static inline void dac0_init(void)
{
    SIM->SCGC2 |= SIM_SCGC2_DAC0_MASK;   // clock DAC0

    // DAC0: habilitar, usar referencia VDDA (por defecto)
    DAC0->C0 = DAC_C0_DACEN_MASK;        // enable
    DAC0->C1 = 0;                        // sin buffer/sw triggers
    DAC0->C2 = 0;

    // valor inicial a mitad de escala (silencio)
    DAC0->DAT[0].DATL = 0x00;
    DAC0->DAT[0].DATH = 0x08;            // 0x800 = midscale
}

static inline uint16_t pcm16_to_dac12(int16_t x)
{
    int32_t u = (int32_t)x + 32768;  // 0..65535
    u >>= 4;                         // 0..4095
    if (u < 0) u = 0;
    if (u > 4095) u = 4095;
    return (uint16_t)u;
}

