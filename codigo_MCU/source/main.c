#include "rtos/uCOSIII/src/uCOS-III/Source/os.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "hardware.h"
#include "mainTask.h"
#include "gpio.h"
#include "UART/uart.h"
#include "board.h"
#include "drivers/encoder/encoder.h"
#include "drivers/display/display.h"
#include "drivers/display/display_board.h"
#include "drivers/magnetic_reader/magnetic_reader.h"
#include "tick.h"

#define QUEUE_SIZE 10
#define BUILDING_SIZE 3

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

static UserProfile_t currentUser;
static UserProfile_t *currentUserPointer;

static uint8_t state;
static uint8_t stateSelecting;
static uint8_t periodTime; // contador que se incrementa con la interupción PISR de getTime().
static uint8_t deleteTime;
static uint8_t *SymPtr;
static uint8_t symIndex;
static uint8_t encAction;
static uint8_t numCounter;
static uint8_t rotating;

static bool buzzerFlag = false;

// Main Task
#define TASKMAIN_STK_SIZE 512u // Definir tamaño de stack a ser usado por la tarea
#define TASKMAIN_PRIO 2u       // Definir prioridad de la tarea
static OS_TCB TaskMainTCB;
static CPU_STK TaskMainStk[TASKMAIN_STK_SIZE];

// Cloud Task
#define TASKCLOUD_STK_SIZE 256u                             // Definir tamaZño de stack a ser usado por la tarea
#define TASKCLOUD_STK_SIZE_LIMIT (TASKCLOUD_STK_SIZE / 10u)
#define TASKCLOUD_PRIO 3u                                   // Definir prioridad de la tarea
static OS_TCB TaskCloudTCB;
static CPU_STK TaskCloudStk[TASKCLOUD_STK_SIZE];

/* Example semaphore */
static OS_SEM semCloud;

// OS QUEUES
static OS_Q QMagReader;
static OS_Q QEncoder;
static OS_Q QDisplay;

static OS_SEM* semaphore;

/*
* SendData: AA 55 C3 3C 07 01 pp pp qq qq rr rr
* SendDataOK: AA 55 C3 3C 01 81
* SendDataFail: AA 55 C3 3C 01 C1
*
* KeepAlive: AA 55 C3 3C 01 02         
* KeepAliveOk: AA 55 C3 3C 01 82
*/

static void TaskCloud(void *p_arg)
{ // Definir funciOn de la tarea cloud
    (void)p_arg;
    OS_ERR os_err;

    // UARTxConfig_t uartConfig = {
	// 	.baudrate = 1200,
	// 	.stop_bits = UART_STOP_BITS_1,
	// 	.parity = 0,
	// 	.data_bits = UART_DATA_BITS_8,
	// 	.mode = UART_MODE_NORMAL};

	// UARTinit(UART_0, &uartConfig);

    uart_cfg_t config = {.baudrate=1200, .parity=true};

    uartInit(UART_0, config, semaphore);

    static uint8_t floorCapacity [BUILDING_SIZE] = {0,0,0};
    static uint8_t data [12] = {0xAA, 0x55, 0xC3, 0x3C, 0x07, 0x01, 0, 0, 0, 0, 0, 0};

    while (1)
    {
        OSSemPend(&semCloud, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
        for (int i=0; i<BUILDING_SIZE; i++)
            floorCapacity[i]=0;

        for (int i=0; i<MAX_NUM_USERS; i++)
            usersLUT[i].inBuilding? floorCapacity[usersLUT[i].floor - 1]++ : false;

        for(int i=0; i<BUILDING_SIZE; i++)
            for(int j=0; j<2; j++)
                data[6+j+i*sizeof(uint16_t)] = (uint8_t) ((floorCapacity[i]>>(8*(j))) & 0x00FF);

        for(int i=0; i<12; i++) {
			uartWriteMsg2(data[i]);
            OSTimeDlyHMSM(0u, 0u, 0u, 100u, OS_OPT_TIME_HMSM_STRICT, &os_err); // Definir tiempo de espera
		}

        char msg2;
        char rx;
		for(int i=0; i<6 ; i++) { //Busco la respuesta del thingSpeak, me interesa únicamente el último
			OSSemPend(semaphore, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
			msg2 = uartReadMsg(UART_0, &rx, 1);
		}
        if(msg2 == 0x81)
        {
            // LED amarillo 
            gpioWrite(PIN_LED_RED,LOW);
            gpioWrite(PIN_LED_GREEN,LOW);
            gpioWrite(PIN_LED_BLUE,HIGH);
        }
        else{
            // LED violeta
            gpioWrite(PIN_LED_RED,LOW);
            gpioWrite(PIN_LED_GREEN,HIGH);
            gpioWrite(PIN_LED_BLUE,LOW);
        }
    }
}

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

    // Create TaskCloud
    OSTaskCreate(&TaskCloudTCB,            // tcb
                 "Task Cloud",             // name
                 TaskCloud,                // func
                 0u,                       // arg
                 TASKCLOUD_PRIO,           // prio
                 &TaskCloudStk[0u],        // stack
                 TASKCLOUD_STK_SIZE_LIMIT, // stack limit
                 TASKCLOUD_STK_SIZE,       // stack size
                 0u,
                 0u,
                 0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 &os_err);

    // Create semaphore
    OSSemCreate(&semCloud, "Cloud Semaphore", 0, &os_err);

    // Initialize drivers
    encoderInit(&QEncoder);
    dispInit();
    magnetic_reader_init(&QMagReader);
    tickAdd(getTime, 1);

    // GPIOs
    gpioMode(PIN_LED_BLUE, OUTPUT);
    gpioMode(PIN_LED_RED, OUTPUT);
    gpioMode(PIN_LED_GREEN, OUTPUT);
    gpioMode(PIN_PISR, OUTPUT);
    gpioMode(PIN_DISR, OUTPUT);
    gpioMode(PIN_BUZZER, OUTPUT);
    gpioMode(PIN_INFO_LED_0, OUTPUT);
    gpioMode(PIN_INFO_LED_1, OUTPUT);

    state = LOGIN;
    stateSelecting = BEGIN_SELECTING;

    SymPtr = getSymbArr();
    currentUserPointer = &currentUser;
    symIndex = 0;

    clearDisplay();

    static bool *cardDetected = 0;

    OS_MSG_SIZE p_size;

    while (1)
    {
        switch (state)
        {
        case LOGIN: // Estado inicial, el usuario debe ingresar su ID, si es válido pasa a VALID_ID si no a INVALID_ID. Cada 15s se resetea.

            login_LED();
            clearDisplay();

            cardDetected = OSQPend(&QMagReader, 0, OS_OPT_PEND_BLOCKING, &p_size, NULL, &os_err);
            if (*cardDetected == false)     // False por el flanco. el post se hace junto con el cambio de "enable"
            { 
                if (getCardStatus())
                {
                    resetCurrentUser();
                    currentUser.primaryAccount = getPrimaryAccount();

                    for (int i = 0; i < MAX_NUM_USERS; i++)
                    {
                        if (currentUser.primaryAccount == usersLUT[i].primaryAccount)
                        {
                            // Assign usersLUT[i] data to currentUser
                            // currentUser.index = usersLUT[i].index;
                            currentUser.index = i;
                            for (int j = 0; j < 8; j++)
                            {
                                currentUser.ID[j] = usersLUT[i].ID[j];
                                currentUser.ID_symbolIndex[j] = usersLUT[i].ID[j];
                            }
                            for (int j = 0; j < 5; j++)
                                currentUser.PIN[j] = usersLUT[i].PIN[j];

                            // Display
                            beginRotation(&usersLUT[i].ID_symbolIndex, 1);

                            // Update states, flags and counters
                            state = VALID_ID;
                            // cardDetected = true;
                            rotating = 0;
                            periodTime = 0;
                            numCounter = 0;
                            break;
                        }
                    }
                }
            }
            break;

        case VALID_ID: // En este estado ya se ingresó un ID válida y se espera al pin. Si el PIN es válido pasa a VALID_PIN, si no pasa a INVALID_PIN.

            validID_LED();

            // Check PIN
            switch (stateSelecting)
            {
            case BEGIN_SELECTING:
                OSTimeDlyHMSM(0u,0u,4u,0u,OS_OPT_TIME_HMSM_STRICT, &os_err);
                symIndex = 0;
                stateSelecting = SELECTING;
                break;
            case SELECTING:
                static uint8_t symIndex = 0;
                static uint8_t* OSQ_encAction = 0;
                
                writeDig(symIndex, 3);

                OSQ_encAction = OSQPend(&QEncoder, 0, OS_OPT_PEND_NON_BLOCKING, &p_size, NULL, &os_err);
                encAction = *OSQ_encAction;

                if (encAction == ENC_LEFT)
                {
                    if (symIndex > 0)
                    {
                        symIndex--;
                    }
                    resetEncDir();
                }
                else if (encAction == ENC_RIGHT)
                {
                    if (symIndex < 9)
                    {
                        symIndex++;
                    }
                    resetEncDir();
                }
                else if (encAction == SW_FLANK)
                { // Si el switch se apretó y es distinto al último estado que tenía el switch entra. (Detecta flanco)
                    periodTime = 0; // si se presionó el botón reseteo el tiempo

                    if (numCounter < 6)
                    {
                        currentUser.PIN[numCounter] = SymPtr[symIndex];
                        writeDig(28, 2);
                        writeDig(numCounter >= 1 ? 28 : 32, 1); // 28 es el 'indice de D_DASH :/ indica que se ingresó un símbolo
                        writeDig(numCounter >= 2 ? 28 : 32, 0); // 32 es el 'indice de D_OFF'

                        numCounter++;

                        if ((numCounter == 4) && (usersLUT[currentUserPointer->index].isPIN4 == 1))
                        {
                            if (checkPIN(currentUserPointer))
                            {
                                state = VALID_PIN;
                                periodTime = 0; // Para contar el tiempo que va a permanecer en ese estado.
                            }
                            else
                            {
                                state = INVALID_PIN;
                                periodTime = 0;
                            }
                        }
                    }
                    if (numCounter == 5 && (usersLUT[currentUserPointer->index].isPIN4 == 0))
                    {
                        currentUser.PIN[4] = SymPtr[symIndex];

                        clearDisplay();

                        if (checkPIN(currentUserPointer))
                        {
                            state = VALID_PIN;
                            periodTime = 0;
                        }
                        else
                        {
                            state = INVALID_PIN;
                            periodTime = 0;
                        }
                        numCounter = 0;
                    }
                    symIndex = 0;
                }
                break;
            }

            if (periodTime == 10000) // Se espera 10 segundos para ingresar el PIN.
            {
                state = LOGIN;
                resetCurrentUser();
                clearDisplay();
            }
            break;

        case INVALID_ID:
            invalid_LED();
            uint8_t invalidID[] = {21, 22, 32, 18, 13, 32, 32, 32}; // Se muestra "nO ID" en pantalla
            uint8_t *invalidIDPtr = &invalidID[0];
            beginRotation(invalidIDPtr, 1);

            if (periodTime > 5000)
            {
                clearDisplay();
                state = LOGIN;
            }
            break;

        case VALID_PIN:
            usersLUT[currentUserPointer->index].inBuilding = usersLUT[currentUserPointer->index].inBuilding? false : true;
            OSSemPost(&semCloud, OS_OPT_POST_1, &os_err);
            valid_LED();
			OSTimeDlyHMSM(0,0,3,0,OS_OPT_TIME_HMSM_STRICT, &os_err);
			state = LOGIN;
			symIndex = 0;
            break;

        case INVALID_PIN:
            invalid_LED();
            uint8_t invalid_PIN[] = {13, 14, 21, 18, 14, 13, 32, 32}; // Se muestra "dEnIEd" en pantalla
            uint8_t *invalPINPtr = &invalid_PIN[0];
            beginRotation(invalPINPtr, 1);
            OSTimeDlyHMSM(0,0,3,0,OS_OPT_TIME_HMSM_STRICT, &os_err);
            stopRotation();
            state = LOGIN;
            break;
        }
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
    OS_CPU_SysTickInit(SystemCoreClock / (uint32_t)OSCfg_TickRate_Hz);

    // OS QUEUES
    OSQCreate(&QMagReader, "QMagReader", (OS_MSG_QTY) QUEUE_SIZE, &os_err);
    OSQCreate(&QEncoder, "QEncoder", (OS_MSG_QTY) QUEUE_SIZE, &os_err);
    OSQCreate(&QDisplay, "QDisplay", (OS_MSG_QTY) QUEUE_SIZE, &os_err);

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

bool checkID()
{
    for (int i = 0; i <= MAX_NUM_USERS - 1; i++)
    {
        if (areArraysEqual(currentUserPointer->ID, usersLUT[i].ID, 8))
        {
            currentUserPointer->index = i; // guardo el índice del usuario ingresado para no tener que recorrer el arreglo de usuarios cada vez que necesite acceder a él.
            return true;
        }
    }
    return false;
}

bool checkPIN()
{
    if (areArraysEqual(currentUserPointer->PIN, usersLUT[currentUserPointer->index].PIN, 4))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool areArraysEqual(uint8_t *Arr2Check, uint8_t *Arr2Compare, int length)
{
    for (int i = 0; i < length; i++)
    {
        if (Arr2Check[i] != Arr2Compare[i])
        {
            return false;
        }
    }
    return true;
}

void resetCurrentUser(void)
{
    for (int i = 0; i < 8; i++)
    {
        currentUser.ID[i] = 0;
        currentUser.ID_symbolIndex[i] = 0;
    }
    for (int i = 0; i < 5; i++)
        currentUser.PIN[i] = 0;
}

void getTime(void)
{
    periodTime++;
    deleteTime++;
}

/********************************************
 *        LED STATE INDICATOR FUNCTIONS     *
 *******************************************/

void login_LED(void)
{
    gpioWrite(PIN_LED_RED, LOW);
    gpioWrite(PIN_LED_BLUE, LOW);
    gpioWrite(PIN_LED_GREEN, LOW);
}
void invalid_LED(void)
{
    gpioWrite(PIN_LED_RED, LOW);
    gpioWrite(PIN_LED_BLUE, HIGH);
    gpioWrite(PIN_LED_GREEN, HIGH);
    writeInfoLed(3);
}

void valid_LED(void)
{
    gpioWrite(PIN_LED_RED, HIGH);
    gpioWrite(PIN_LED_BLUE, HIGH);
    gpioWrite(PIN_LED_GREEN, LOW);
    writeInfoLed(1);
    buzzerFlag = true;
}

void validID_LED(void)
{
    gpioWrite(PIN_LED_RED, HIGH);
    gpioWrite(PIN_LED_BLUE, LOW);
    gpioWrite(PIN_LED_GREEN, LOW);
    writeInfoLed(1);
}

void admin_LED(void)
{
    gpioWrite(PIN_LED_RED, HIGH);
    gpioWrite(PIN_LED_BLUE, LOW);
    gpioWrite(PIN_LED_GREEN, HIGH);
    writeInfoLed(2);
}

/**************************************
 *          ADMIN FUNCTIONS           *
 **************************************/

void printError(void)
{
    uint8_t ERRORword[] = {14, 25, 25, 22, 25, 32, 32, 32}; // Se muestra "ErrOr" en pantalla
    uint8_t *ERRORwordPtr = &ERRORword[0];
    beginRotation(ERRORwordPtr, 1);
}
