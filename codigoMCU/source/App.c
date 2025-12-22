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

#include "os.h"
#include "cpu.h"
#include "board.h"
#include "drivers/TICKS/ticks.h"
#include "MK64F12.h"
#include "drivers/gpio.h"
#include "BTN/BTN.h"
#include "drivers/LCD/LCD.h"
#include "drivers/Encoder/encoder.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

//SD
bool isPlaying = false;

volatile uint16_t bufA[AUDIO_BUF_LEN];
volatile uint16_t bufB[AUDIO_BUF_LEN];


volatile bool decode = true;

// LED MATRIX
static LEDM_t *matrix;
static volatile bool start_new_frame;

static void PIT_cb(void);

// FAT
#ifndef MAX_LISTED_ENTRIES
#define MAX_LISTED_ENTRIES 64
#endif

#ifndef MAX_NAME_LEN
#define MAX_NAME_LEN 64     // suficiente para pruebas; si querés LFN largo, subilo
#endif

#define MAX_TRACKS   64
#define MAX_PATH_LEN 128


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
// #ifndef SystemCoreClock
// #define SystemCoreClock (120000000U)
// #endif

// task priorities 
#define MAIN_TASK_PRIO              3u
#define AUDIO_TASK_PRIO             5u
#define SD_TASK_PRIO                6u
#define DISP_TASK_PRIO              4u
#define LEDMATRIX_TASK_PRIO         7u

// stack sizes (check this values)
#define MAIN_STK_SIZE               256u
#define AUDIO_STK_SIZE              2048u
#define SD_STK_SIZE                 1024u
#define DISP_STK_SIZE               2048u
#define LEDMATRIX_STK_SIZE          2048u

#define QUEUE_SIZE  10

typedef enum {
    APP_STATE_PLAYING = 0,
    APP_STATE_PAUSED,
    APP_STATE_SELECT_TRACK,
} controlState_t;
static controlState_t currentState = APP_STATE_SELECT_TRACK;
static controlState_t SDState = APP_STATE_SELECT_TRACK;
static controlState_t displayState = APP_STATE_SELECT_TRACK;

typedef enum {
    APP_EVENT_NONE = 0,
    APP_EVENT_PLAY,
    APP_EVENT_PAUSE,
    APP_EVENT_ENC_RIGHT,
    APP_EVENT_ENC_LEFT,
    APP_EVENT_ENC_BUTTON,
    APP_EVENT_NEXT_TRACK,
    APP_EVENT_PREV_TRACK,
} controlEvent_t ;
static controlEvent_t currentEvent = APP_EVENT_NONE;
static controlEvent_t SDEvent = APP_EVENT_NONE;

/*******************************************************************************
 * RTOS OBJECTS DECLARATIONS
 ******************************************************************************/
static CPU_STK MainStk[MAIN_STK_SIZE];
static CPU_STK AudioStk[AUDIO_STK_SIZE];
static CPU_STK SdStk[SD_STK_SIZE];
static CPU_STK DispStk[DISP_STK_SIZE];
static CPU_STK LedStk[LEDMATRIX_STK_SIZE];

static OS_TCB MainTCB;
static OS_TCB AudioTCB;
static OS_TCB SdTCB;
static OS_TCB DispTCB;
static OS_TCB LedTCB;

static OS_Q     UiQ;        /* UI event queue */
static OS_MUTEX AppMtx;     /* Application state mutex */

static OS_SEM DisplaySem;
static OS_SEM LedFrameSem;
static OS_SEM g_mp3ReadySem;        // 

OS_SEM g_AudioSem;         // indica que hay datos de audio listos

char *filenames[MAX_TRACKS] = {"TALKTO~1.MP3"};
static char filenames_storage[MAX_TRACKS][MAX_PATH_LEN];
static uint32_t filenames_count = 0;
static uint32_t filenamesIdx = 0;

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/
void App_Init(void);
/*******************************************************************************
 * FUNCTION DEFINITIONS
 ******************************************************************************/
static void App_TaskCreate(void);

static void Main_Task(void *p_arg);
static void Audio_Task(void *p_arg);
static void Display_Task(void *p_arg);
static void LedMatrix_Task(void *p_arg);
static void SD_Task(void *p_arg);

void App_Init(void)
{
    // OS Task creation & init
    App_TaskCreate();
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
}

/* TASKS */
static void Main_Task(void *p_arg)
{
    (void)p_arg;
    OS_ERR err;

    // Drivers initialization
    init_LCD();
    encoderInit();
    init_user_buttons();

    write_LCD("Welcome!", 0);
    OSTimeDlyHMSM(0u, 0u, 3u, 0u, OS_OPT_TIME_HMSM_STRICT, &err);
    OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);

    while (1)
    {
        /****** BUTTON EVENTS ***************/ 
        if(get_BTN_state(PLAY_BTN))
        {
            switch (currentState)
            {
                case APP_STATE_PLAYING:
                    currentEvent = APP_EVENT_PAUSE;
                    break;
                case APP_STATE_PAUSED:
                    currentEvent = APP_EVENT_PLAY;
                    break;
                default:
                    currentEvent = APP_EVENT_NONE;
                    break;
            }
        }
        else if(get_BTN_state(NEXT_BTN))
            currentEvent = (currentState == APP_STATE_SELECT_TRACK) ? APP_EVENT_NEXT_TRACK : APP_EVENT_NONE;
        else if(get_BTN_state(PREV_BTN))
            currentEvent = (currentState == APP_STATE_SELECT_TRACK) ? APP_EVENT_PREV_TRACK : APP_EVENT_NONE;
        else currentEvent = APP_EVENT_NONE;
        /****************************************/ 
        /******** ENCODER EVENTS ****************/
        if(getTurns() > 0) 
            currentEvent = APP_EVENT_ENC_RIGHT;
        if(getTurns() < 0) 
            currentEvent = APP_EVENT_ENC_LEFT;
        if(getSwitchState() == BTN_CLICK) 
            currentEvent = APP_EVENT_ENC_BUTTON;
        if(getSwitchState() == BTN_LONG_CLICK) 
            currentEvent = APP_EVENT_ENC_BUTTON;
        /*****************************************/

        switch (currentState){
        	case APP_STATE_SELECT_TRACK:
                // falta hacer el menu literalmente, 
                // escribir cosas en el display
                switch(currentEvent) {
                    case(APP_EVENT_ENC_BUTTON):
                        // guardar referencia a la canción que va a empezar a reproducir (SD)
                        // empezar transferencia SD → audio (SD decoder + audio + ledMatrix)
                        // OSSemPost();
                        // isPlaying = true;
                        // currentState = APP_STATE_PLAYING;
                        // La transicion de estado se da despues de abrir el archivo MP3
                        displayState= APP_STATE_PLAYING;
                        SDState = APP_STATE_SELECT_TRACK;
                        currentState = APP_STATE_PLAYING;
                        
                        SDEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;
                        
                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        break;
                    case(APP_EVENT_ENC_RIGHT):
                        // actualiza display con la siguiente cancion
                        // puntero a la siguiente cancion
                        // SD browsing (metadata)
                        filenamesIdx = (filenamesIdx + 1) % filenames_count;

                        displayState= APP_STATE_SELECT_TRACK;
                        SDState = APP_STATE_SELECT_TRACK;
                        
                        SDEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;
                        
                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        break;
                    case(APP_EVENT_ENC_LEFT):
                        // actualiza display con la cancion anterior
                        // puntero a la cancion anterior
                        // SD browsing (metadata)
                        filenamesIdx = (filenamesIdx - 1 + filenames_count) % filenames_count;

                        displayState= APP_STATE_SELECT_TRACK;
                        SDState = APP_STATE_SELECT_TRACK;

                        SDEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;

                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        break;
                    default:
                }
        		break;

        	case APP_STATE_PLAYING:
                switch(currentEvent) {
                    case(APP_EVENT_NEXT_TRACK):
                        // empezar a reproducir el siguiente track
                        SDEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;

                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        break;
                    case(APP_EVENT_PREV_TRACK):
                        // empezar a reproducir el track anterior
                        SDEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;

                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        break;
                    case(APP_EVENT_PAUSE):
                    case(APP_EVENT_ENC_BUTTON):
                        // transitions between states
                        // pausar transferencia SD → audio (SD decoder + audio + ledMatrix)
                        // actualiza display para mostrar estado de pausa
                        isPlaying = false;
                            
                        SDState = APP_STATE_PAUSED;
                        displayState = APP_STATE_PAUSED;
                        currentState = APP_STATE_PAUSED;
                        
                        SDEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;
                        
                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        break;
                    case(APP_EVENT_ENC_RIGHT):
                    case(APP_EVENT_ENC_LEFT):
                        // pausar transferencia SD → audio (SD decoder + audio + ledMatrix)
                        // actualiza display con el menu
                        // puntero a la cancion anterior
                        // SD browsing (metadata)
                        filenamesIdx = 0; // volver al primer track
                        isPlaying = false;

                        SDState = APP_STATE_SELECT_TRACK;
                        displayState = APP_STATE_SELECT_TRACK;
                        currentState = APP_STATE_SELECT_TRACK;
                        
                        SDEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;

                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        break;
                    default:
                }
        		break;
        	case APP_STATE_PAUSED:
                switch(currentEvent) {
                    case(APP_EVENT_PLAY):
                    case(APP_EVENT_ENC_BUTTON):
                        // empezar transferencia SD → audio desde donde quedo (SD decoder + audio + ledMatrix)
                        // actualiza display para mostrar estado de playing
                        // isPlaying = true;
                        DMA_SetEnableRequest(DMA_CH1, true);   //T
                        isPlaying = true;

                        SDState = APP_STATE_PLAYING;
                        displayState = APP_STATE_PLAYING;
                        currentState = APP_STATE_PLAYING;

                        SDEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;

                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        break;
                    case(APP_EVENT_ENC_RIGHT):
                    case(APP_EVENT_ENC_LEFT):
                        filenamesIdx = 0; // volver al primer track
                        // ir a seleccion de track
                        isPlaying = false;
                        
                        SDState = APP_STATE_SELECT_TRACK;
                        displayState = APP_STATE_SELECT_TRACK;
                        currentState = APP_STATE_SELECT_TRACK;
                        
                        SDEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;

                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        break;
                    // case(APP_EVENT_NEXT_TRACK):
                    //     // empezar a reproducir el siguiente track
                    //     currentState = APP_STATE_PLAYING;
                    //     currentEvent = APP_EVENT_NONE;
                    //     break;
                    // case(APP_EVENT_PREV_TRACK):
                    //     // empezar a reproducir el track anterior
                    //     currentState = APP_STATE_PLAYING;
                    //     currentEvent = APP_EVENT_NONE;
                    //     break;
                    default:
                }
        		break;
        }
        OSTimeDlyHMSM(0u, 0u, 0u, 20u, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}


static void Audio_Task(void *p_arg)
{
    (void)p_arg;
    OS_ERR err;

    OSSemPend(&g_mp3ReadySem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);
    Audio_Init();
    // antes de arrancar DMA/audio

    while (1) {
            if(isPlaying)
            {
                OSSemPend(&g_AudioSem, 0u, OS_OPT_PEND_BLOCKING, 0u, &err);
                Audio_Service();         
            }
            else
            {
                DMA_SetEnableRequest(DMA_CH1, false);
            }
        }
}

static void Display_Task(void *p_arg)
{
    (void)p_arg;
    OS_ERR err;

    while (1) {
        // Display
    	OSSemPend(&DisplaySem, 0u, OS_OPT_PEND_BLOCKING, 0u, &err);
        switch(displayState) {
            case(APP_STATE_PLAYING):
                clear_LCD();
                write_LCD("Playing", 0);
                break;
            case(APP_STATE_PAUSED):
                clear_LCD();
                write_LCD("Paused", 0);
                break;
            case(APP_STATE_SELECT_TRACK):
                clear_LCD();
                write_LCD("Select Track", 0);
                break;
            default:
        }
        write_LCD(filenames[filenamesIdx], 1);
        OSTimeDlyHMSM(0u, 0u, 0u, 50u, OS_OPT_TIME_HMSM_STRICT, &err);
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

    DIR dir;
    FILINFO fno;

    filenames_count = 0;

    #if FF_USE_LFN
        // Si LFN está habilitado en FatFs
        static char lfn[128];
        fno.lfname = lfn;
        fno.lfsize = sizeof(lfn);
    #endif

    filenames_count = 0;

    fr = f_opendir(&dir, "0:/");
    if (fr != FR_OK) return fr;

    for (;;) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK) break;
        if (fno.fname[0] == 0) break;          // fin

        if (fno.fattrib & AM_DIR) continue;    // descartar directorios

        const char *name =
        #if FF_USE_LFN
            (fno.lfname && fno.lfname[0]) ? fno.lfname : fno.fname;
        #else
            fno.fname;
        #endif

        if (!is_mp3_file(name)) continue;   // descartar no-mp3

        // Guardar mientras haya espacio
        if (filenames_count < MAX_TRACKS) {
            // Guardar path completo en storage fijo
            // OJO: si el nombre es muy largo, se trunca para no overflow.
            char *dst = filenames_storage[filenames_count];

            // Inicializar string vacío
            dst[0] = '\0';

            // Copiar "0:/"
            strcpy(dst, "0:/");

            // Concatenar nombre del archivo, cuidando overflow
            strncat(dst, name, MAX_PATH_LEN - strlen(dst) - 1);

            filenames[filenames_count] = filenames_storage[filenames_count];
            filenames_count++;
        } else {
            break; // ya llenaste filenames[4]
        }
    }

    f_closedir(&dir);



    
    while (1) {
        switch(SDState) {
            case(APP_STATE_PLAYING):
                bool ok = MP3Player_DecodeAsMuchAsPossibleToRing();

                if (!ok)
                    // reintento corto, no 5 ticks
                    OSTimeDly(1u, OS_OPT_TIME_DLY, &err);
                else
                {
                // si ya está bastante lleno, podés dormir un toque
                    if (pcm_ring_free() == 0)
                        OSTimeDly(1u, OS_OPT_TIME_DLY, &err);
                    else 
                        OSTimeDly(0u, OS_OPT_TIME_DLY, &err); // yield
                }
                break;
            case(APP_STATE_SELECT_TRACK):
                switch(SDEvent) {
                    case(APP_EVENT_ENC_BUTTON):
                        // 3) Abrir MP3
                        fr = f_open(&g_song, filenames[filenamesIdx], FA_READ);
                        if (fr != FR_OK) 
                            while (1) OSTimeDly(10u, OS_OPT_TIME_DLY, &err);
                        // 4) Inicializar Helix + buffers del player
                        if (!MP3Player_InitWithOpenFile(&g_song))
                            while (1) OSTimeDly(10u, OS_OPT_TIME_DLY, &err);

                        DMA_SetEnableRequest(DMA_CH1, true);
                        isPlaying = true;

                        OSSemPost(&g_mp3ReadySem, OS_OPT_POST_1, &err);
                        SDState = APP_STATE_PLAYING;
                        break;
                    case(APP_EVENT_ENC_RIGHT):
                        // avanzar ptr de lista de canciones
                        break;
                    case(APP_EVENT_ENC_LEFT):
                        // retroceder ptr de lista de canciones
                        break;
                    default:
                }
                break;
        }
        SDEvent = APP_EVENT_NONE;
        // if(isPlaying)
        // {
            // bool ok = MP3Player_DecodeAsMuchAsPossibleToRing();

            // if (!ok) {
            //     // reintento corto, no 5 ticks
            //     OSTimeDly(1u, OS_OPT_TIME_DLY, &err);
            // } else {
            //     // si ya está bastante lleno, podés dormir un toque
            //     if (pcm_ring_free() == 0) {
            //         OSTimeDly(1u, OS_OPT_TIME_DLY, &err);
            //     } else {
            //         OSTimeDly(0u, OS_OPT_TIME_DLY, &err); // yield
            //     }
            // }
        // }
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


