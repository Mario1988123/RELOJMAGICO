#include "ble_hid_combined.h"
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#include "esp_hidd.h"
#include "esp_hid_common.h"

static const char *TAG = "BLE_HID_COMBINED";

/* Estado interno */
static bool s_ble_started = false;
static bool s_connected = false;
static bool s_use_pin = false;
static char s_device_name[32] = {0};
static esp_hidd_dev_t *s_hid_dev = NULL;

/* HID Report Descriptor COMBINADO (Mouse + Keyboard) */
static const uint8_t s_combined_report_map[] = {
    // Mouse
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x02,       // Usage (Mouse)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report ID (1) - MOUSE
    0x09, 0x01,       //   Usage (Pointer)
    0xA1, 0x00,       //   Collection (Physical)
    0x05, 0x09,       //     Usage Page (Buttons)
    0x19, 0x01,       //     Usage Minimum (1)
    0x29, 0x03,       //     Usage Maximum (3)
    0x15, 0x00,       //     Logical Minimum (0)
    0x25, 0x01,       //     Logical Maximum (1)
    0x95, 0x03,       //     Report Count (3)
    0x75, 0x01,       //     Report Size (1)
    0x81, 0x02,       //     Input (Data,Var,Abs)
    0x95, 0x01,       //     Report Count (1)
    0x75, 0x05,       //     Report Size (5)
    0x81, 0x01,       //     Input (Const)
    0x05, 0x01,       //     Usage Page (Generic Desktop)
    0x09, 0x30,       //     Usage (X)
    0x09, 0x31,       //     Usage (Y)
    0x09, 0x38,       //     Usage (Wheel)
    0x15, 0x81,       //     Logical Minimum (-127)
    0x25, 0x7F,       //     Logical Maximum (127)
    0x75, 0x08,       //     Report Size (8)
    0x95, 0x03,       //     Report Count (3)
    0x81, 0x06,       //     Input (Data,Var,Rel)
    0xC0,             //   End Collection
    0xC0,             // End Collection

    // Keyboard
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x02,       //   Report ID (2) - KEYBOARD
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0xE0,       //   Usage Minimum (224)
    0x29, 0xE7,       //   Usage Maximum (231)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data,Var,Abs) - Modifier keys
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x08,       //   Report Size (8)
    0x81, 0x01,       //   Input (Const) - Reserved byte
    0x95, 0x06,       //   Report Count (6)
    0x75, 0x08,       //   Report Size (8)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x65,       //   Logical Maximum (101)
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0x00,       //   Usage Minimum (0)
    0x29, 0x65,       //   Usage Maximum (101)
    0x81, 0x00,       //   Input (Data,Array)
    0xC0              // End Collection
};

static esp_hid_raw_report_map_t s_report_maps[] = {
    {
        .data = s_combined_report_map,
        .len = sizeof(s_combined_report_map),
    }
};

static const esp_hid_device_config_t s_hid_config = {
    .vendor_id = 0x16C0,
    .product_id = 0x05DF,
    .version = 0x0100,
    .device_name = "S3Watch HID",
    .manufacturer_name = "Pablo",
    .serial_number = "0001",
    .report_maps = s_report_maps,
    .report_maps_len = 1,
};

static esp_ble_adv_data_t s_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x03C2,  // Mouse appearance
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t s_adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/* Callbacks HID */
static void hid_event_handler(void *handler_arg, esp_event_base_t base,
                              int32_t event_id, void *event_data)
{
    esp_hidd_event_t ev = (esp_hidd_event_t)event_id;
    esp_hidd_event_data_t *ed = (esp_hidd_event_data_t *)event_data;

    ESP_LOGI(TAG, "!!! HID EVENT: %d !!!", (int)ev);

    switch (ev) {
    case ESP_HIDD_START_EVENT:
        ESP_LOGI(TAG, ">>> HID START - iniciando advertising");
        esp_ble_gap_config_adv_data(&s_adv_data);
        break;

    case ESP_HIDD_CONNECT_EVENT:
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, ">>> HID CONECTADO! <<<");
        ESP_LOGI(TAG, "========================================");
        s_connected = true;
        break;

    case ESP_HIDD_DISCONNECT_EVENT:
        ESP_LOGW(TAG, ">>> HID DESCONECTADO, reason=%d", ed->disconnect.reason);
        s_connected = false;
        esp_ble_gap_start_advertising(&s_adv_params);
        break;

    case ESP_HIDD_OUTPUT_EVENT:
        ESP_LOGI(TAG, ">>> HID OUTPUT EVENT");
        break;

    case ESP_HIDD_FEATURE_EVENT:
        ESP_LOGI(TAG, ">>> HID FEATURE EVENT");
        break;

    case ESP_HIDD_STOP_EVENT:
        ESP_LOGI(TAG, ">>> HID STOP EVENT");
        break;

    default:
        ESP_LOGW(TAG, ">>> HID evento desconocido: %d", (int)ev);
        break;
    }
}

/* Callback GAP - Aceptar emparejamiento automáticamente */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    ESP_LOGI(TAG, "!!! GAP EVENT: %d !!!", (int)event);

    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, ">>> ADV configurado, iniciando advertising");
        esp_ble_gap_start_advertising(&s_adv_params);
        break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, ">>> Advertising BLE OK - Dispositivo: %s", s_device_name);
            ESP_LOGI(TAG, "*** EMPAREJAMIENTO SIN PIN (Just Works) ***");
        }
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        ESP_LOGI(TAG, ">>> Advertising DETENIDO");
        break;

    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(TAG, ">>> Parámetros de conexión actualizados");
        ESP_LOGI(TAG, "    Status: %d, Min interval: %d, Max interval: %d, Latency: %d, Timeout: %d",
                 param->update_conn_params.status,
                 param->update_conn_params.min_int,
                 param->update_conn_params.max_int,
                 param->update_conn_params.latency,
                 param->update_conn_params.timeout);

        // DIAGNÓSTICO: Verificar estado de conexión HID después de actualizar parámetros
        if (s_hid_dev) {
            bool connected = esp_hidd_dev_connected(s_hid_dev);
            ESP_LOGI(TAG, "    >>> Estado HID después de actualizar parámetros: %s",
                     connected ? "CONECTADO ✓" : "NO CONECTADO ✗");
        }
        break;

    case ESP_GAP_BLE_SEC_REQ_EVT:
        ESP_LOGI(TAG, "!!! Solicitud de seguridad recibida - ACEPTANDO AUTOMATICAMENTE !!!");
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;

    case ESP_GAP_BLE_NC_REQ_EVT:
        ESP_LOGI(TAG, "!!! Confirmación numérica solicitada - ACEPTANDO AUTOMATICAMENTE !!!");
        ESP_LOGI(TAG, "    Código: %06" PRIu32, param->ble_security.key_notif.passkey);
        esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
        break;

    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
        ESP_LOGI(TAG, "Passkey notificación: %06" PRIu32, param->ble_security.key_notif.passkey);
        break;

    case ESP_GAP_BLE_KEY_EVT:
        ESP_LOGI(TAG, "Key recibida, type: %d", param->ble_security.ble_key.key_type);
        break;

    case ESP_GAP_BLE_PASSKEY_REQ_EVT:
        ESP_LOGI(TAG, "Passkey solicitado");
        break;

    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (param->ble_security.auth_cmpl.success) {
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, ">>> EMPAREJAMIENTO EXITOSO! <<<");
            ESP_LOGI(TAG, "    Dirección: %02X:%02X:%02X:%02X:%02X:%02X",
                     param->ble_security.auth_cmpl.bd_addr[0],
                     param->ble_security.auth_cmpl.bd_addr[1],
                     param->ble_security.auth_cmpl.bd_addr[2],
                     param->ble_security.auth_cmpl.bd_addr[3],
                     param->ble_security.auth_cmpl.bd_addr[4],
                     param->ble_security.auth_cmpl.bd_addr[5]);

            // DIAGNÓSTICO: Verificar inmediatamente el estado HID
            if (s_hid_dev) {
                bool hid_conn = esp_hidd_dev_connected(s_hid_dev);
                ESP_LOGI(TAG, "    Estado HID INMEDIATAMENTE después de emparejamiento: %s",
                         hid_conn ? "CONECTADO ✓" : "NO CONECTADO ✗ (esperando...)");
            }

            ESP_LOGI(TAG, "========================================");
            // NO establecer s_connected aquí - esperar ESP_HIDD_CONNECT_EVENT o que se detecte automáticamente
        } else {
            ESP_LOGE(TAG, "!!! EMPAREJAMIENTO FALLIDO !!!");
            ESP_LOGE(TAG, "    Código error: 0x%02X", param->ble_security.auth_cmpl.fail_reason);
            s_connected = false;
            esp_ble_gap_start_advertising(&s_adv_params);
        }
        break;

    case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
        ESP_LOGI(TAG, "Bond eliminado, reiniciando advertising");
        esp_ble_gap_start_advertising(&s_adv_params);
        break;

    default:
        break;
    }
}

esp_err_t ble_hid_combined_init(const char *device_name, bool use_pin)
{
    if (!device_name) return ESP_ERR_INVALID_ARG;

    strncpy(s_device_name, device_name, sizeof(s_device_name) - 1);
    s_use_pin = use_pin;

    ESP_LOGI(TAG, "Inicializando BLE HID Combinado...");

    if (!s_ble_started) {
        ESP_LOGI(TAG, "Configurando controlador BT...");

        // NO llamar a mem_release aquí, ya se hizo en main
        // esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
        ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
        ESP_ERROR_CHECK(esp_bluedroid_init());
        ESP_ERROR_CHECK(esp_bluedroid_enable());
        ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));

        s_ble_started = true;
    }

    ESP_ERROR_CHECK(esp_ble_gap_set_device_name(s_device_name));

    // Configurar seguridad - SIEMPRE SIN PIN para empezar
    ESP_LOGI(TAG, "Configurando seguridad (SIN PIN)...");

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t resp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(auth_req));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &resp_key, sizeof(resp_key));

    ESP_LOGI(TAG, "Inicializando dispositivo HID...");
    ESP_ERROR_CHECK(esp_hidd_dev_init(&s_hid_config, ESP_HID_TRANSPORT_BLE,
                                      hid_event_handler, &s_hid_dev));

    ESP_LOGI(TAG, "✓ HID Combinado inicializado: %s (SIN PIN)", s_device_name);

    // FORZAR inicio de advertising (el evento START a veces no se dispara)
    ESP_LOGI(TAG, "Forzando inicio de advertising...");
    vTaskDelay(pdMS_TO_TICKS(100));  // Pequeño delay
    esp_ble_gap_config_adv_data(&s_adv_data);

    return ESP_OK;
}

bool ble_hid_combined_is_connected(void)
{
    // FIX: Confiar SOLO en esp_hidd_dev_connected() como fuente de verdad
    // El evento ESP_HIDD_CONNECT_EVENT a veces no se dispara después del emparejamiento
    bool hid_connected = s_hid_dev ? esp_hidd_dev_connected(s_hid_dev) : false;

    // Sincronizar s_connected con el estado real del HID
    if (hid_connected && !s_connected) {
        ESP_LOGI(TAG, "!!! CONEXION HID DETECTADA (sin evento) - actualizando estado !!!");
        s_connected = true;
    } else if (!hid_connected && s_connected) {
        ESP_LOGW(TAG, "!!! DESCONEXION HID DETECTADA - actualizando estado !!!");
        s_connected = false;
    }

    // Log solo cuando cambia el estado (evitar spam)
    static bool last_state = false;
    if (hid_connected != last_state) {
        ESP_LOGI(TAG, "Estado conexión cambió: %s", hid_connected ? "CONECTADO" : "DESCONECTADO");
        last_state = hid_connected;
    }

    return hid_connected;
}

/* Mouse functions */
void ble_hid_mouse_move(int8_t dx, int8_t dy, int8_t wheel)
{
    // Verificar estado detalladamente
    static int warning_count = 0;
    static int attempt_count = 0;

    if (!s_hid_dev) {
        if (warning_count++ < 5) {
            ESP_LOGE(TAG, "❌ Mouse move RECHAZADO: s_hid_dev es NULL");
        }
        return;
    }

    bool connected = esp_hidd_dev_connected(s_hid_dev);

    // DIAGNÓSTICO: Intentar enviar de todos modos las primeras 3 veces
    if (!connected && attempt_count < 3) {
        ESP_LOGW(TAG, "⚠️  HID reporta NO conectado, pero INTENTANDO enviar de todos modos (intento %d/3)...",
                 attempt_count + 1);
    } else if (!connected) {
        if (warning_count++ < 5) {
            ESP_LOGW(TAG, "❌ Mouse move RECHAZADO: HID no conectado (s_connected=%d, esp_hidd=%d)",
                     s_connected, connected);
        }
        return;
    }

    // Report format: [Buttons, X, Y, Wheel] - SIN Report ID en el buffer
    uint8_t report[4] = {0x00, (uint8_t)dx, (uint8_t)dy, (uint8_t)wheel};

    ESP_LOGI(TAG, ">>> ENVIANDO Mouse Report: dx=%d, dy=%d, wheel=%d (connected=%d, attempt=%d)",
             dx, dy, wheel, connected, attempt_count);

    esp_err_t ret = esp_hidd_dev_input_set(s_hid_dev, 0, 0x01, report, sizeof(report));

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Error enviando mouse: %s (0x%x)", esp_err_to_name(ret), ret);
    } else {
        ESP_LOGI(TAG, "✅ Mouse report ENVIADO exitosamente!");
        warning_count = 0;  // Reset warning counter on success
    }

    attempt_count++;
}

void ble_hid_mouse_buttons(bool left, bool right, bool middle)
{
    if (!s_hid_dev || !esp_hidd_dev_connected(s_hid_dev)) return;

    uint8_t buttons = 0;
    if (left) buttons |= 0x01;
    if (right) buttons |= 0x02;
    if (middle) buttons |= 0x04;

    // Report format: [Buttons, X, Y, Wheel] - SIN Report ID en el buffer
    uint8_t report[4] = {buttons, 0x00, 0x00, 0x00};
    esp_hidd_dev_input_set(s_hid_dev, 0, 0x01, report, sizeof(report));

    ESP_LOGI(TAG, "✓ Mouse buttons: L=%d R=%d M=%d (0x%02X)", left, right, middle, buttons);
}

/* Keyboard functions */
static const uint8_t ascii_to_hid[128] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2A, 0x2B, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2C, 0x1E, 0x34, 0x20, 0x21, 0x22, 0x24, 0x34,
    0x26, 0x27, 0x25, 0x2E, 0x36, 0x2D, 0x37, 0x38,
    0x27, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
    0x25, 0x26, 0x33, 0x33, 0x36, 0x2E, 0x37, 0x38,
    0x1F, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,
    0x1B, 0x1C, 0x1D, 0x2F, 0x31, 0x30, 0x23, 0x2D,
    0x35, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,
    0x1B, 0x1C, 0x1D, 0x2F, 0x31, 0x30, 0x35, 0x00
};

static void send_keyboard_report(uint8_t modifier, uint8_t keycode)
{
    if (!s_hid_dev || !esp_hidd_dev_connected(s_hid_dev)) return;

    // Report format: [Modifier, Reserved, Key1, Key2, Key3, Key4, Key5, Key6] - SIN Report ID
    uint8_t report[8] = {modifier, 0x00, keycode, 0x00, 0x00, 0x00, 0x00, 0x00};
    esp_hidd_dev_input_set(s_hid_dev, 0, 0x02, report, sizeof(report));
    vTaskDelay(pdMS_TO_TICKS(20));

    // Release
    memset(report, 0, 8);
    esp_hidd_dev_input_set(s_hid_dev, 0, 0x02, report, sizeof(report));
    vTaskDelay(pdMS_TO_TICKS(20));
}

void ble_hid_keyboard_send_key(uint8_t keycode)
{
    send_keyboard_report(0x00, keycode);
}

void ble_hid_keyboard_send_text(const char *text)
{
    if (!text) return;

    for (size_t i = 0; text[i] != '\0'; i++) {
        uint8_t ch = (uint8_t)text[i];
        if (ch >= 128) continue;

        uint8_t modifier = 0x00;
        uint8_t keycode = ascii_to_hid[ch];

        if (ch >= 'A' && ch <= 'Z') {
            modifier = 0x02;  // Shift
        }

        if (keycode != 0x00) {
            send_keyboard_report(modifier, keycode);
        }
    }
}
