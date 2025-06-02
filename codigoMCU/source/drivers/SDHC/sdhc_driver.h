#ifndef SDHC_DRIVER
#define SDHC_DRIVER

#include <fsl_sdhc.h>
#include <stdbool.h>

bool sdhc_is_initialized(void);
bool sdhc_is_card_inserted(void);
bool sdhc_init_card(void);


#endif //SDHC_DRIVER
