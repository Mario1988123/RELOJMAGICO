#pragma once

#include "card_decoder.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi beacon module
 *
 * @return ESP_OK on success
 */
esp_err_t wifi_beacon_init(void);

/**
 * @brief Send card as WiFi beacon with invisible characters
 *
 * @param card Card to send
 * @param repeat_count Number of times to send the beacon
 */
void wifi_beacon_send_card(card_t card, uint32_t repeat_count);

/**
 * @brief Stop sending beacons
 */
void wifi_beacon_stop(void);

#ifdef __cplusplus
}
#endif
