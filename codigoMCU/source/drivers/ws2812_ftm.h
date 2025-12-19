/**
 * @file     ws2812_ftm.h
 * @brief    WS2812 transport interface using FTM and DMA.
 *
 * This file declares the public API for the WS2812 transport layer:
 * - Initialization and deinitialization of the transport
 * - Non-blocking frame transmission
 * - Transfer status querying
 *
 * @author   Grupo 3
 *           - Ezequiel Díaz Guzmán
 *           - José Iván Hertter
 *           - Cristian Damián Meichtry
 *           - Lucía Inés Ruiz
 */

#ifndef WS2812_TRANSPORT_H_
#define WS2812_TRANSPORT_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initializes the WS2812 transport layer.
 *
 * Sets up the required peripherals (FTM, DMA, DMAMUX) so that WS2812
 * timing data can be streamed automatically via DMA on each FTM overflow.
 *
 * @return true if initialization succeeds.
 */
bool WS2_TransportInit(void);

/**
 * @brief Deinitializes the WS2812 transport layer.
 *
 * Stops the FTM clock and disables further WS2812 data transmission.
 */
void WS2_TransportDeinit(void);

/**
 * @brief Sends a WS2812 frame using DMA-driven FTM timing.
 *
 * Converts the provided pixel buffer into a timing buffer and starts
 * a DMA transfer that updates the FTM compare register on each overflow.
 * The function is non-blocking; transfer completion can be queried with
 * WS2_TransferInProgress().
 *
 * @param buf Pointer to the pixel data buffer.
 * @param len Length of the buffer in bytes.
 * @return true if the transfer was successfully started, false otherwise.
 */
bool WS2_TransportSend(uint8_t *buf, uint32_t len);

/**
 * @brief Checks whether a WS2812 transfer is currently in progress.
 *
 * @return true if a transfer is ongoing, false if the last transfer has finished.
 */
bool WS2_TransferInProgress(void);

#endif /* WS2812_TRANSPORT_H_ */
