/***************************************************************************/ /**
   @file     main.c
   @brief    MP3 Player main
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

#include "Audio.h"

//#include "App.h"
#include "os.h"
#include "cpu.h"
#include "board.h"
#include "drivers/TICKS/ticks.h"
#include "MK64F12.h"
#include "drivers/gpio.h"

#include <stdio.h>
#include <stdint.h>
// #include <stdbool.h>
#include <math.h>
#include "hardware.h"
//FAT
#include "drivers/SDHC/sdhc.h"
#include "drivers/FAT/ff.h"
#include "drivers/FAT/diskio.h"
//MP3
#include "helix/pub/mp3dec.h"
#include "mp3_player.h"

// AUDIO
volatile bool PIT_trigger;
volatile bool DMA_trigger;

volatile uint16_t bufA[AUDIO_BUF_LEN];
volatile uint16_t bufB[AUDIO_BUF_LEN];

bool isPlaying = false;

volatile bool decode = true;

// LED MATRIX
static LEDM_t *matrix;
static volatile bool start_new_frame;

//static void make_test_pcm(int16_t *pcm, uint32_t fs_hz);
static void PIT_cb(void);

// FAT
#ifndef MAX_LISTED_ENTRIES
#define MAX_LISTED_ENTRIES 64
#endif

#ifndef MAX_NAME_LEN
#define MAX_NAME_LEN 64     // suficiente para pruebas; si querés LFN largo, subilo
#endif


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#ifndef SystemCoreClock
#define SystemCoreClock 120000000U
#endif

// task priorities 
#define MAIN_TASK_PRIO              8u
#define AUDIO_TASK_PRIO             4u
#define SD_TASK_PRIO                5u
#define DISP_TASK_PRIO              6u
#define LEDMATRIX_TASK_PRIO         7u
// #define DECODE_TASK_PRIO            5u

// stack sizes (check this values)
#define MAIN_STK_SIZE               256u
#define AUDIO_STK_SIZE              2048u
#define SD_STK_SIZE                 1024u
#define DISP_STK_SIZE               2048u
#define LEDMATRIX_STK_SIZE          2048u
#define DECODE_STK_SIZE             2048u

/*******************************************************************************
 * RTOS OBJECTS DECLARATIONS
 ******************************************************************************/
static CPU_STK MainStk[MAIN_STK_SIZE];
static CPU_STK AudioStk[AUDIO_STK_SIZE];
static CPU_STK SdStk[SD_STK_SIZE];
static CPU_STK DispStk[DISP_STK_SIZE];
static CPU_STK LedStk[LEDMATRIX_STK_SIZE];
static CPU_STK DecodeStk[DECODE_STK_SIZE];

static OS_TCB MainTCB;
static OS_TCB AudioTCB;
static OS_TCB SdTCB;
static OS_TCB DispTCB;
static OS_TCB LedTCB;
static OS_TCB DecodeTCB;

static OS_Q     UiQ;        /* UI event queue */
static OS_MUTEX AppMtx;     /* Application state mutex */

static OS_SEM DisplaySem;
static OS_SEM LedFrameSem;
static OS_SEM g_mp3ReadySem;        // 

OS_SEM g_AudioSem;         // indica que hay datos de audio listos


/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/
void App_Init(void);
void App_Start(void);
void App_Run(void);
/*******************************************************************************
 * FUNCTION DEFINITIONS
 ******************************************************************************/
static void App_TaskCreate(void);

static void Main_Task(void *p_arg);
static void Audio_Task(void *p_arg);
static void Display_Task(void *p_arg);
static void LedMatrix_Task(void *p_arg);
static void SD_Task(void *p_arg);
static void Decode_Task(void *p_arg);

void App_Init(void)
{
    App_TaskCreate();


}

void App_Run(void)
{

}

static void App_TaskCreate(void)
{
    OS_ERR err;

    // Create RTOS objects

	OSSemCreate(&LedFrameSem,
	                "LedFrameSem",
	                0u,                     // empieza en 0 → nadie pasa hasta el 1er PIT
	                &err);
    OSSemCreate(&DisplaySem,
                    "Display semaphore",
                    0u,
                    &err);

    OSSemCreate(&g_mp3ReadySem, "mp3_ready", 0, &err);

    OSSemCreate(&g_AudioSem,
                    "Audio semaphore",
                    0u,
                    &err);

    // Create tasks                

    OSTaskCreate(&MainTCB,
                 "Main Task",
                 Main_Task,
                 0,
                 MAIN_TASK_PRIO,
                 &MainStk[0],
                 MAIN_STK_SIZE / 10u,
                 MAIN_STK_SIZE,
                 0u,
                 0u,
                 0u,
                 OS_OPT_TASK_STK_CHK,
                 &err);
//
    OSTaskCreate(&AudioTCB,
                 "Audio Task",
                 Audio_Task,
                 0,
                 AUDIO_TASK_PRIO,
                 &AudioStk[0],
                 AUDIO_STK_SIZE / 10u,
                 AUDIO_STK_SIZE,
                 0u,
                 0u,
                 0u,
                 OS_OPT_TASK_STK_CHK,
                 &err);


    OSTaskCreate(&DispTCB,
                 "Display Task",
                 Display_Task,
                 0,
                 DISP_TASK_PRIO,
                 &DispStk[0],
                 DISP_STK_SIZE / 10u,
                 DISP_STK_SIZE,
                 0u,
                 0u,
                 0u,
                 OS_OPT_TASK_STK_CHK,
                 &err);

    OSTaskCreate(&LedTCB,
                 "LedMatrix Task",
                 LedMatrix_Task,
                 0,
                 LEDMATRIX_TASK_PRIO,
                 &LedStk[0],
                 LEDMATRIX_STK_SIZE / 10u,
                 LEDMATRIX_STK_SIZE,
                 0u,
                 0u,
                 0u,
				 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_SAVE_FP,
                 &err);
                
    OSTaskCreate(&SdTCB,
                 "SD Task",
                 SD_Task,
                 0,
                 SD_TASK_PRIO,
                 &SdStk[0],
                 SD_STK_SIZE / 10u,
                 SD_STK_SIZE,
                 0u,
                 0u,
                 0u,
                 OS_OPT_TASK_STK_CHK,
                 &err);

//    OSTaskCreate(&DecodeTCB,
//                "Decode Task",
//                Decode_Task,
//                0,
//                DECODE_TASK_PRIO,
//                &DecodeStk[0],
//                DECODE_STK_SIZE / 10u,
//                DECODE_STK_SIZE,
//                0u,
//                0u,
//                0u,
//                OS_OPT_TASK_STK_CHK,
//                &err);
}

/* TASKS */
static void Main_Task(void *p_arg)
{
    (void)p_arg;
    OS_ERR err;

    while (1) {
        // Display

        OSTimeDlyHMSM(0u, 0u, 0u, 20u, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}


static void Audio_Task(void *p_arg)
{
    (void)p_arg;
    OS_ERR err;

    // while (pcm_ring_level() < (PCM_RING_SIZE / 8)) {
    //     OSTimeDly(1u, OS_OPT_TIME_DLY, &err);
    // }
    
    // uint32_t fs_mp3 = MP3Player_GetSampleRateHz();
    gpioMode(PORTNUM2PIN(PC,10), OUTPUT);
	gpioWrite(PORTNUM2PIN(PC,10), 1);
    

    OSSemPend(&g_mp3ReadySem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);
    Audio_Init();
    // antes de arrancar DMA/audio

    while (1) {
        // buttons state / debounce
        
        // if (isPlaying)
        // {
            OSSemPend(&g_AudioSem, 0u, OS_OPT_PEND_BLOCKING, 0u, &err);
    		// gpioWrite(PORTNUM2PIN(PC,11), HIGH);
            Audio_Service();
            // gpioWrite(PORTNUM2PIN(PC,11), LOW);            
        }
    // }
}

static void Display_Task(void *p_arg)
{
    (void)p_arg;
    OS_ERR err;

    while (1) {
        // Display
    	OSSemPend(&DisplaySem, 0u, OS_OPT_PEND_BLOCKING, 0u, &err);

        OSTimeDlyHMSM(0,0,0,20, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}
static void LedMatrix_Task(void *p_arg)
{
    (void)p_arg;
    OS_ERR err;

    //	LEDMATRIX TEST
	matrix = LEDM_Init(8, 8);

	FFT_Init();
	PIT_Init(PIT_0, 10);		// PIT 0 para controlar FPS y refrescar matrix de leds,
	PIT_SetCallback(PIT_cb, PIT_0);

    static int16_t frame[FFT_N];
	static float bands[8];

    while (1) {
    	OSSemPend(&LedFrameSem, 0u, OS_OPT_PEND_BLOCKING, 0u, &err);

		LEDM_SetBrightness(matrix, 2);

        MP3Player_GetLastPCMwindow(frame, FFT_N);

		FFT_ComputeBands(frame, FFT_N, AUDIO_FS_HZ, bands);

		Visualizer_DrawBars(bands, matrix);

		bool ok = LEDM_Show(matrix);

		if(ok){
			while (LEDM_TransferInProgress()) {
//			    OSTimeDly(5u, OS_OPT_TIME_DLY, &err);
				OSTimeDlyHMSM(0u, 0u, 0u, 20u, OS_OPT_TIME_HMSM_STRICT, &err);
			}
		}
    }
}


static FATFS g_fs;
static FIL   g_song;

static void SD_Task(void *p_arg)
{
    (void)p_arg;
    OS_ERR err;

    // 1) SDHC init (igual que antes)
    sdhc_enable_clocks_and_pins();
    sdhc_reset(SDHC_RESET_CMD);
    sdhc_reset(SDHC_RESET_DATA);
    __enable_irq();

    // 2) Montar FS
    FRESULT fr = f_mount(&g_fs, "0:", 1);
    if (fr != FR_OK) {
        // Si falla, quedate vivo (sin printf)
        while (1) OSTimeDly(10u, OS_OPT_TIME_DLY, &err);
    }

    // 3) Abrir MP3
    fr = f_open(&g_song, "0:/TALKTO~1.MP3", FA_READ);
    if (fr != FR_OK) {
        while (1) OSTimeDly(10u, OS_OPT_TIME_DLY, &err);
    }

    // 4) Inicializar Helix + buffers del player
    if (!MP3Player_InitWithOpenFile(&g_song)) {
        while (1) OSTimeDly(10u, OS_OPT_TIME_DLY, &err);
    }
    
    gpioMode(PORTNUM2PIN(PC,11), OUTPUT);
	gpioWrite(PORTNUM2PIN(PC,11), 1);

    OSSemPost(&g_mp3ReadySem, OS_OPT_POST_1, &err);

    while (pcm_ring_level() < (AUDIO_BUF_LEN * 12)) {   // 6 bloques, ajustá
        MP3Player_DecodeAsMuchAsPossibleToRing();
    }

    // // opcional: prellenar un poco antes de arrancar audio (ej 1/4 ring)
    // while (pcm_ring_level() < (PCM_RING_SIZE / 4)) {
    //     (void)MP3Player_DecodeOneFrameToRing();
    // }

    // 5) A partir de acá, el audio task puede ir pidiendo samples.
    // Si querés, podés dormir esta task.
    while (1) {
        // Si hay espacio, decodificar más
        // if (!MP3Player_DecodeAsMuchAsPossibleToRing()) {
        //     // EOF/underrun: podés dormir o marcar stop
        //     OSTimeDly(5u, OS_OPT_TIME_DLY, &err);
        // }
//    	gpioWrite(PORTNUM2PIN(PC,11),HIGH);
        bool ok = MP3Player_DecodeAsMuchAsPossibleToRing();
//        gpioWrite(PORTNUM2PIN(PC,11),LOW);

        if (!ok) {
            // reintento corto, no 5 ticks
            OSTimeDly(1u, OS_OPT_TIME_DLY, &err);
        } else {
            // si ya está bastante lleno, podés dormir un toque
            if (pcm_ring_free() == 0) {
                OSTimeDly(1u, OS_OPT_TIME_DLY, &err);
            } else {
                OSTimeDly(0u, OS_OPT_TIME_DLY, &err); // yield
            }
        }
    }
}

/********************************
 *      PIT CALLBACKS
 ********************************/

static void PIT_cb(void)
{
	OS_ERR err;
	OSSemPost(&LedFrameSem, OS_OPT_POST_1, &err);
}

/********************************
 *      MAIN
 ********************************/


int main(void)
{
    OS_ERR err;

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_ERR cpu_err;
#endif

    hw_Init();
    OSInit(&err);
#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
    /* Enable task round robin. */
    OSSchedRoundRobinCfg((CPU_BOOLEAN)1, 0, &err);
#endif
    OS_CPU_SysTickInit(SystemCoreClock / (uint32_t)OSCfg_TickRate_Hz);

    CPU_Init();
    App_Init();

    App_OS_SetAllHooks();


    OSStart(&err);

    /* Should Never Get Here */
    while (1);
}


