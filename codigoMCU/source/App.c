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


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
// #ifndef SystemCoreClock
// #define SystemCoreClock (120000000U)
// #endif

// task priorities 
#define MAIN_TASK_PRIO              8u
#define AUDIO_TASK_PRIO             4u
#define SD_TASK_PRIO                5u
#define DISP_TASK_PRIO              6u
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
static controlEvent_t displayEvent = APP_EVENT_NONE;

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

char * filenames[4] = {
    "0:/TALKTO~1.MP3",
    "track2.mp3",
    "track3.mp3",
    "track4.mp3"
};

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
    OSTimeDlyHMSM(0u, 0u, 0u, 20u, OS_OPT_TIME_HMSM_STRICT, &err);

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
                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        // OSSemPost();
                        // isPlaying = true;
                        // currentState = APP_STATE_PLAYING;
                        // La transicion de estado se da despues de abrir el archivo MP3
                        displayState= APP_STATE_SELECT_TRACK;
                        SDState = APP_STATE_SELECT_TRACK;
                        currentState = APP_STATE_PLAYING;
                        SDEvent = currentEvent;
                        displayEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;
                        break;
                    case(APP_EVENT_ENC_RIGHT):
                        // actualiza display con la siguiente cancion
                        // puntero a la siguiente cancion
                        // SD browsing (metadata)
                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        SDEvent = currentEvent;
                        displayEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;
                        break;
                    case(APP_EVENT_ENC_LEFT):
                        // actualiza display con la cancion anterior
                        // puntero a la cancion anterior
                        // SD browsing (metadata)
                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        SDEvent = currentEvent;
                        displayEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;
                        break;
                    default:
                }
        		break;

        	case APP_STATE_PLAYING:
                switch(currentEvent) {
                    case(APP_EVENT_NEXT_TRACK):
                        // empezar a reproducir el siguiente track
                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        SDEvent = currentEvent;
                        displayEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;
                        break;
                    case(APP_EVENT_PREV_TRACK):
                        // empezar a reproducir el track anterior
                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        SDEvent = currentEvent;
                        displayEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;
                        break;
                    case(APP_EVENT_PAUSE):
                    // transitions between states
                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        // pausar transferencia SD → audio (SD decoder + audio + ledMatrix)
                        // actualiza display para mostrar estado de pausa
                        isPlaying = false;
                        SDState = APP_STATE_PAUSED;
                        displayState = APP_STATE_PAUSED;
                        currentState = APP_STATE_PAUSED;
                        SDEvent = currentEvent;
                        displayEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;
                        break;
                    case(APP_EVENT_ENC_RIGHT):
                    case(APP_EVENT_ENC_LEFT):
                    case(APP_EVENT_ENC_BUTTON):
                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        // pausar transferencia SD → audio (SD decoder + audio + ledMatrix)
                        // actualiza display con el menu
                        // puntero a la cancion anterior
                        // SD browsing (metadata)
                        isPlaying = false;
                        SDState = APP_STATE_SELECT_TRACK;
                        displayState = APP_STATE_SELECT_TRACK;
                        currentState = APP_STATE_SELECT_TRACK;
                        SDEvent = currentEvent;
                        displayEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;
                        break;
                    default:
                }
        		break;
        	case APP_STATE_PAUSED:
                switch(currentEvent) {
                    case(APP_EVENT_PLAY):
                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        // empezar transferencia SD → audio desde donde quedo (SD decoder + audio + ledMatrix)
                        // actualiza display para mostrar estado de playing
                        // isPlaying = true;
                        SDState = APP_STATE_PAUSED;
                        displayState = APP_STATE_PAUSED;
                        displayEvent = currentEvent;
                        SDEvent = currentEvent;
                        currentState = APP_STATE_PLAYING;
                        currentEvent = APP_EVENT_NONE;
                        break;
                    case(APP_EVENT_ENC_RIGHT):
                    case(APP_EVENT_ENC_LEFT):
                    case(APP_EVENT_ENC_BUTTON):
                        OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                        // ir a seleccion de track
                        isPlaying = false;
                        SDState = APP_STATE_PAUSED;
                        displayState = APP_STATE_PAUSED;
                        currentState = APP_STATE_SELECT_TRACK;
                        SDEvent = currentEvent;
                        displayEvent = currentEvent;
                        currentEvent = APP_EVENT_NONE;
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

    // while (pcm_ring_level() < (PCM_RING_SIZE / 8)) {
    //     OSTimeDly(1u, OS_OPT_TIME_DLY, &err);
    // }
    
    // uint32_t fs_mp3 = MP3Player_GetSampleRateHz();
	//    gpioMode(PORTNUM2PIN(PC,10), OUTPUT);
	// gpioWrite(PORTNUM2PIN(PC,10), 1);
    

    OSSemPend(&g_mp3ReadySem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);
    Audio_Init();
    // antes de arrancar DMA/audio

    while (1) {
            if(isPlaying)
            {
                OSSemPend(&g_AudioSem, 0u, OS_OPT_PEND_BLOCKING, 0u, &err);
                Audio_Service();         
            }
            // else
            // {
            //     // STOP DMA?
            // }
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
                write_LCD(filenames[0], 1);
                break;
            case(APP_STATE_PAUSED):
                clear_LCD();
                write_LCD("Paused", 0);
                write_LCD(filenames[0], 1);
                break;
            case(APP_STATE_SELECT_TRACK):
                clear_LCD();
                write_LCD("Menu", 0);
                switch(displayEvent) {
                    case(APP_EVENT_ENC_RIGHT):
                        write_LCD("Encoder right", 1);
                        break;
                    case(APP_EVENT_ENC_LEFT):
                        write_LCD("Encoder left", 1);
                        break;
                    default:
                }
                break;
            default:
        }
        displayEvent = APP_EVENT_NONE;
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

    // // 3) Abrir MP3
    // fr = f_open(&g_song, "0:/TALKTO~1.MP3", FA_READ);
    // if (fr != FR_OK) {
    //     while (1) OSTimeDly(10u, OS_OPT_TIME_DLY, &err);
    // }


    // OSSemPost(&g_mp3ReadySem, OS_OPT_POST_1, &err);

    // while (pcm_ring_level() < (AUDIO_BUF_LEN * 12)) {   // 6 bloques, ajustá
    //     MP3Player_DecodeAsMuchAsPossibleToRing();
    // }

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
                        fr = f_open(&g_song, filenames[0], FA_READ);
                        if (fr != FR_OK) 
                            while (1) OSTimeDly(10u, OS_OPT_TIME_DLY, &err);
                        // 4) Inicializar Helix + buffers del player
                        if (!MP3Player_InitWithOpenFile(&g_song))
                            while (1) OSTimeDly(10u, OS_OPT_TIME_DLY, &err);

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


