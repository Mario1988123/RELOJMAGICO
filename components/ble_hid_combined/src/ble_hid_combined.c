#include "ble_hid_combined.h"
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

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
static bool s_sec_conn = false;  // NUEVO: Flag de conexión segura (como ejemplo oficial ESP-IDF)
static bool s_use_pin = false;
static char s_device_name[32] = {0};
static esp_hidd_dev_t *s_hid_dev = NULL;
static uint16_t s_hid_conn_id = 0;  // NUEVO: ID de conexión HID (como ejemplo oficial)
static esp_bd_addr_t s_peer_bd_addr = {0};  // Dirección del dispositivo conectado
static bool s_pairing_completed = false;    // Flag para tracking de emparejamiento
static TimerHandle_t s_connection_check_timer = NULL;  // Timer para verificar conexión periódicamente
static int s_connection_check_count = 0;  // Contador de verificaciones

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
        ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
        ESP_LOGI(TAG, "║                                            ║");
        ESP_LOGI(TAG, "║    ✅ HID CONECTADO EXITOSAMENTE! ✅       ║");
        ESP_LOGI(TAG, "║                                            ║");
        ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
        s_connected = true;
        // NUEVO: Guardar ID de conexión como en ejemplo oficial ESP-IDF
        s_hid_conn_id = ed->connect.conn_id;
        ESP_LOGI(TAG, ">>> ID de conexión HID: %d", s_hid_conn_id);
        ESP_LOGI(TAG, ">>> El dispositivo ahora puede enviar eventos de mouse/keyboard");
        break;

    case ESP_HIDD_DISCONNECT_EVENT:
        ESP_LOGW(TAG, "╔════════════════════════════════════════════╗");
        ESP_LOGW(TAG, "║    ⚠️  HID DESCONECTADO                    ║");
        ESP_LOGW(TAG, "╚════════════════════════════════════════════╝");
        ESP_LOGW(TAG, ">>> Razón: %d", ed->disconnect.reason);
        s_connected = false;
        s_sec_conn = false;  // NUEVO: Limpiar flag de conexión segura
        s_hid_conn_id = 0;   // NUEVO: Limpiar ID de conexión
        s_pairing_completed = false;
        memset(s_peer_bd_addr, 0, sizeof(esp_bd_addr_t));

        // Detener timer de verificación si está activo
        if (s_connection_check_timer) {
            xTimerStop(s_connection_check_timer, 0);
            ESP_LOGI(TAG, ">>> Timer de verificación detenido");
        }

        ESP_LOGI(TAG, ">>> Reiniciando advertising...");
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

/* Timer callback para verificar conexión HID periódicamente */
static void connection_check_timer_callback(TimerHandle_t xTimer)
{
    s_connection_check_count++;

    if (!s_pairing_completed || !s_hid_dev) {
        // Si no hay emparejamiento o HID device, detener el timer
        if (s_connection_check_timer) {
            xTimerStop(s_connection_check_timer, 0);
        }
        return;
    }

    bool hid_connected = esp_hidd_dev_connected(s_hid_dev);

    ESP_LOGI(TAG, "🔍 VERIFICACIÓN PERIÓDICA #%d: esp_hidd_dev_connected()=%s, s_connected=%s, s_pairing_completed=%s",
             s_connection_check_count,
             hid_connected ? "TRUE ✓" : "FALSE ✗",
             s_connected ? "TRUE" : "FALSE",
             s_pairing_completed ? "TRUE" : "FALSE");

    // SOLUCIÓN: Si el emparejamiento está completo, ASUMIR que estamos conectados
    // El ESP32 BLE stack a veces no reporta correctamente esp_hidd_dev_connected()
    if (s_pairing_completed && !s_connected) {
        ESP_LOGW(TAG, "╔═══════════════════════════════════════════════════╗");
        ESP_LOGW(TAG, "║ FORZANDO CONEXIÓN HID                             ║");
        ESP_LOGW(TAG, "║ Emparejamiento completado pero conexión no        ║");
        ESP_LOGW(TAG, "║ detectada. Asumiendo conexión válida.             ║");
        ESP_LOGW(TAG, "╚═══════════════════════════════════════════════════╝");
        s_connected = true;
    }

    // Actualizar el estado si hay cambios
    if (hid_connected && !s_connected) {
        ESP_LOGI(TAG, "✅ CONEXIÓN HID DETECTADA - Actualizando s_connected=true");
        s_connected = true;
    }

    // Detener el timer después de 20 verificaciones o si ya está conectado
    if (s_connection_check_count >= 20 || s_connected) {
        ESP_LOGI(TAG, "⏹️  Deteniendo timer de verificación (count=%d, connected=%s)",
                 s_connection_check_count, s_connected ? "TRUE" : "FALSE");
        if (s_connection_check_timer) {
            xTimerStop(s_connection_check_timer, 0);
        }
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

        // DIAGNÓSTICO MEJORADO: Verificar múltiples veces el estado HID
        if (s_hid_dev) {
            ESP_LOGI(TAG, "    >>> Verificando estado HID (inmediato)...");
            bool connected_immediate = esp_hidd_dev_connected(s_hid_dev);
            ESP_LOGI(TAG, "        Inmediato: %s", connected_immediate ? "CONECTADO ✓" : "NO CONECTADO ✗");

            // Dar tiempo al stack BLE para actualizar el estado
            vTaskDelay(pdMS_TO_TICKS(100));

            ESP_LOGI(TAG, "    >>> Verificando estado HID (después de 100ms)...");
            bool connected_delayed = esp_hidd_dev_connected(s_hid_dev);
            ESP_LOGI(TAG, "        Después de 100ms: %s", connected_delayed ? "CONECTADO ✓" : "NO CONECTADO ✗");

            // Si se detecta conexión, actualizar el flag
            if (connected_delayed && !s_connected) {
                ESP_LOGI(TAG, "    ╔══════════════════════════════════════╗");
                ESP_LOGI(TAG, "    ║ CONEXIÓN HID CONFIRMADA!             ║");
                ESP_LOGI(TAG, "    ║ Actualizando flag s_connected        ║");
                ESP_LOGI(TAG, "    ╚══════════════════════════════════════╝");
                s_connected = true;
            }
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

            // Guardar dirección del peer
            memcpy(s_peer_bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            s_pairing_completed = true;

            // ✅ SOLUCIÓN CORRECTA (como ejemplo oficial ESP-IDF):
            // Setear flag de conexión segura cuando autenticación es exitosa
            s_sec_conn = true;
            ESP_LOGI(TAG, "    ✅ CONEXIÓN SEGURA ESTABLECIDA (s_sec_conn=true)");
            ESP_LOGI(TAG, "    >>> El mouse/keyboard ahora puede enviar reportes HID");

            // FIX: Forzar actualización de parámetros de conexión para activar HID
            // Basado en https://github.com/asterics/esp32_mouse_keyboard
            ESP_LOGI(TAG, "    >>> FORZANDO actualización de parámetros de conexión...");
            esp_ble_conn_update_params_t conn_params;
            memcpy(conn_params.bda, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            conn_params.min_int = 6;   // 6 * 1.25ms = 7.5ms
            conn_params.max_int = 6;   // 6 * 1.25ms = 7.5ms
            conn_params.latency = 0;   // Sin latencia de esclavo
            conn_params.timeout = 500; // 500 * 10ms = 5000ms = 5s

            esp_err_t ret = esp_ble_gap_update_conn_params(&conn_params);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "    ❌ Error actualizando parámetros: %s", esp_err_to_name(ret));
            } else {
                ESP_LOGI(TAG, "    ✓ Solicitud de actualización enviada");
            }

            // NUEVO: Iniciar timer de verificación periódica de conexión HID
            if (!s_connection_check_timer) {
                s_connection_check_timer = xTimerCreate(
                    "hid_conn_check",           // Nombre del timer
                    pdMS_TO_TICKS(500),         // Período: 500ms
                    pdTRUE,                     // Auto-reload: sí
                    NULL,                       // Timer ID (no usado)
                    connection_check_timer_callback  // Callback
                );
            }

            if (s_connection_check_timer) {
                s_connection_check_count = 0;  // Reset contador
                ESP_LOGI(TAG, "    🔄 Iniciando verificación periódica de conexión HID...");
                xTimerStart(s_connection_check_timer, 0);
            } else {
                ESP_LOGE(TAG, "    ❌ Error creando timer de verificación");
            }

            ESP_LOGI(TAG, "========================================");
        } else {
            ESP_LOGE(TAG, "!!! EMPAREJAMIENTO FALLIDO !!!");
            ESP_LOGE(TAG, "    Código error: 0x%02X", param->ble_security.auth_cmpl.fail_reason);
            s_connected = false;
            s_sec_conn = false;  // NUEVO: Limpiar flag de conexión segura
            s_hid_conn_id = 0;   // NUEVO: Limpiar ID de conexión
            s_pairing_completed = false;
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
    // ✅ SOLUCIÓN CORRECTA (basada en ejemplo oficial ESP-IDF):
    // NO usar esp_hidd_dev_connected() - no es confiable en ESP32
    // Usar flag s_sec_conn que se setea en ESP_GAP_BLE_AUTH_CMPL_EVT
    //
    // Referencia: https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/bluedroid/ble/ble_hid_device_demo/main/ble_hidd_demo_main.c
    //
    // El ejemplo oficial usa:
    // - hid_conn_id: ID de conexión (seteado en ESP_HIDD_CONNECT_EVENT)
    // - sec_conn: Flag de conexión segura (seteado en ESP_GAP_BLE_AUTH_CMPL_EVT)
    // - Verifica sec_conn antes de enviar reportes HID

    static int check_count = 0;
    static bool last_state = false;

    // Log solo en cambios de estado o primeras 10 verificaciones
    if (check_count < 10 || s_sec_conn != last_state) {
        ESP_LOGI(TAG, "[CHECK #%d] s_sec_conn=%s, s_connected=%s, s_hid_conn_id=%d",
                 check_count,
                 s_sec_conn ? "TRUE ✓" : "FALSE ✗",
                 s_connected ? "TRUE" : "FALSE",
                 s_hid_conn_id);
        check_count++;
    }

    last_state = s_sec_conn;
    return s_sec_conn;
}

/* Mouse functions */
void ble_hid_mouse_move(int8_t dx, int8_t dy, int8_t wheel)
{
    static int total_attempts = 0;
    static int successful_sends = 0;
    static int failed_sends = 0;

    if (!s_hid_dev) {
        if (failed_sends++ < 3) {
            ESP_LOGE(TAG, "❌ Mouse move RECHAZADO: s_hid_dev es NULL");
        }
        return;
    }

    // Verificar estado ANTES de enviar
    bool connected = esp_hidd_dev_connected(s_hid_dev);

    ESP_LOGI(TAG, ">>> INTENTO ENVÍO #%d: dx=%d, dy=%d, wheel=%d",
             total_attempts + 1, dx, dy, wheel);
    ESP_LOGI(TAG, "    Estado: s_connected=%d, esp_hidd_dev_connected()=%d, s_hid_dev=%p",
             s_connected, connected, s_hid_dev);

    // INTENTAR ENVIAR SIEMPRE (ignorar el estado reportado)
    // El stack BLE puede estar conectado aunque esp_hidd_dev_connected() retorne false
    uint8_t report[4] = {0x00, (uint8_t)dx, (uint8_t)dy, (uint8_t)wheel};

    ESP_LOGI(TAG, "    >>> Llamando esp_hidd_dev_input_set()...");
    esp_err_t ret = esp_hidd_dev_input_set(s_hid_dev, 0, 0x01, report, sizeof(report));

    total_attempts++;

    if (ret != ESP_OK) {
        failed_sends++;
        ESP_LOGE(TAG, "    ❌ FALLO: %s (0x%x) - Total fallos: %d/%d",
                 esp_err_to_name(ret), ret, failed_sends, total_attempts);

        // Si el error es ESP_ERR_INVALID_STATE, podría significar que realmente no está conectado
        if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "    ERROR: ESP_ERR_INVALID_STATE - El HID no está listo para enviar");
        }
    } else {
        successful_sends++;
        ESP_LOGI(TAG, "    ✅ ÉXITO! Report enviado - Total éxitos: %d/%d",
                 successful_sends, total_attempts);

        // Si logramos enviar pero connected era false, hay un bug en la detección
        if (!connected) {
            ESP_LOGW(TAG, "    ⚠️  ADVERTENCIA: Envío exitoso pero esp_hidd_dev_connected() retornó FALSE!");
            ESP_LOGW(TAG, "    ⚠️  Esto indica un BUG en la detección de conexión HID");
        }
    }
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
