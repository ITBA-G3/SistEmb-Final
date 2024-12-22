#include "rtos/uCOSIII/src/uCOS-III/Source/os.h"
#include "gpio.h"
#include "tick.h"

typedef struct
{
	pinIrqFun_t callback_ptr;
	unsigned int period;
} pisr_callback_t;

static pisr_callback_t callbacks[MAX_FUNCTIONS];
static bool called = false;
static uint32_t index = 0;


 /*******************************************************************************
 * FUNCTION WITH GLOBAL SCOPE
 ******************************************************************************/

void App_OS_SetAllHooks (void)                             /* os_app_hooks.c         */
{
    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();
    OS_AppTimeTickHookPtr = PISR;
    CPU_CRITICAL_EXIT();
}

bool tickAdd (pinIrqFun_t funcallback, unsigned int period) {
	bool check = index < MAX_FUNCTIONS;
	if (check)
	{
		callbacks[index].period = period;
		callbacks[index].callback_ptr = funcallback;
		index++;
	}
	return check;
}

void PISR(void)
{
	static unsigned int counter = 0;
	for(int i = 0; i < index; i++)
	{
		if ((counter%(callbacks[i].period) == 0) && (callbacks[i].callback_ptr != 0))
		{
			(callbacks[i].callback_ptr)();
		}
	}
	counter++;
}
