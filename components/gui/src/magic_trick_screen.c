#include "magic_trick_screen.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui_fonts.h"
#include "ble_hid_keyboard.h"
#include <stdio.h>

static const char *TAG = "MAGIC_TRICK";

static lv_obj_t *s_screen = NULL;
static lv_obj_t *s_lbl_suit = NULL;     // Palo actual
static lv_obj_t *s_lbl_value = NULL;    // Valor actual
static lv_obj_t *s_lbl_status = NULL;   // Estado de conexión
static lv_timer_t *s_status_timer = NULL;

static int s_suit = 1;   // 1=Corazones, 2=Picas, 3=Tréboles, 4=Diamantes
static int s_value = 1;  // 1=As, 2-10=números, 11=J, 12=Q, 13=K

/* Nombres de palos en español */
static const char *suit_names[] = {
    "",
    "CORAZONES",  // 1
    "PICAS",      // 2
    "TREBOLES",   // 3
    "DIAMANTES"   // 4
};

/* Símbolos Unicode de palos */
static const char *suit_symbols[] = {
    "",
    "♥",  // Corazones
    "♠",  // Picas
    "♣",  // Tréboles
    "♦"   // Diamantes
};

/* Nombres de valores */
static const char *value_names[] = {
    "",
    "AS", "2", "3", "4", "5", "6", "7", "8", "9", "10",
    "J", "Q", "K"
};

/* ============================================================
 * TIMER PARA ACTUALIZAR ESTADO
 * ========================================================== */
static void status_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_lbl_status) return;

    bool connected = ble_hid_keyboard_is_connected();
    if (connected) {
        lv_label_set_text(s_lbl_status, "[Connected]");
        lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(s_lbl_status, "[Not Connected]");
        lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(0xFF6060), 0);
    }
}

/* ============================================================
 * ACTUALIZAR DISPLAY
 * ========================================================== */
static void update_display(void)
{
    if (s_lbl_suit && s_suit >= 1 && s_suit <= 4) {
        lv_label_set_text(s_lbl_suit, suit_names[s_suit]);
    }

    if (s_lbl_value && s_value >= 1 && s_value <= 13) {
        lv_label_set_text(s_lbl_value, value_names[s_value]);
    }
}

/* ============================================================
 * ENVIAR CARTA AL MÓVIL
 * ========================================================== */
static void send_card_to_phone(void)
{
    if (!ble_hid_keyboard_is_connected()) {
        ESP_LOGW(TAG, "No hay conexión HID Keyboard");
        return;
    }

    // Formato: "AS de CORAZONES"
    char card_text[64];
    snprintf(card_text, sizeof(card_text), "%s de %s",
             value_names[s_value], suit_names[s_suit]);

    ESP_LOGI(TAG, "Enviando carta: %s", card_text);
    ble_hid_keyboard_send_text(card_text);
    ble_hid_keyboard_send_key(HID_KEY_ENTER);  // Enviar Enter al final
}

/* ============================================================
 * BOTONES DE CONTROL
 * ========================================================== */
static void btn_suit_up_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    s_suit++;
    if (s_suit > 4) s_suit = 1;
    update_display();
    ESP_LOGI(TAG, "Palo: %d - %s", s_suit, suit_names[s_suit]);
}

static void btn_suit_down_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    s_suit--;
    if (s_suit < 1) s_suit = 4;
    update_display();
    ESP_LOGI(TAG, "Palo: %d - %s", s_suit, suit_names[s_suit]);
}

static void btn_value_up_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    s_value++;
    if (s_value > 13) s_value = 1;
    update_display();
    ESP_LOGI(TAG, "Valor: %d - %s", s_value, value_names[s_value]);
}

static void btn_value_down_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    s_value--;
    if (s_value < 1) s_value = 13;
    update_display();
    ESP_LOGI(TAG, "Valor: %d - %s", s_value, value_names[s_value]);
}

static void btn_send_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    send_card_to_phone();
}

/* ============================================================
 * CREAR PANTALLA
 * ========================================================== */
lv_obj_t *magic_trick_screen_get(void)
{
    if (s_screen) {
        return s_screen;
    }

    /* Pantalla negra */
    s_screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(s_screen);
    lv_obj_set_style_bg_color(s_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Título */
    lv_obj_t *lbl_title = lv_label_create(s_screen);
    lv_label_set_text(lbl_title, "MAGIC TRICK");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFD700), 0);
    lv_obj_set_style_text_font(lbl_title, &font_bold_32, 0);
    lv_obj_set_align(lbl_title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(lbl_title, 10);

    /* Estado de conexión */
    s_lbl_status = lv_label_create(s_screen);
    lv_label_set_text(s_lbl_status, "[Checking...]");
    lv_obj_set_style_text_color(s_lbl_status, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_lbl_status, &font_normal_26, 0);
    lv_obj_set_align(s_lbl_status, LV_ALIGN_TOP_MID);
    lv_obj_set_y(s_lbl_status, 45);

    /* Contenedor para PALO */
    lv_obj_t *cont_suit = lv_obj_create(s_screen);
    lv_obj_remove_style_all(cont_suit);
    lv_obj_set_size(cont_suit, 200, 100);
    lv_obj_align(cont_suit, LV_ALIGN_CENTER, -100, -40);
    lv_obj_set_flex_flow(cont_suit, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont_suit, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *lbl_suit_title = lv_label_create(cont_suit);
    lv_label_set_text(lbl_suit_title, "PALO");
    lv_obj_set_style_text_color(lbl_suit_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_suit_title, &font_normal_26, 0);

    /* Botón UP para palo */
    lv_obj_t *btn_suit_up = lv_btn_create(cont_suit);
    lv_obj_set_size(btn_suit_up, 60, 40);
    lv_obj_set_style_bg_color(btn_suit_up, lv_color_hex(0x3B82F6), 0);
    lv_obj_add_event_cb(btn_suit_up, btn_suit_up_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_up1 = lv_label_create(btn_suit_up);
    lv_label_set_text(lbl_up1, "+");
    lv_obj_center(lbl_up1);

    /* Display del palo */
    s_lbl_suit = lv_label_create(cont_suit);
    lv_label_set_text(s_lbl_suit, "CORAZONES");
    lv_obj_set_style_text_color(s_lbl_suit, lv_color_hex(0xFF6060), 0);
    lv_obj_set_style_text_font(s_lbl_suit, &font_bold_26, 0);

    /* Botón DOWN para palo */
    lv_obj_t *btn_suit_down = lv_btn_create(cont_suit);
    lv_obj_set_size(btn_suit_down, 60, 40);
    lv_obj_set_style_bg_color(btn_suit_down, lv_color_hex(0x3B82F6), 0);
    lv_obj_add_event_cb(btn_suit_down, btn_suit_down_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_down1 = lv_label_create(btn_suit_down);
    lv_label_set_text(lbl_down1, "-");
    lv_obj_center(lbl_down1);

    /* Contenedor para VALOR */
    lv_obj_t *cont_value = lv_obj_create(s_screen);
    lv_obj_remove_style_all(cont_value);
    lv_obj_set_size(cont_value, 150, 100);
    lv_obj_align(cont_value, LV_ALIGN_CENTER, 100, -40);
    lv_obj_set_flex_flow(cont_value, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont_value, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *lbl_value_title = lv_label_create(cont_value);
    lv_label_set_text(lbl_value_title, "VALOR");
    lv_obj_set_style_text_color(lbl_value_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_value_title, &font_normal_26, 0);

    /* Botón UP para valor */
    lv_obj_t *btn_value_up = lv_btn_create(cont_value);
    lv_obj_set_size(btn_value_up, 60, 40);
    lv_obj_set_style_bg_color(btn_value_up, lv_color_hex(0x3B82F6), 0);
    lv_obj_add_event_cb(btn_value_up, btn_value_up_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_up2 = lv_label_create(btn_value_up);
    lv_label_set_text(lbl_up2, "+");
    lv_obj_center(lbl_up2);

    /* Display del valor */
    s_lbl_value = lv_label_create(cont_value);
    lv_label_set_text(s_lbl_value, "AS");
    lv_obj_set_style_text_color(s_lbl_value, lv_color_hex(0x90F090), 0);
    lv_obj_set_style_text_font(s_lbl_value, &font_bold_42, 0);

    /* Botón DOWN para valor */
    lv_obj_t *btn_value_down = lv_btn_create(cont_value);
    lv_obj_set_size(btn_value_down, 60, 40);
    lv_obj_set_style_bg_color(btn_value_down, lv_color_hex(0x3B82F6), 0);
    lv_obj_add_event_cb(btn_value_down, btn_value_down_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_down2 = lv_label_create(btn_value_down);
    lv_label_set_text(lbl_down2, "-");
    lv_obj_center(lbl_down2);

    /* Botón SEND (grande, abajo) */
    lv_obj_t *btn_send = lv_btn_create(s_screen);
    lv_obj_set_size(btn_send, 200, 60);
    lv_obj_align(btn_send, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(btn_send, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_radius(btn_send, 10, 0);
    lv_obj_add_event_cb(btn_send, btn_send_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_send = lv_label_create(btn_send);
    lv_label_set_text(lbl_send, "ENVIAR CARTA");
    lv_obj_set_style_text_color(lbl_send, lv_color_black(), 0);
    lv_obj_set_style_text_font(lbl_send, &font_bold_26, 0);
    lv_obj_center(lbl_send);

    /* Crear timer para estado */
    if (!s_status_timer) {
        s_status_timer = lv_timer_create(status_timer_cb, 1000, NULL);
        lv_timer_ready(s_status_timer);
    }

    update_display();

    ESP_LOGI(TAG, "Pantalla Magic Trick creada");
    return s_screen;
}
