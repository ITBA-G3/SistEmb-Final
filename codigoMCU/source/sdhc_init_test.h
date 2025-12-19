/*
 * sdhc_init_test.h
 *
 *  Created on: 12 Dec 2025
 *      Author: lucia
 */

#ifndef SDHC_INIT_TEST_H_
#define SDHC_INIT_TEST_H_

#include <stdint.h>
#include <stdbool.h>
#include "drivers/SDHC/sdhc.h"

typedef struct {
    uint16_t rca;      // Relative Card Address
    bool     sdhc;     // true = SDHC/SDXC (addressing por bloque), false = SDSC (addressing por byte)
    uint32_t ocr;      // OCR devuelto por ACMD41 (R3)
} sd_card_t;

#define SD_CMD_GO_IDLE_STATE        0   // CMD0
#define SD_CMD_SEND_IF_COND         8   // CMD8
#define SD_CMD_APP_CMD              55  // CMD55
#define SD_ACMD_SD_SEND_OP_COND     41  // ACMD41
#define SD_CMD_ALL_SEND_CID         2   // CMD2
#define SD_CMD_SEND_REL_ADDR        3   // CMD3
#define SD_CMD_SELECT_CARD          7   // CMD7
#define SD_CMD_SET_BLOCKLEN         16  // CMD16 (solo relevante en SDSC normalmente)



sdhc_error_t sd_card_init_full(sd_card_t *card);
sdhc_error_t sd_read_single_block_adma2(uint32_t block, uint8_t *buf512);
#endif /* SDHC_INIT_TEST_H_ */
