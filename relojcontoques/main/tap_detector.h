#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback function type for tap detection
 *
 * @param short_tap true if short tap, false if long tap (>2s)
 */
typedef void (*tap_callback_t)(bool short_tap);

/**
 * @brief Initialize tap detector with QMI8658 accelerometer
 *
 * @return ESP_OK on success
 */
esp_err_t tap_detector_init(void);

/**
 * @brief Register callback for tap events
 *
 * @param callback Function to call when tap detected
 */
void tap_detector_register_callback(tap_callback_t callback);

/**
 * @brief Start tap detection task
 */
void tap_detector_start(void);

/**
 * @brief Stop tap detection
 */
void tap_detector_stop(void);

#ifdef __cplusplus
}
#endif
