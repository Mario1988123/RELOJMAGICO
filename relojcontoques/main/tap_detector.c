#include "tap_detector.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "TAP_DETECTOR";

// I2C Configuration for QMI8658
#define I2C_MASTER_SCL_IO           8
#define I2C_MASTER_SDA_IO           18
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          400000
#define I2C_MASTER_TIMEOUT_MS       1000

// QMI8658 Registers
#define QMI8658_ADDR                0x6B
#define QMI8658_WHO_AM_I            0x00
#define QMI8658_CTRL1               0x02
#define QMI8658_CTRL2               0x03
#define QMI8658_CTRL7               0x08
#define QMI8658_ACCEL_XOUT_L        0x35

// Tap detection thresholds
#define TAP_THRESHOLD_MG            1500.0f   // Minimum acceleration for tap (mg)
#define TAP_COOLDOWN_MS             200       // Minimum time between taps
#define LONG_TAP_DURATION_MS        2000      // Duration for long tap

static tap_callback_t s_tap_callback = NULL;
static TaskHandle_t s_task_handle = NULL;
static bool s_running = false;

/**
 * @brief Write to QMI8658 register
 */
static esp_err_t qmi8658_write_reg(uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, QMI8658_ADDR,
                                      write_buf, sizeof(write_buf),
                                      pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

/**
 * @brief Read from QMI8658 register
 */
static esp_err_t qmi8658_read_reg(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(I2C_MASTER_NUM, QMI8658_ADDR,
                                        &reg, 1, data, len,
                                        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

/**
 * @brief Read accelerometer data from QMI8658
 */
static esp_err_t qmi8658_read_accel(float *ax, float *ay, float *az) {
    uint8_t data[6];
    esp_err_t ret = qmi8658_read_reg(QMI8658_ACCEL_XOUT_L, data, 6);
    if (ret != ESP_OK) {
        return ret;
    }

    // Convert to mg (assuming 4g range, 16-bit resolution)
    int16_t raw_x = (int16_t)((data[1] << 8) | data[0]);
    int16_t raw_y = (int16_t)((data[3] << 8) | data[2]);
    int16_t raw_z = (int16_t)((data[5] << 8) | data[4]);

    // 4g range: 32768 LSB = 4000 mg
    *ax = (float)raw_x * 4000.0f / 32768.0f;
    *ay = (float)raw_y * 4000.0f / 32768.0f;
    *az = (float)raw_z * 4000.0f / 32768.0f;

    return ESP_OK;
}

/**
 * @brief Tap detection task
 */
static void tap_detector_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tap detector task started");

    uint32_t last_tap_ms = 0;
    bool tap_in_progress = false;
    uint32_t tap_start_ms = 0;

    while (s_running) {
        float ax, ay, az;

        if (qmi8658_read_accel(&ax, &ay, &az) == ESP_OK) {
            // Calculate magnitude (remove gravity ~1000mg)
            float mag = sqrtf(ax * ax + ay * ay + az * az);
            float mag_no_gravity = fabsf(mag - 1000.0f);

            uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);

            // Detect tap start
            if (mag_no_gravity > TAP_THRESHOLD_MG && !tap_in_progress) {
                uint32_t dt = now_ms - last_tap_ms;
                if (dt > TAP_COOLDOWN_MS) {
                    tap_in_progress = true;
                    tap_start_ms = now_ms;
                    ESP_LOGI(TAG, "Tap detected: mag=%.0f mg", mag_no_gravity);
                }
            }

            // Detect tap end and classify (short vs long)
            if (tap_in_progress && mag_no_gravity < (TAP_THRESHOLD_MG * 0.5f)) {
                uint32_t tap_duration = now_ms - tap_start_ms;
                bool is_short = tap_duration < LONG_TAP_DURATION_MS;

                ESP_LOGI(TAG, "Tap completed: %s (duration=%lu ms)",
                         is_short ? "SHORT" : "LONG", tap_duration);

                if (s_tap_callback) {
                    s_tap_callback(is_short);
                }

                tap_in_progress = false;
                last_tap_ms = now_ms;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // 50 Hz sampling
    }

    vTaskDelete(NULL);
}

esp_err_t tap_detector_init(void) {
    ESP_LOGI(TAG, "Initializing tap detector");

    // Initialize I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Check WHO_AM_I
    uint8_t who_am_i;
    ret = qmi8658_read_reg(QMI8658_WHO_AM_I, &who_am_i, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WHO_AM_I: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "QMI8658 WHO_AM_I: 0x%02X", who_am_i);

    // Configure accelerometer: 4g range, 125Hz ODR
    qmi8658_write_reg(QMI8658_CTRL1, 0x00); // Disable all first
    vTaskDelay(pdMS_TO_TICKS(10));
    qmi8658_write_reg(QMI8658_CTRL2, 0x95); // Accel: 4g range, 125Hz
    qmi8658_write_reg(QMI8658_CTRL7, 0x03); // Enable accel

    ESP_LOGI(TAG, "QMI8658 configured successfully");
    return ESP_OK;
}

void tap_detector_register_callback(tap_callback_t callback) {
    s_tap_callback = callback;
}

void tap_detector_start(void) {
    if (!s_running) {
        s_running = true;
        xTaskCreate(tap_detector_task, "tap_detector", 4096, NULL, 5, &s_task_handle);
    }
}

void tap_detector_stop(void) {
    s_running = false;
}
