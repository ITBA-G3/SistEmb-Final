#ifndef WS2812_TRANSPORT_H_
#define WS2812_TRANSPORT_H_

#include <stdint.h>
#include <stdbool.h>

bool WS2_TransportInit(void);
void WS2_TransportDeinit(void);

// Start send of raw GRB bytes buffer (length = num_pixels * 3).
bool WS2_TransportSend(uint8_t *buf, uint32_t len);

#endif /* WS2812_TRANSPORT_H_ */
