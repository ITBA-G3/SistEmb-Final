#include "rtos/uCOSIII/src/uCOS-III/Source/os.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "hardware.h"
#include "gpio.h"
#include "UART/uart.h"
#include "board.h"
#include "tick.h"

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/


// Main Task
#define TASKMAIN_STK_SIZE 512u // Definir tamaño de stack a ser usado por la tarea
#define TASKMAIN_PRIO 2u       // Definir prioridad de la tarea
static OS_TCB TaskMainTCB;
static CPU_STK TaskMainStk[TASKMAIN_STK_SIZE];

/* Example semaphore */

// OS QUEUES

static void TaskMain(void *p_arg)
{
    (void)p_arg;
    OS_ERR os_err;

    /* Initialize the uC/CPU Services. */
    CPU_Init();

#if OS_CFG_STAT_TASK_EN > 0u
    /* (optional) Compute CPU capacity with no task running */
    OSStatTaskCPUUsageInit(&os_err);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

    // Initialize drivers

    // GPIOs
    
    OS_MSG_SIZE p_size;

    while (1)
    {
        //task xd
    }
}

int main(void)
{
    OS_ERR os_err;

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_ERR cpu_err;
#endif

    hw_Init();
    OSInit(&os_err);
#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
    /* Enable task round robin. */
    OSSchedRoundRobinCfg((CPU_BOOLEAN)1, 0, &os_err);
#endif
    // OS_CPU_SysTickInit(SystemCoreClock / (uint32_t)OSCfg_TickRate_Hz);

    OSTaskCreate(&TaskMainTCB,
                 "App Task Start",
                 TaskMain,
                 0u,
                 TASKMAIN_PRIO,
                 &TaskMainStk[0u],
                 (TASKMAIN_STK_SIZE / 10u),
                 TASKMAIN_STK_SIZE,
                 0u,
                 0u,
                 0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
                 &os_err);

    App_OS_SetAllHooks();
    OSStart(&os_err);

    /* Should Never Get Here */
    while (1)
    {
    }
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

