#include "wifi_beacon.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "WIFI_BEACON";

// Caracteres Unicode invisibles (Zero Width Characters)
// Usaremos diferentes caracteres para codificar la información
#define ZWSP    "\xE2\x80\x8B"  // Zero Width Space (U+200B)
#define ZWNJ    "\xE2\x80\x8C"  // Zero Width Non-Joiner (U+200C)
#define ZWJ     "\xE2\x80\x8D"  // Zero Width Joiner (U+200D)
#define ZWSP2   "\xE2\x80\x8E"  // Left-to-Right Mark (U+200E)

// Prefijo visible para identificar nuestros beacons
#define BEACON_PREFIX "CARD_"

static bool s_wifi_initialized = false;

/**
 * @brief Build SSID with invisible characters encoding the card
 *
 * El formato será: "CARD_" + caracteres_invisibles
 * Los caracteres invisibles codificarán: palo (4 bits) + número (4 bits)
 */
static void build_card_ssid(card_t card, char *ssid, size_t max_len) {
    // Iniciar con el prefijo visible
    snprintf(ssid, max_len, "%s", BEACON_PREFIX);
    size_t offset = strlen(ssid);

    // Codificar el palo (1-4) usando diferentes combinaciones de caracteres invisibles
    switch (card.suit) {
        case CARD_SUIT_HEARTS:   // ♥ = 1
            strncat(ssid, ZWSP, max_len - offset - 1);
            break;
        case CARD_SUIT_SPADES:   // ♠ = 2
            strncat(ssid, ZWNJ, max_len - offset - 1);
            break;
        case CARD_SUIT_CLUBS:    // ♣ = 3
            strncat(ssid, ZWJ, max_len - offset - 1);
            break;
        case CARD_SUIT_DIAMONDS: // ♦ = 4
            strncat(ssid, ZWSP2, max_len - offset - 1);
            break;
        default:
            break;
    }
    offset = strlen(ssid);

    // Separador (siempre el mismo)
    strncat(ssid, ZWSP, max_len - offset - 1);
    offset = strlen(ssid);

    // Codificar el número (1-13) usando combinaciones binarias
    // Usaremos 4 bits: ZWSP=0, ZWNJ=1
    for (int i = 3; i >= 0; i--) {
        if ((card.number >> i) & 1) {
            strncat(ssid, ZWNJ, max_len - offset - 1);
        } else {
            strncat(ssid, ZWSP, max_len - offset - 1);
        }
        offset = strlen(ssid);
    }

    ESP_LOGI(TAG, "SSID built: \"%s\" (length=%d bytes)", ssid, strlen(ssid));
    ESP_LOGI(TAG, "Card encoded: suit=%d, number=%d", card.suit, card.number);
}

/**
 * @brief Send beacon frame with custom SSID
 */
static void send_beacon_frame(const char *ssid) {
    // Beacon frame structure (simplified)
    uint8_t beacon_frame[128];
    memset(beacon_frame, 0, sizeof(beacon_frame));

    // Frame Control
    beacon_frame[0] = 0x80; // Type: Management, Subtype: Beacon
    beacon_frame[1] = 0x00;

    // Duration
    beacon_frame[2] = 0x00;
    beacon_frame[3] = 0x00;

    // Destination Address (broadcast)
    memset(&beacon_frame[4], 0xFF, 6);

    // Source Address (MAC address del ESP32)
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_AP, mac);
    memcpy(&beacon_frame[10], mac, 6);

    // BSSID (same as source)
    memcpy(&beacon_frame[16], mac, 6);

    // Sequence Control
    beacon_frame[22] = 0x00;
    beacon_frame[23] = 0x00;

    // Timestamp (8 bytes)
    memset(&beacon_frame[24], 0, 8);

    // Beacon Interval (100 TU = 102.4 ms)
    beacon_frame[32] = 0x64;
    beacon_frame[33] = 0x00;

    // Capability Info
    beacon_frame[34] = 0x01;
    beacon_frame[35] = 0x04;

    // SSID Parameter Set
    size_t ssid_len = strlen(ssid);
    beacon_frame[36] = 0x00;           // Element ID: SSID
    beacon_frame[37] = ssid_len;       // Length
    memcpy(&beacon_frame[38], ssid, ssid_len);

    // Calculate total frame length
    size_t frame_len = 38 + ssid_len;

    // Send raw frame
    esp_err_t ret = esp_wifi_80211_tx(WIFI_IF_AP, beacon_frame, frame_len, false);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send beacon: %s", esp_err_to_name(ret));
    }
}

/**
 * @brief Task to send beacons repeatedly
 */
static void beacon_send_task(void *pvParameters) {
    card_t card = *(card_t *)pvParameters;
    free(pvParameters);

    char ssid[64];
    build_card_ssid(card, ssid, sizeof(ssid));

    ESP_LOGI(TAG, "Starting beacon transmission");

    // Enviar 50 beacons con intervalos de 100ms (total ~5 segundos)
    for (int i = 0; i < 50; i++) {
        send_beacon_frame(ssid);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "Beacon transmission completed");
    vTaskDelete(NULL);
}

esp_err_t wifi_beacon_init(void) {
    if (s_wifi_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing WiFi for beacon transmission");

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    // Configure AP with minimal settings
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "ESP32_BEACON",
            .ssid_len = strlen("ESP32_BEACON"),
            .channel = 1,
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 0,
            .beacon_interval = 100,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_wifi_initialized = true;
    ESP_LOGI(TAG, "WiFi initialized successfully");

    return ESP_OK;
}

void wifi_beacon_send_card(card_t card, uint32_t repeat_count) {
    if (!s_wifi_initialized) {
        ESP_LOGE(TAG, "WiFi not initialized");
        return;
    }

    char card_str[64];
    card_to_string(card, card_str, sizeof(card_str));
    ESP_LOGI(TAG, "Sending card via WiFi beacon: %s", card_str);

    // Create task to send beacons
    card_t *card_copy = malloc(sizeof(card_t));
    if (card_copy) {
        memcpy(card_copy, &card, sizeof(card_t));
        xTaskCreate(beacon_send_task, "beacon_tx", 4096, card_copy, 5, NULL);
    }
}

void wifi_beacon_stop(void) {
    if (s_wifi_initialized) {
        esp_wifi_stop();
        esp_wifi_deinit();
        s_wifi_initialized = false;
    }
}
