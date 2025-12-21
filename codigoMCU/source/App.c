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

// AUDIO
volatile bool PIT_trigger;
volatile bool DMA_trigger;

//SD
volatile bool playingFlag = false;

volatile uint16_t bufA[AUDIO_BUF_LEN];
volatile uint16_t bufB[AUDIO_BUF_LEN];

bool isPlaying = false;

// LED MATRIX
static LEDM_t *matrix;
static volatile bool start_new_frame;

static void make_test_pcm(int16_t *pcm, uint32_t fs_hz);
static void PIT_cb(void);
static void ws2_dump_ftm_dma(void);

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#ifndef SystemCoreClock
// #define SystemCoreClock 120000000U
#endif

// task priorities 
#define MAIN_TASK_PRIO      2u
#define AUDIO_TASK_PRIO     3u
#define SD_TASK_PRIO        4u
#define DISP_TASK_PRIO      5u
#define LEDMATRIX_TASK_PRIO       6u

// stack sizes (check this values)
#define MAIN_STK_SIZE               1024u
#define AUDIO_STK_SIZE              256u
#define SD_STK_SIZE                 512u
#define DISP_STK_SIZE               2048u
#define LEDMATRIX_STK_SIZE          2048u

typedef enum {
    APP_STATE_PLAYING = 0,
    APP_STATE_PAUSED,
    APP_STATE_SELECT_TRACK,
} controlState_t;
static controlState_t currentState = APP_STATE_SELECT_TRACK;

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

void App_Init(void)
{
    // OS Task creation & init
    App_TaskCreate();
}

void App_Run(void)
{

}

static void App_TaskCreate(void)
{
    OS_ERR err;

    // Create RTOS objects
//    OSQCreate(&UiQ, "UiQ", 16u, &err);
//    OSSemCreate(&DispSem, "DispSem", 0u, &err);
//    OSMutexCreate(&AppMtx, "AppMtx", &err);

	OSSemCreate(&LedFrameSem,
	                "LedFrameSem",
	                0u,                     // empieza en 0 → nadie pasa hasta el 1er PIT
	                &err);
    OSSemCreate(&DisplaySem,
                    "Display semaphore",
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

    gpioMode(PIN_LED_BLUE, OUTPUT);
    gpioWrite(PIN_LED_BLUE, 1);
    gpioMode(PIN_LED_GREEN, OUTPUT);
    gpioWrite(PIN_LED_GREEN, 1);
    gpioMode(PIN_LED_RED, OUTPUT);
    gpioWrite(PIN_LED_RED, 1);
    gpioMode(PORTNUM2PIN(PE, 25), OUTPUT);

    while (1)
    {
        /****************************************** BUTTON EVENTS ******************************************/ 
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
        /**************************************************************************************************/ 
        
        /**************************************** ENCODER EVENTS ******************************************/
        if(getTurns() > 0) 
            currentEvent = APP_EVENT_ENC_RIGHT;
        if(getTurns() < 0) 
            currentEvent = APP_EVENT_ENC_LEFT;
        if(getSwitchState() == BTN_CLICK) 
            currentEvent = APP_EVENT_ENC_BUTTON;
        if(getSwitchState() == BTN_LONG_CLICK) 
            currentEvent = APP_EVENT_ENC_BUTTON;
        /**************************************************************************************************/

        switch (currentState){
        	case APP_STATE_SELECT_TRACK:
                // falta hacer el menu literalmente, 
                // escribir cosas en el display

                if(currentEvent == APP_EVENT_ENC_BUTTON){
                    OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                    // guardar referencia a la canción que va a empezar a reproducir (SD)
                    // empezar transferencia SD → audio (SD decoder + audio + ledMatrix)
                    
                    playingFlag = true;
                    currentState = APP_STATE_PLAYING;
                    currentEvent = APP_EVENT_NONE;
                }
                else if(currentEvent == APP_EVENT_ENC_RIGHT){
                    OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                    // actualiza display con la siguiente cancion
                    // puntero a la siguiente cancion
                    // SD browsing (metadata)
                    currentEvent = APP_EVENT_NONE;
                }
                else if(currentEvent == APP_EVENT_ENC_LEFT){
                    OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                    // actualiza display con la cancion anterior
                    // puntero a la cancion anterior
                    // SD browsing (metadata)
                    currentEvent = APP_EVENT_NONE;
                }
        		break;

        	case APP_STATE_PLAYING:
                /* coming soon... */                
                if(currentEvent == APP_EVENT_NEXT_TRACK){
                    OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                    // empezar a reproducir el siguiente track
                    currentEvent = APP_EVENT_NONE;
                }
                else if(currentEvent == APP_EVENT_PREV_TRACK){
                    OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                    // empezar a reproducir el track anterior
                    currentEvent = APP_EVENT_NONE;
                }

                // transitions between states
        		if(currentEvent == APP_EVENT_PAUSE){
                    OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                    // pausar transferencia SD → audio (SD decoder + audio + ledMatrix)
                    // actualiza display para mostrar estado de pausa
                    playingFlag = false;
        			currentState = APP_STATE_PAUSED;
        			currentEvent = APP_EVENT_NONE;
        		}
                if(currentEvent == APP_EVENT_ENC_BUTTON ||
                   currentEvent == APP_EVENT_ENC_LEFT ||
                   currentEvent == APP_EVENT_ENC_RIGHT){
                    OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                    // pausar transferencia SD → audio (SD decoder + audio + ledMatrix)
                    // actualiza display con el menu
                    // puntero a la cancion anterior
                    // SD browsing (metadata)
                    playingFlag = false;
                    currentState = APP_STATE_SELECT_TRACK;
                    currentEvent = APP_EVENT_NONE; //queda esperando alguna accion del encoder
                }
        		break;
        	case APP_STATE_PAUSED:
                // transitions between states
        		if(currentEvent == APP_EVENT_PLAY){
                    OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                    // empezar transferencia SD → audio desde donde quedo (SD decoder + audio + ledMatrix)
                    // actualiza display para mostrar estado de playing
                    playingFlag = true;
        			currentState = APP_STATE_PLAYING;
        			currentEvent = APP_EVENT_NONE;
        		}
                if(currentEvent == APP_EVENT_ENC_BUTTON ||
                   currentEvent == APP_EVENT_ENC_LEFT ||
                   currentEvent == APP_EVENT_ENC_RIGHT){
                    OSSemPost(&DisplaySem, OS_OPT_POST_1, &err);
                    // ir a seleccion de track
                    playingFlag = false;
                    currentState = APP_STATE_SELECT_TRACK;
                    currentEvent = APP_EVENT_NONE;
                }
                
                /* coming soon... */
                /*
                if(currentEvent == APP_EVENT_NEXT_TRACK){
                    // empezar a reproducir el siguiente track
                    currentState = APP_STATE_PLAYING;
                    currentEvent = APP_EVENT_NONE;
                }
                if(currentEvent == APP_EVENT_PREV_TRACK){
                    // empezar a reproducir el track anterior
                    currentState = APP_STATE_PLAYING;
                    currentEvent = APP_EVENT_NONE;
                }
                */
        		break;
        }
        OSTimeDlyHMSM(0u, 0u, 0u, 20u, OS_OPT_TIME_HMSM_STRICT, &err);

    }
}


static void Audio_Task(void *p_arg)
{
    (void)p_arg;
    OS_ERR err;

	 //Enable port clock for DAC pin
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

	 //Put DAC pin in pure analog mode
	PORTB->PCR[2] = 0;   // adjust pin number to your board

	Audio_Init();
    

    while (1)
    {
        // buttons state / debounce

        if (playingFlag)
        {
            Audio_Service();

            if(PIT_trigger)
            {
                PIT_trigger = false;
            }
            if(DMA_trigger)
            {
                DMA_trigger = false;
            }
        }

        OSTimeDlyHMSM(0u, 0u, 0u, 10u, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

static void Display_Task(void *p_arg)
{
    (void)p_arg;
    OS_ERR err;

    while (1) {
        // Display
    	OSSemPend(&DisplaySem, 0u, OS_OPT_PEND_BLOCKING, 0u, &err);
        switch(currentState) {
            case(APP_STATE_PLAYING):
                write_LCD("Playing", 0);
                break;
            case(APP_STATE_PAUSED):
                write_LCD("Paused", 0);
                break;
            case(APP_STATE_SELECT_TRACK):
                write_LCD("Menu", 0);
                switch(currentEvent) {
                    case(APP_EVENT_ENC_RIGHT):
                        write_LCD("Encoder right", 1);
                        break;
                    case(APP_EVENT_ENC_LEFT):
                        write_LCD("Encoder right", 1);
                        break;
                    default:
                }
                break;
            default:
        }
        OSTimeDlyHMSM(0u, 0u, 0u, 20u, OS_OPT_TIME_HMSM_STRICT, &err);
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

	// For debugging
	gpioMode(PORTNUM2PIN(PC,10), OUTPUT);
	gpioWrite(PORTNUM2PIN(PC,10), 1);
	gpioMode(PORTNUM2PIN(PC,11), OUTPUT);
	gpioWrite(PORTNUM2PIN(PC,11), 0);

    static int16_t frame[FFT_N];
	static float bands[8];

    while (1) {
    	OSSemPend(&LedFrameSem, 0u, OS_OPT_PEND_BLOCKING, 0u, &err);

		LEDM_SetBrightness(matrix, 2);

		// FOR TESTING
		make_test_pcm(frame, AUDIO_FS_HZ);

		FFT_ComputeBands(frame, FFT_N, AUDIO_FS_HZ, bands);

//		Visualizer_UpdateFrame(matrix);
		Visualizer_DrawBars(bands, matrix);

		bool ok = LEDM_Show(matrix);

		if(ok){
			while (LEDM_TransferInProgress()) {
			    OSTimeDly(5u, OS_OPT_TIME_DLY, &err);
			}
		}
    }
}

static void SD_Task(void *p_arg)
{
    (void)p_arg;
    OS_ERR err;

    while (1) {
        // SD
        if(playingFlag){
            // leer datos del archivo actual en SD
            // llenar buffers de audio y ledMatrix
        }

        OSTimeDlyHMSM(0u, 0u, 0u, 200u, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

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

/********************************
 *      PIT CALLBACKS
 ********************************/

static void PIT_cb(void)
{
	OS_ERR err;
	OSSemPost(&LedFrameSem, OS_OPT_POST_1, &err);
//	gpioToggle(PORTNUM2PIN(PC,11));
}


static void ws2_dump_ftm_dma(void)
{
    volatile uint32_t ftm_sc   = FTM0->SC;
    volatile uint32_t ftm_mod  = FTM0->MOD;
    volatile uint32_t ftm_c0sc = FTM0->CONTROLS[0].CnSC;
    volatile uint32_t ftm_c0v  = FTM0->CONTROLS[0].CnV;

    volatile uint32_t erq    = DMA0->ERQ;
    volatile uint8_t  mux0   = DMAMUX0->CHCFG[0];
    volatile uint16_t citer  = DMA0->TCD[0].CITER_ELINKNO;
    volatile uint16_t biter  = DMA0->TCD[0].BITER_ELINKNO;
    volatile uint32_t csr    = DMA0->TCD[0].CSR;

    (void)ftm_sc; (void)ftm_mod; (void)ftm_c0sc; (void)ftm_c0v;
    (void)erq; (void)mux0; (void)citer; (void)biter; (void)csr;
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
    extern uint32_t SystemCoreClock;
    
    OS_CPU_SysTickInit(SystemCoreClock / (uint32_t)OSCfg_TickRate_Hz);

    CPU_Init();
    App_Init();
    App_OS_SetAllHooks();

    OSStart(&err);

    /* Should Never Get Here */
    while (1);
}


