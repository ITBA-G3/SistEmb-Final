#include "sd.h"
#include <string.h>
#include "source/drivers/SDHC/sdhc.h"

#include "hardware.h"
#include "MK64F12.h"
#include "board.h"

static sdhc_error_t sd_send_cmd0_with_retry(void);
static inline sdhc_error_t sd_send_cmd(uint8_t idx, uint32_t arg,
                                      sdhc_response_type_t respType,
                                      sdhc_command_t *out_cmd);
static inline sdhc_error_t sd_send_acmd(uint32_t rca16, uint8_t acmd_idx, uint32_t acmd_arg,
                                       sdhc_response_type_t acmd_resp,
                                       sdhc_command_t *out_acmd);

static sd_error_t sd_map_sdhc_err(sdhc_error_t e)
{
    if (e == SDHC_ERROR_OK)
    	return SD_OK;
    if (e & SDHC_ERROR_CMD_TIMEOUT)
		return SD_ERR_TIMEOUT;
    if (e & SDHC_ERROR_DATA_TIMEOUT)
		return SD_ERR_TIMEOUT;
    if (e & SDHC_ERROR_CMD_CRC)
		return SD_ERR_CRC;
    if (e & SDHC_ERROR_DATA_CRC)
		return SD_ERR_CRC;
    return SD_ERR_HOST;
}

static inline uint32_t sd_addr_arg(const sd_card_t *card, uint32_t lba)
{
    return card->is_sdhc ? lba : (lba * SD_BLOCK_SIZE);
}

static sd_error_t sd_cmd(uint8_t idx, uint32_t arg, sdhc_response_type_t resp, uint32_t r[4])
{
    sdhc_command_t cmd = {0};
    cmd.index = idx;
    cmd.argument = arg;
    cmd.commandType = 0;
    cmd.responseType = resp;

    sdhc_error_t e = sdhc_transfer(&cmd, NULL);
    if (e != SDHC_ERROR_OK) return sd_map_sdhc_err(e);

    if (r) {
        r[0]=cmd.response[0]; r[1]=cmd.response[1]; r[2]=cmd.response[2]; r[3]=cmd.response[3];
    }
    return SD_OK;
}

static sd_error_t sd_cmd55(const sd_card_t *card, uint32_t r[4])
{
    return sd_cmd(55, ((uint32_t)card->rca) << 16, SDHC_RESPONSE_TYPE_R1, r);
}

static sd_error_t sd_acmd41(sd_card_t *card, uint32_t *ocr_out)
{
    uint32_t r[4];

    for (uint32_t i = 0; i < 100000; i++)
    {
        sd_error_t e = sd_cmd55(card, r);
        if (e != SD_OK) return e;

        e = sd_cmd(41, 0x40000000u, SDHC_RESPONSE_TYPE_R3, r);
        if (e != SD_OK) return e;

        uint32_t ocr = r[0];                
        if (ocr_out) *ocr_out = ocr;

        if (ocr & 0x80000000u) {            // busy cleared (ready)
            card->is_sdhc = (ocr & 0x40000000u) ? true : false; // CCS
            return SD_OK;
        }
    }
    return SD_ERR_TIMEOUT;
}

sd_error_t sd_init(sd_card_t *card)
{
    if (!card) return SDHC_ERROR_DATA;
    card->rca  = 0;
    card->is_sdhc = false;
    card->ocr  = 0;

    //CMD0: reset a IDLE
    sdhc_error_t e = sd_send_cmd0_with_retry();
    if (e != SDHC_ERROR_OK) return sd_map_sdhc_err(e);

    //CMD8: interfaz/voltaje + check pattern (R7)
    sdhc_command_t cmd8 = {0};
    e = sd_send_cmd(SD_CMD_SEND_IF_COND, 0x000001AAu, SDHC_RESPONSE_TYPE_R7, &cmd8);
    if (e != SDHC_ERROR_OK) return sd_map_sdhc_err(e);

    //ACMD41: negociación de OCR
    //HCS= 1 para pedir High Capacity si la tarjeta soporta (SD v2)
    const uint32_t ACMD41_ARG = 0x40000000u | 0x00FF8000u; // HCS | VDD window
    sdhc_command_t acmd41 = {0};

    // Loop con timeout por cantidad de intentos
    for (uint32_t i = 0; i < 100000u; i++)
    {
        e = sd_send_acmd(0, SD_ACMD_SD_SEND_OP_COND, ACMD41_ARG, SDHC_RESPONSE_TYPE_R3, &acmd41);
        if (e != SDHC_ERROR_OK) return sd_map_sdhc_err(e);

        // R3: OCR en response[0]
        uint32_t ocr = acmd41.response[0];
        card->ocr = ocr;

        if (ocr & 0x80000000u) {
            //CCS (bit 30) indica SDHC/SDXC si es 1.
            card->is_sdhc = (ocr & 0x40000000u) ? true : false;
            break;
        }

        if (i == 99999u) {
            return SDHC_ERROR_CMD_TIMEOUT;
        }
    }

    //CMD2: ALL_SEND_CID (R2)
    e = sd_send_cmd(SD_CMD_ALL_SEND_CID, 0, SDHC_RESPONSE_TYPE_R2, NULL);
    if (e != SDHC_ERROR_OK) return sd_map_sdhc_err(e);

    //CMD3: SEND_REL_ADDR (R6)
    sdhc_command_t cmd3 = {0};
    e = sd_send_cmd(SD_CMD_SEND_REL_ADDR, 0, SDHC_RESPONSE_TYPE_R6, &cmd3);
    if (e != SDHC_ERROR_OK) return sd_map_sdhc_err(e);


    card->rca = (uint16_t)((cmd3.response[0] >> 16) & 0xFFFFu);
    if (card->rca == 0) {
    	return SDHC_ERROR_DATA;		// BAD RCA
    }

    //CMD7: SELECT_CARD 
    e = sd_send_cmd(SD_CMD_SELECT_CARD, (uint32_t)card->rca << 16, SDHC_RESPONSE_TYPE_R1, NULL);
    if (e != SDHC_ERROR_OK) return sd_map_sdhc_err(e);

    //CMD16 (SET_BLOCKLEN) solo si NO es SDHC
    if (!card->is_sdhc) {
        e = sd_send_cmd(SD_CMD_SET_BLOCKLEN, 512u, SDHC_RESPONSE_TYPE_R1, NULL);
        if (e != SDHC_ERROR_OK) return sd_map_sdhc_err(e);
    }

    sdhc_set_clock(25000000u);

    card->initialized = true;

    return SDHC_ERROR_OK;
}

sd_error_t sd_read_blocks(sd_card_t *card, uint32_t lba, uint32_t *buf_w, uint32_t block_count)
{
    if (!card || !card->initialized) return SD_ERR_NOT_READY;
    if (!buf_w || block_count == 0u) return SD_ERR_PARAM;
    if (((uintptr_t)buf_w & 0x3u) != 0u) return SD_ERR_PARAM;

    sdhc_command_t cmd = {0};
    sdhc_data_t data = {0};

    cmd.commandType = 0;

    if (block_count == 1u) {
        cmd.index = 17; // CMD17
        cmd.responseType = SDHC_RESPONSE_TYPE_R1;
    } else {
        cmd.index = 18; // CMD18
        cmd.responseType = SDHC_RESPONSE_TYPE_R1;
    }
    cmd.argument = sd_addr_arg(card, lba);

    data.blockSize = SD_BLOCK_SIZE;
    data.blockCount = block_count;
    data.readBuffer = buf_w;
    data.writeBuffer = NULL;
    data.transferMode = SDHC_TRANSFER_MODE_ADMA2;

    sdhc_error_t he = sdhc_transfer(&cmd, &data);
    if (he != SDHC_ERROR_OK) return sd_map_sdhc_err(he);

    return SD_OK;
}

sd_error_t sd_write_blocks(sd_card_t *card, uint32_t lba, const uint32_t *buf_w, uint32_t block_count)
{
    if (!card || !card->initialized) return SD_ERR_NOT_READY;
    if (!buf_w || block_count == 0u) return SD_ERR_PARAM;
    if (((uintptr_t)buf_w & 0x3u) != 0u) return SD_ERR_PARAM;

    if (block_count > 1u) {
        uint32_t r[4];
        sd_error_t e = sd_cmd55(card, r);
        if (e != SD_OK) return e;

        e = sd_cmd(23, block_count, SDHC_RESPONSE_TYPE_R1, NULL); // ACMD23
    }

    sdhc_command_t cmd = {0};
    sdhc_data_t data = {0};

    cmd.commandType = 0;

    if (block_count == 1u) {
        cmd.index = 24; // CMD24
        cmd.responseType = SDHC_RESPONSE_TYPE_R1;
    } else {
        cmd.index = 25; // CMD25
        cmd.responseType = SDHC_RESPONSE_TYPE_R1;
    }
    cmd.argument = sd_addr_arg(card, lba);

    data.blockSize = SD_BLOCK_SIZE;
    data.blockCount = block_count;
    data.readBuffer = NULL;
    data.writeBuffer = (uint32_t*)(uintptr_t)buf_w;
    data.transferMode = SDHC_TRANSFER_MODE_ADMA2;

    sdhc_error_t he = sdhc_transfer(&cmd, &data);
    if (he != SDHC_ERROR_OK) return sd_map_sdhc_err(he);

    return SD_OK;
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

        // Wait host not inhibited
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
            return e; 
        }

        sdhc_reset(SDHC_RESET_CMD);
    }

    return (SDHC_ERROR_CMD_TIMEOUT | SDHC_ERROR_CMD_CRC);
}

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
    sdhc_error_t e = sd_send_cmd(SD_CMD_APP_CMD, (uint32_t)rca16 << 16, SDHC_RESPONSE_TYPE_R1, NULL);
    if (e != SDHC_ERROR_OK) return e;

    return sd_send_cmd(acmd_idx, acmd_arg, acmd_resp, out_acmd);
}



