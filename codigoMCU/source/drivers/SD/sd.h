#ifndef SD_H
#define SD_H

#pragma once
#include <stdint.h>
#include <stdbool.h>

#define SD_CMD_GO_IDLE_STATE        0   // CMD0
#define SD_CMD_SEND_IF_COND         8   // CMD8
#define SD_CMD_APP_CMD              55  // CMD55
#define SD_ACMD_SD_SEND_OP_COND     41  // ACMD41
#define SD_CMD_ALL_SEND_CID         2   // CMD2
#define SD_CMD_SEND_REL_ADDR        3   // CMD3
#define SD_CMD_SELECT_CARD          7   // CMD7
#define SD_CMD_SET_BLOCKLEN         16  // CMD16

#define SD_BLOCK_SIZE 512

typedef struct {
    uint16_t rca;       // RCA asignado por CMD3
    bool is_sdhc;       // CCS = 1 => block addressing
    bool initialized;
    uint32_t ocr;      // OCR devuelto por ACMD41 (R3)
} sd_card_t;

typedef enum {
    SD_OK = 0,
    SD_ERR_TIMEOUT,
    SD_ERR_ILLEGAL_CMD,
    SD_ERR_IO,
    SD_ERR_CRC,
    SD_ERR_PARAM,
    SD_ERR_NOT_READY,
    SD_ERR_HOST,
} sd_error_t;

// Inicialización de tarjeta SD en modo 1-bit, usando el host SDHC. Asume SDHC inicializado
sd_error_t sd_init(sd_card_t *card);

sd_error_t sd_read_blocks(sd_card_t *card, uint32_t lba, uint32_t *buf_w, uint32_t block_count);
sd_error_t sd_write_blocks(sd_card_t *card, uint32_t lba, const uint32_t *buf_w, uint32_t block_count);

#endif //SD_H
