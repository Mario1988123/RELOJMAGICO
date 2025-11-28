#include "magic_trick_screen.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui_fonts.h"
#include "ble_hid_combined.h"
#include <stdio.h>

static const char *TAG = "MAGIC_TRICK";

static lv_obj_t *s_screen = NULL;
static lv_obj_t *s_lbl_suit = NULL;     // Palo actual
static lv_obj_t *s_lbl_value = NULL;    // Valor actual
static lv_obj_t *s_lbl_status = NULL;   // Estado de conexión
static lv_obj_t *s_lbl_central = NULL;  // Display central grande
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

    bool connected = ble_hid_combined_is_connected();
    if (connected) {
        lv_label_set_text(s_lbl_status, "C");  // Solo una letra, casi invisible
        lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(0x252525), 0);
    } else {
        lv_label_set_text(s_lbl_status, "");  // Vacío si no está conectado
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

    // Actualizar display central grande
    if (s_lbl_central && s_suit >= 1 && s_suit <= 4 && s_value >= 1 && s_value <= 13) {
        char central_text[64];
        snprintf(central_text, sizeof(central_text), "%s\n%s",
                 value_names[s_value], suit_names[s_suit]);
        lv_label_set_text(s_lbl_central, central_text);
    }
}

/* ============================================================
 * ENVIAR CARTA AL MÓVIL
 * ========================================================== */
static void send_card_to_phone(void)
{
    if (!ble_hid_combined_is_connected()) {
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

    /* Título - MOE en gris oscuro (casi invisible) */
    lv_obj_t *lbl_title = lv_label_create(s_screen);
    lv_label_set_text(lbl_title, "MOE");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x202020), 0);  // Gris muy oscuro
    lv_obj_set_style_text_font(lbl_title, &font_normal_26, 0);
    lv_obj_set_align(lbl_title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(lbl_title, 5);

    /* Estado de conexión - gris oscuro */
    s_lbl_status = lv_label_create(s_screen);
    lv_label_set_text(s_lbl_status, "");
    lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(0x303030), 0);
    lv_obj_set_style_text_font(s_lbl_status, &font_normal_26, 0);
    lv_obj_set_align(s_lbl_status, LV_ALIGN_TOP_MID);
    lv_obj_set_y(s_lbl_status, 30);

    /* Contenedor para PALO - más compacto arriba */
    lv_obj_t *cont_suit = lv_obj_create(s_screen);
    lv_obj_remove_style_all(cont_suit);
    lv_obj_set_size(cont_suit, 180, 80);
    lv_obj_align(cont_suit, LV_ALIGN_CENTER, -100, -80);
    lv_obj_set_flex_flow(cont_suit, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont_suit, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *lbl_suit_title = lv_label_create(cont_suit);
    lv_label_set_text(lbl_suit_title, "P");
    lv_obj_set_style_text_color(lbl_suit_title, lv_color_hex(0x252525), 0);  // Gris oscuro
    lv_obj_set_style_text_font(lbl_suit_title, &font_normal_26, 0);

    /* Botón UP para palo - gris oscuro */
    lv_obj_t *btn_suit_up = lv_btn_create(cont_suit);
    lv_obj_set_size(btn_suit_up, 50, 30);
    lv_obj_set_style_bg_color(btn_suit_up, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(btn_suit_up, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(btn_suit_up, btn_suit_up_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_up1 = lv_label_create(btn_suit_up);
    lv_label_set_text(lbl_up1, "+");
    lv_obj_set_style_text_color(lbl_up1, lv_color_hex(0x303030), 0);
    lv_obj_center(lbl_up1);

    /* Display del palo - MÁS GRANDE */
    s_lbl_suit = lv_label_create(cont_suit);
    lv_label_set_text(s_lbl_suit, "CORAZONES");
    lv_obj_set_style_text_color(s_lbl_suit, lv_color_hex(0x404040), 0);  // Gris medio oscuro
    lv_obj_set_style_text_font(s_lbl_suit, &font_normal_26, 0);

    /* Botón DOWN para palo - gris oscuro */
    lv_obj_t *btn_suit_down = lv_btn_create(cont_suit);
    lv_obj_set_size(btn_suit_down, 50, 30);
    lv_obj_set_style_bg_color(btn_suit_down, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(btn_suit_down, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(btn_suit_down, btn_suit_down_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_down1 = lv_label_create(btn_suit_down);
    lv_label_set_text(lbl_down1, "-");
    lv_obj_set_style_text_color(lbl_down1, lv_color_hex(0x303030), 0);
    lv_obj_center(lbl_down1);

    /* Contenedor para VALOR - más compacto */
    lv_obj_t *cont_value = lv_obj_create(s_screen);
    lv_obj_remove_style_all(cont_value);
    lv_obj_set_size(cont_value, 140, 80);
    lv_obj_align(cont_value, LV_ALIGN_CENTER, 95, -80);
    lv_obj_set_flex_flow(cont_value, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont_value, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *lbl_value_title = lv_label_create(cont_value);
    lv_label_set_text(lbl_value_title, "V");
    lv_obj_set_style_text_color(lbl_value_title, lv_color_hex(0x252525), 0);  // Gris oscuro
    lv_obj_set_style_text_font(lbl_value_title, &font_normal_26, 0);

    /* Botón UP para valor - gris oscuro */
    lv_obj_t *btn_value_up = lv_btn_create(cont_value);
    lv_obj_set_size(btn_value_up, 50, 30);
    lv_obj_set_style_bg_color(btn_value_up, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(btn_value_up, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(btn_value_up, btn_value_up_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_up2 = lv_label_create(btn_value_up);
    lv_label_set_text(lbl_up2, "+");
    lv_obj_set_style_text_color(lbl_up2, lv_color_hex(0x303030), 0);
    lv_obj_center(lbl_up2);

    /* Display del valor - MUCHO MÁS GRANDE */
    s_lbl_value = lv_label_create(cont_value);
    lv_label_set_text(s_lbl_value, "AS");
    lv_obj_set_style_text_color(s_lbl_value, lv_color_hex(0x505050), 0);  // Gris medio
    lv_obj_set_style_text_font(s_lbl_value, &font_bold_32, 0);

    /* Botón DOWN para valor - gris oscuro */
    lv_obj_t *btn_value_down = lv_btn_create(cont_value);
    lv_obj_set_size(btn_value_down, 50, 30);
    lv_obj_set_style_bg_color(btn_value_down, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(btn_value_down, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(btn_value_down, btn_value_down_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_down2 = lv_label_create(btn_value_down);
    lv_label_set_text(lbl_down2, "-");
    lv_obj_set_style_text_color(lbl_down2, lv_color_hex(0x303030), 0);
    lv_obj_center(lbl_down2);

    /* ZONA CENTRAL AMPLIADA - Display grande de la carta seleccionada */
    lv_obj_t *central_display = lv_obj_create(s_screen);
    lv_obj_remove_style_all(central_display);
    lv_obj_set_size(central_display, 350, 200);  // MUCHO MÁS GRANDE
    lv_obj_align(central_display, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(central_display, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(central_display, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(central_display, 10, 0);
    lv_obj_set_style_border_width(central_display, 1, 0);
    lv_obj_set_style_border_color(central_display, lv_color_hex(0x202020), 0);

    /* Texto central - GRANDE y en gris oscuro */
    s_lbl_central = lv_label_create(central_display);
    lv_label_set_text(s_lbl_central, "AS\nCORAZONES");
    lv_obj_set_style_text_color(s_lbl_central, lv_color_hex(0x404040), 0);
    lv_obj_set_style_text_font(s_lbl_central, &font_bold_42, 0);
    lv_obj_set_style_text_align(s_lbl_central, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(s_lbl_central);

    /* Botón SEND (pequeño, gris oscuro abajo) */
    lv_obj_t *btn_send = lv_btn_create(s_screen);
    lv_obj_set_size(btn_send, 120, 45);
    lv_obj_align(btn_send, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_bg_color(btn_send, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(btn_send, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn_send, 8, 0);
    lv_obj_add_event_cb(btn_send, btn_send_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_send = lv_label_create(btn_send);
    lv_label_set_text(lbl_send, "OK");
    lv_obj_set_style_text_color(lbl_send, lv_color_hex(0x303030), 0);
    lv_obj_set_style_text_font(lbl_send, &font_normal_26, 0);
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
