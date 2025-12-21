#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "tap_detector.h"
#include "card_decoder.h"
#include "wifi_beacon.h"

static const char *TAG = "MAIN";

/**
 * @brief Callback when a tap is detected
 */
static void on_tap_detected(bool short_tap) {
    ESP_LOGI(TAG, "→ TAP: %s", short_tap ? "SHORT" : "LONG");
    card_decoder_feed_tap(short_tap);
}

/**
 * @brief Callback when a card is fully decoded
 */
static void on_card_decoded(card_t card) {
    char card_str[64];
    card_to_string(card, card_str, sizeof(card_str));

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  CARTA DETECTADA: %-20s ║", card_str);
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // Enviar carta por WiFi beacon
    ESP_LOGI(TAG, "Enviando carta por WiFi beacon...");
    wifi_beacon_send_card(card, 50);

    // Opcional: hacer que el reloj vibre o emita un sonido
    // TODO: agregar feedback táctil/sonoro cuando se complete la carta
}

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║     RELOJ CON TOQUES - WiFi Beacons    ║");
    ESP_LOGI(TAG, "║                                        ║");
    ESP_LOGI(TAG, "║  Instrucciones:                        ║");
    ESP_LOGI(TAG, "║  1. Toca para seleccionar PALO:        ║");
    ESP_LOGI(TAG, "║     • 1 toque = Corazones ♥            ║");
    ESP_LOGI(TAG, "║     • 2 toques = Picas ♠               ║");
    ESP_LOGI(TAG, "║     • 3 toques = Tréboles ♣            ║");
    ESP_LOGI(TAG, "║     • 4 toques = Diamantes ♦           ║");
    ESP_LOGI(TAG, "║                                        ║");
    ESP_LOGI(TAG, "║  2. Espera 2-3 segundos                ║");
    ESP_LOGI(TAG, "║                                        ║");
    ESP_LOGI(TAG, "║  3. Toca para seleccionar NÚMERO:      ║");
    ESP_LOGI(TAG, "║     • 1 toque = As                     ║");
    ESP_LOGI(TAG, "║     • 2-10 toques = 2-10               ║");
    ESP_LOGI(TAG, "║     • 11 toques = J                    ║");
    ESP_LOGI(TAG, "║     • 12 toques = Q                    ║");
    ESP_LOGI(TAG, "║     • 13 toques = K                    ║");
    ESP_LOGI(TAG, "║                                        ║");
    ESP_LOGI(TAG, "║  Ejemplo: 3 de Corazones               ║");
    ESP_LOGI(TAG, "║  → 1 toque (♥)                         ║");
    ESP_LOGI(TAG, "║  → Espera 2-3s                         ║");
    ESP_LOGI(TAG, "║  → 3 toques (3)                        ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize modules
    ESP_LOGI(TAG, "Inicializando módulos...");

    // Initialize WiFi beacon
    if (wifi_beacon_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi beacon");
        return;
    }

    // Initialize card decoder
    card_decoder_init();
    card_decoder_register_callback(on_card_decoded);

    // Initialize tap detector
    if (tap_detector_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize tap detector");
        return;
    }
    tap_detector_register_callback(on_tap_detected);

    // Start tap detection
    tap_detector_start();

    ESP_LOGI(TAG, "Sistema listo. Esperando toques...");
    ESP_LOGI(TAG, "");

    // Main loop - just keep alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
