/***************************************************************************/ /**
   @file     main.c
   @brief    MP3 Player main
   @author   Grupo 3
  ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "drivers/pisr.h"
#include "drivers/PIT.h"
#include "drivers/SD/sd.h"
#include "drivers/Encoder/encoder.h"
#include "drivers/LCD/LCD.h"
#include "LEDmatrix.h"
#include "Visualizer.h"
#include "FFT.h"
#include "App.h"
#include "os.h"
#include "cpu.h"
#include "board.h"
#include "tick.h"
#include "MK64F12.h"
#include "gpio.h"

#include <stdio.h>
#include <math.h>
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#ifndef SystemCoreClock
#define SystemCoreClock 120000000U
#endif

// task priorities 
#define ENC_TASK_PRIO       4u
#define BTN_TASK_PRIO       5u
#define DISP_TASK_PRIO      8u
#define LED_TASK_PRIO       9u
#define SD_TASK_PRIO        6u

// stack sizes (check this values)
#define ENC_STK_SIZE        256u
#define BTN_STK_SIZE        256u
#define DISP_STK_SIZE       256u
#define LED_STK_SIZE        256u
#define SD_STK_SIZE         512u

/*******************************************************************************
 * RTOS OBJECTS DECLARATIONS
 ******************************************************************************/
static CPU_STK EncStk[ENC_STK_SIZE];
static CPU_STK BtnStk[BTN_STK_SIZE];
static CPU_STK DispStk[DISP_STK_SIZE];
static CPU_STK LedStk[LED_STK_SIZE];
static CPU_STK SdStk[SD_STK_SIZE];

static OS_TCB EncTCB;
static OS_TCB BtnTCB;
static OS_TCB DispTCB;
static OS_TCB LedTCB;
static OS_TCB SdTCB;

static OS_Q     UiQ;        /* UI event queue */
static OS_SEM   DispSem;    /* Display refresh semaphore */
static OS_MUTEX AppMtx;     /* Application state mutex */
/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/
void App_Init(void);
void App_Start(void);
void App_Run(void);
/*******************************************************************************
 * FUNCTION DEFINITIONS
 ******************************************************************************/
static void App_ModuleInit(void);
static void App_TaskCreate(void);

static void Encoder_Task(void *p_arg);
static void Buttons_Task(void *p_arg);
static void Display_Task(void *p_arg);
static void LedMatrix_Task(void *p_arg);
static void SD_Task(void *p_arg);

int main(void) {
    OS_ERR err;

    Board_Init();
    OSInit(&err);
    OS_CPU_SysTickInit(SystemCoreClock / (uint32_t)OSCfg_TickRate_Hz);
    
    CPU_Init();
    App_Init();
    App_Start();
    OSStart(&err);

    while (1) {
        App_Run();
    }
}

void App_Init(void) {
    App_ModuleInit();
    App_TaskCreate();
}

void App_Run(void) {

}

void App_Start(void) {
    Display_Init();
    LedMatrix_Init();
    SD_Init();
    Encoder_Init();
}

void App_ModuleInit(void) {
    // Initialize application modules here
    Display_HwInit();      // GPIO, reset pins
    LedMatrix_HwInit();    // GPIO direction, timers
    SD_HwInit();           // SD pins, power enable
    Encoder_HwInit();      // GPIO + EXTI config
}

static void App_TaskCreate(void) {
    OS_ERR err;

    // Create RTOS objects
    OSQCreate(&UiQ, "UiQ", 16u, &err);
    OSSemCreate(&DispSem, "DispSem", 0u, &err);
    OSMutexCreate(&AppMtx, "AppMtx", &err);

    // Create tasks
    OSTaskCreate(&EncTCB,
                 "Encoder Task",
                 Encoder_Task,
                 0,
                 ENC_TASK_PRIO,
                 &EncStk[0],
                 ENC_STK_SIZE / 10u,
                 ENC_STK_SIZE,
                 0u,
                 0u,
                 0u,
                 OS_OPT_TASK_STK_CHK,
                 &err);

    OSTaskCreate(&BtnTCB,
                 "Buttons Task",
                 Buttons_Task,
                 0,
                 BTN_TASK_PRIO,
                 &BtnStk[0],
                 BTN_STK_SIZE / 10u,
                 BTN_STK_SIZE,
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
                 LED_TASK_PRIO,
                 &LedStk[0],
                 LED_STK_SIZE / 10u,
                 LED_STK_SIZE,
                 0u,
                 0u,
                 0u,
                 OS_OPT_TASK_STK_CHK,
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
static void Encoder_Task(void *p_arg) {
    (void)p_arg;
    OS_ERR err;

    while (1) {
        // encoder state 

        OSTimeDlyHMSM(0u, 0u, 0u, 10u, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

static void Buttons_Task(void *p_arg){
    (void)p_arg;
    OS_ERR err;

    while (1) {
        // buttons state / debounce

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

    while (1) {
        // LED matrix

        OSTimeDlyHMSM(0u, 0u, 0u, 10u, OS_OPT_TIME_HMSM_STRICT, &err);
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
