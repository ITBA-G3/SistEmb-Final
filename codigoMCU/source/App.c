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
#include "ticks.h"
#include "MK64F12.h"
#include "drivers/gpio.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "hardware.h"

// AUDIO
volatile bool PIT_trigger;
volatile bool DMA_trigger;

volatile uint16_t bufA[AUDIO_BUF_LEN];
volatile uint16_t bufB[AUDIO_BUF_LEN];

// LED MATRIX
static LEDM_t *matrix;
static volatile bool start_new_frame;

static void make_test_pcm(int16_t *pcm, uint32_t fs_hz);
static void PIT_cb(void);

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#ifndef SystemCoreClock
#define SystemCoreClock 120000000U
#endif

// task priorities 
#define MAIN_TASK_PRIO      2u
#define AUDIO_TASK_PRIO     3u
#define SD_TASK_PRIO        4u
#define DISP_TASK_PRIO      5u
#define LEDMATRIX_TASK_PRIO       6u

// stack sizes (check this values)
#define MAIN_STK_SIZE               256u
#define AUDIO_STK_SIZE              256u
#define SD_STK_SIZE                 512u
#define DISP_STK_SIZE               2048u
#define LEDMATRIX_STK_SIZE          2048u

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

static OS_Q     UiQ;        /* UI event queue */\
static OS_MUTEX AppMtx;     /* Application state mutex */


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

void App_Init(void) {
    App_TaskCreate();


}

void App_Run(void) {

}

static void App_TaskCreate(void) {
    OS_ERR err;

    // Create RTOS objects
//    OSQCreate(&UiQ, "UiQ", 16u, &err);
//    OSSemCreate(&DispSem, "DispSem", 0u, &err);
//    OSMutexCreate(&AppMtx, "AppMtx", &err);

	OSSemCreate(&LedFrameSem,
	                "LedFrameSem",
	                0u,                     // empieza en 0 → nadie pasa hasta el 1er PIT
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
}

/* TASKS */
static void Main_Task(void *p_arg) {
    (void)p_arg;
    OS_ERR err;

    while (1) {
        // Display

        OSTimeDlyHMSM(0u, 0u, 0u, 20u, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}


static void Audio_Task(void *p_arg){
    (void)p_arg;
    OS_ERR err;

	 //Enable port clock for DAC pin
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

	 //Put DAC pin in pure analog mode
	PORTB->PCR[2] = 0;   // adjust pin number to your board

	Audio_Init();
	__enable_irq();

    while (1) {
        // buttons state / debounce

    	Audio_Service();

        if(PIT_trigger){
        	PIT_trigger = false;
        }
        if(DMA_trigger){
        	DMA_trigger = false;
        }

        OSTimeDlyHMSM(0u, 0u, 0u, 10u, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

static void Display_Task(void *p_arg) {
    (void)p_arg;
    OS_ERR err;

    while (1) {
        // Display

        OSTimeDlyHMSM(0u, 0u, 0u, 20u, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

static void LedMatrix_Task(void *p_arg) {
    (void)p_arg;
    OS_ERR err;

    //	LEDMATRIX TEST
	matrix = LEDM_Init(8, 8);

	FFT_Init();
	PIT_Init(PIT_0, 10);		// PIT 0 para controlar FPS y refrescar matrix de leds,
	PIT_SetCallback(PIT_cb, PIT_0);

	gpioMode(PORTNUM2PIN(PC,10), OUTPUT);
	gpioWrite(PORTNUM2PIN(PC,10), 1);


    static int16_t frame[FFT_N];
	static float bands[8];

	LEDM_SetBrightness(matrix, 8);

    while (1) {
        // LED matrix
    	// LED MATRIX TEST


    	OSSemPend(&LedFrameSem, 0u, OS_OPT_PEND_BLOCKING, 0u, &err);

		make_test_pcm(frame, AUDIO_FS_HZ);

		FFT_ComputeBands(frame, FFT_N, AUDIO_FS_HZ, bands);
    	gpioToggle(PORTNUM2PIN(PC,10));

//		Visualizer_UpdateFrame(matrix);
		Visualizer_DrawBars(bands, matrix);

		bool ok = LEDM_Show(matrix);

//		if(ok){
//			while(LEDM_TransferInProgress()){
//				OSTimeDly(1u, OS_OPT_TIME_DLY, &err);
//			}
//		}

        OSTimeDlyHMSM(0u, 0u, 0u, 5u, OS_OPT_TIME_HMSM_STRICT, &err);

		LEDM_Clear(matrix);


    }
}

static void SD_Task(void *p_arg) {
    (void)p_arg;
    OS_ERR err;

    while (1) {
        // SD

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

static void PIT_cb(void){
	OS_ERR err;
	OSSemPost(&LedFrameSem, OS_OPT_POST_1, &err);

//	start_new_frame = true;
}


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

//    // // OS QUEUES
//     OSQCreate(&QMagReader, "QMagReader", (OS_MSG_QTY) QUEUE_SIZE, &os_err);
//     OSQCreate(&QEncoder, "QEncoder", (OS_MSG_QTY) QUEUE_SIZE, &os_err);
//     OSQCreate(&QDisplay, "QDisplay", (OS_MSG_QTY) QUEUE_SIZE, &os_err);

    App_OS_SetAllHooks();
    OSStart(&err);

    /* Should Never Get Here */
    while (1)
    {
    }
}
