#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp/display.h"
#include "bsp/esp-bsp.h"
#include "bsp_board_extra.h"
#include "display_manager.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lvgl.h"
#include "sensors.h"
#include "settings.h"
#include "ui.h"

// HID MOUSE y KEYBOARD
#include "ble_mouse_hid.h"
#include "ble_hid_keyboard.h"

// Power management
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_sleep.h"
#include "esp_pm.h"

#include "audio_alert.h"

// ⚠️ NVS para BLE
#include "nvs_flash.h"

static const char *TAG = "MAIN";

/* --------------------------------------------------------
   NVS INIT – necesario antes de usar Bluetooth
   -------------------------------------------------------- */
static void nvs_init_safe(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }
}

/* --------------------------------------------------------
   POWER INIT – BLE only + sin WiFi + sin BT clásico
   -------------------------------------------------------- */
static void power_init(void) {
    // WiFi completamente apagado
    esp_wifi_stop();
    esp_wifi_deinit();

    // Liberar memoria del Bluetooth clásico
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
}

/* ========================================================
   APP_MAIN
   ======================================================== */
extern "C" void app_main(void) {

    // 1) NVS PRIMERO (BLE lo necesita)
    nvs_init_safe();

    // 2) Config de energía
    power_init();

    // 3) Event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 4) Activar pantalla pronto
    display_manager_pm_early_init();

    // 5) Iniciar pantalla / BSP
    bsp_display_start();
    bsp_extra_init();

    // 6) Cargar ajustes del usuario
    settings_init();

    /* --------------------------------------------------------
       HID MOUSE BLE — Modo Just Works (sin PIN)
       -------------------------------------------------------- */

    ESP_LOGI(TAG, "Starting BLE HID Mouse (Just Works, sin PIN)...");

    esp_err_t hid_mouse_err = ble_hid_mouse_init("S3Watch Mouse");

    if (hid_mouse_err != ESP_OK) {
        ESP_LOGE(TAG, "BLE HID Mouse init FAILED: %s",
                 esp_err_to_name(hid_mouse_err));
    } else {
        ESP_LOGI(TAG, "BLE HID Mouse READY (Just Works, sin PIN)");
    }

    /* --------------------------------------------------------
       HID KEYBOARD BLE — Para el truco de magia
       -------------------------------------------------------- */

    ESP_LOGI(TAG, "Starting BLE HID Keyboard (para Magic Trick)...");

    esp_err_t hid_kb_err = ble_hid_keyboard_init("S3Watch Keyboard");

    if (hid_kb_err != ESP_OK) {
        ESP_LOGE(TAG, "BLE HID Keyboard init FAILED: %s",
                 esp_err_to_name(hid_kb_err));
    } else {
        ESP_LOGI(TAG, "BLE HID Keyboard READY (para enviar cartas)");
    }

    // NOTA: Ambos HID están inicializados. El móvil verá dos dispositivos BLE.
    // Empareja ambos para usar ambas funcionalidades.

    /* --------------------------------------------------------
       UI
       -------------------------------------------------------- */

    // Tarea principal de LVGL
    xTaskCreate(ui_task, "ui", 8000, NULL, 4, NULL);

    // Sonido de inicio
    audio_alert_play_startup();

    /* --------------------------------------------------------
       POWER MANAGEMENT — NO APAGAR NUNCA
       -------------------------------------------------------- */

    esp_pm_config_t pm_cfg = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 240,
        .light_sleep_enable = false
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_cfg));

    // Sin fuentes de wakeup (por seguridad)
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    ESP_LOGI(TAG, "Screen will NEVER turn off. PM disabled.");
}
