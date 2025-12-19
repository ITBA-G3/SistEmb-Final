/*
 * sdhc_init_test.c
 *
 *  Created on: 12 Dec 2025
 *      Author: lucia
 */

#include "sdhc_init_test.h"
#include "drivers/SDHC/sdhc.h"
#include "hardware.h"
#include "MK64F12.h"
#include <string.h>
#include "drivers/SDHC/sdhc.h"



static inline sdhc_error_t sd_send_cmd(uint8_t idx, uint32_t arg,
                                      sdhc_response_type_t respType,
                                      sdhc_command_t *out_cmd)
{
    sdhc_command_t cmd = {0};
    cmd.index = idx;
    cmd.argument = arg;
    cmd.commandType = SDHC_COMMAND_TYPE_NORMAL;
    cmd.responseType = respType;

    sdhc_error_t e = sdhc_transfer(&cmd, NULL);
    if (out_cmd) *out_cmd = cmd;
    return e;
}

static inline sdhc_error_t sd_send_acmd(uint32_t rca16, uint8_t acmd_idx, uint32_t acmd_arg,
                                       sdhc_response_type_t acmd_resp,
                                       sdhc_command_t *out_acmd)
{
    // CMD55: APP_CMD, argumento = RCA<<16 (si RCA aún no existe, se manda 0)
    sdhc_error_t e = sd_send_cmd(SD_CMD_APP_CMD, (uint32_t)rca16 << 16, SDHC_RESPONSE_TYPE_R1, NULL);
    if (e != SDHC_ERROR_OK) return e;

    // ACMDxx
    return sd_send_cmd(acmd_idx, acmd_arg, acmd_resp, out_acmd);
}

static sdhc_error_t sd_send_cmd0_with_retry(void)
{
    const uint32_t RETRIES = 5;

    for (uint32_t attempt = 0; attempt < RETRIES; attempt++)
    {
        // Clear pending IRQ flags (W1C)
        SDHC->IRQSTAT = 0xFFFFFFFFu;

        // Give the card extra clocks/time to wake up
        sdhc_initialization_clocks();

        // Wait host not inhibited (recommended by RM flow)
        uint32_t t = 0xFFFFFFu;
        while (t--)
        {
            uint32_t prs = SDHC->PRSSTAT;
            if (((prs & SDHC_PRSSTAT_CIHB_MASK) == 0u) &&
                ((prs & SDHC_PRSSTAT_CDIHB_MASK) == 0u))
            {
                break;
            }
        }

        // Try CMD0 (GO_IDLE_STATE): no response
        sdhc_error_t e = sd_send_cmd(SD_CMD_GO_IDLE_STATE, 0, SDHC_RESPONSE_TYPE_NONE, NULL);

        if (e == SDHC_ERROR_OK) {
            return SDHC_ERROR_OK;
        }

        // Retry only if it's the "startup flake" class (timeout / crc)
        if ((e & (SDHC_ERROR_CMD_TIMEOUT | SDHC_ERROR_CMD_CRC)) == 0u) {
            return e; // different error -> don't mask it
        }

        // Optional: also reset the CMD line between retries
        sdhc_reset(SDHC_RESET_CMD);
    }

    // If we get here, retries exhausted. Return last observed error class.
    return (SDHC_ERROR_CMD_TIMEOUT | SDHC_ERROR_CMD_CRC);
}



sdhc_error_t sd_card_init_full(sd_card_t *card)
{
    if (!card) return SDHC_ERROR_DATA;
    card->rca  = 0;
    card->sdhc = false;
    card->ocr  = 0;

    // 1) CMD0: reset a IDLE
//    sdhc_error_t e = sd_send_cmd(SD_CMD_GO_IDLE_STATE, 0, SDHC_RESPONSE_TYPE_NONE, NULL);
    sdhc_error_t e = sd_send_cmd0_with_retry();
    if (e != SDHC_ERROR_OK) return e;



    // 2) CMD8: interfaz/voltaje + check pattern (R7)
    // Arg típico: VHS=0x1 (2.7–3.6V) y check pattern 0xAA
    sdhc_command_t cmd8 = {0};
    e = sd_send_cmd(SD_CMD_SEND_IF_COND, 0x000001AAu, SDHC_RESPONSE_TYPE_R7, &cmd8);
    if (e != SDHC_ERROR_OK) return e;

//    if ( (cmd8.response[0] & 0xFFu) != 0xAAu ) {
        // Si esto falla, puede ser tarjeta vieja (no SD v2.0).
//        return SDHC_ERROR_UNSUPPORTED;
//    }

    // 3) ACMD41 (con CMD55 antes): negociación de OCR y “ready”
    // HCS (bit 30) = 1 para pedir High Capacity si la tarjeta soporta (SD v2)
    // Voltage window: típicamente 2.7–3.6V (bits 20..15), acá lo pedimos en conjunto usual: 0x00FF8000
    const uint32_t ACMD41_ARG = 0x40000000u | 0x00FF8000u; // HCS | VDD window
    sdhc_command_t acmd41 = {0};

    // Loop con timeout por cantidad de intentos (no te cuelgues)
    for (uint32_t i = 0; i < 100000u; i++)
    {
        e = sd_send_acmd(0, SD_ACMD_SD_SEND_OP_COND, ACMD41_ARG, SDHC_RESPONSE_TYPE_R3, &acmd41);
        if (e != SDHC_ERROR_OK) return e;

        // R3: OCR en response[0]. El bit 31 “power up status” indica listo.
        uint32_t ocr = acmd41.response[0];
        card->ocr = ocr;

        if (ocr & 0x80000000u) {
            // Listo. CCS (bit 30) indica SDHC/SDXC si es 1.
            card->sdhc = (ocr & 0x40000000u) ? true : false;
            break;
        }

        if (i == 99999u) {
            return SDHC_ERROR_CMD_TIMEOUT;
        }
    }

    // 4) CMD2: ALL_SEND_CID (R2). No es estrictamente necesario para leer, pero es parte del flow.
    e = sd_send_cmd(SD_CMD_ALL_SEND_CID, 0, SDHC_RESPONSE_TYPE_R2, NULL);
    if (e != SDHC_ERROR_OK) return e;

    // 5) CMD3: SEND_REL_ADDR (R6). Devuelve RCA en [31:16] de la respuesta.
    sdhc_command_t cmd3 = {0};
    e = sd_send_cmd(SD_CMD_SEND_REL_ADDR, 0, SDHC_RESPONSE_TYPE_R6, &cmd3);
    if (e != SDHC_ERROR_OK) return e;

    card->rca = (uint16_t)((cmd3.response[0] >> 16) & 0xFFFFu);
    if (card->rca == 0) {
    	return SDHC_ERROR_DATA;		// BAD RCA
    }

    // 6) CMD7: SELECT_CARD (R1b idealmente, pero muchos drivers usan R1 y luego esperan busy)
    // Arg = RCA<<16
    e = sd_send_cmd(SD_CMD_SELECT_CARD, (uint32_t)card->rca << 16, SDHC_RESPONSE_TYPE_R1, NULL);
    if (e != SDHC_ERROR_OK) return e;

    // 7) CMD16 (SET_BLOCKLEN) solo si NO es SDHC. Para SDHC el block length es fijo 512.
    if (!card->sdhc) {
        e = sd_send_cmd(SD_CMD_SET_BLOCKLEN, 512u, SDHC_RESPONSE_TYPE_R1, NULL);
        if (e != SDHC_ERROR_OK) return e;
    }

    return SDHC_ERROR_OK;
}


sdhc_error_t sd_read_single_block_adma2(uint32_t block, uint8_t *buf512)
{
    if (!buf512) return SDHC_ERROR_DATA;

    sdhc_command_t cmd = {0};
    sdhc_data_t data = {0};

    data.blockCount   = 1;
    data.blockSize    = 512;
    data.readBuffer   = (uint32_t*)buf512;
    data.writeBuffer  = NULL;
    data.transferMode = SDHC_TRANSFER_MODE_ADMA2;

    cmd.index = 17;                         // CMD17
    cmd.argument     = block;                      // SDHC: block addressing
    cmd.commandType  = SDHC_COMMAND_TYPE_NORMAL;
    cmd.responseType = SDHC_RESPONSE_TYPE_R1;

    return sdhc_transfer(&cmd, &data);
}

