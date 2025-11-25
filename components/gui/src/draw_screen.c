#include "draw_screen.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui_fonts.h"
#include <math.h>  // 👈 Añadir esto para sqrt()

static const char *TAG = "DRAW_SCREEN";

static lv_obj_t *s_draw_screen = NULL;
static lv_obj_t *s_draw_area  = NULL;
static lv_obj_t *s_btn_clear  = NULL;

/* Variables para dibujo continuo */
static lv_point_t s_last_point;
static bool s_has_last_point = false;

/* ============================================================
 * EVENTO DEL ÁREA DE DIBUJO MEJORADO (LÍNEAS CONTINUAS)
 * ========================================================== */
static void draw_area_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    
    if (!indev) return;

    lv_point_t current_point;
    lv_indev_get_point(indev, &current_point);

    if (code == LV_EVENT_PRESSED) {
        // Primer punto - crear un círculo
        lv_obj_t *dot = lv_obj_create(s_draw_area);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, 8, 8);  // Punto un poco más grande
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(dot, lv_color_white(), 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_pos(dot, current_point.x - 4, current_point.y - 4);
        
        s_last_point = current_point;
        s_has_last_point = true;
    }
    else if (code == LV_EVENT_PRESSING && s_has_last_point) {
        // Dibujar línea entre el último punto y el actual
        lv_coord_t dx = current_point.x - s_last_point.x;
        lv_coord_t dy = current_point.y - s_last_point.y;
        
        // Calcular distancia sin usar sqrt (más eficiente)
        lv_coord_t distance_sq = dx * dx + dy * dy;
        
        // Dibujar múltiples puntos para hacer la línea continua
        if (distance_sq > 25) {  // 5*5 = 25 (solo si hay suficiente distancia)
            lv_coord_t distance = (lv_coord_t)sqrt(distance_sq);
            lv_coord_t steps = distance / 3;  // Más puntos = más continuo
            
            for (lv_coord_t i = 1; i <= steps; i++) {
                float t = (float)i / (float)steps;
                lv_coord_t x = s_last_point.x + (lv_coord_t)(dx * t);
                lv_coord_t y = s_last_point.y + (lv_coord_t)(dy * t);
                
                lv_obj_t *dot = lv_obj_create(s_draw_area);
                lv_obj_remove_style_all(dot);
                lv_obj_set_size(dot, 6, 6);
                lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
                lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
                lv_obj_set_style_bg_color(dot, lv_color_white(), 0);
                lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_set_pos(dot, x - 3, y - 3);
            }
            
            s_last_point = current_point;
        }
    }
    else if (code == LV_EVENT_RELEASED) {
        s_has_last_point = false;
    }
}

/* ============================================================
 * BOTÓN CLEAR MEJORADO
 * ========================================================== */
static void btn_clear_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    if (s_draw_area) {
        lv_obj_clean(s_draw_area);
    }

    ESP_LOGI(TAG, "Pantalla de dibujo limpiada");
}

/* ============================================================
 * CREAR LA PANTALLA DRAW MEJORADA
 * ========================================================== */
lv_obj_t *draw_screen_get(void)
{
    if (s_draw_screen) {
        return s_draw_screen;
    }

    /* Pantalla vacía negra */
    s_draw_screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(s_draw_screen);
    lv_obj_set_style_bg_color(s_draw_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_draw_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_draw_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Título arriba */
    lv_obj_t *label_title = lv_label_create(s_draw_screen);
    lv_label_set_text(label_title, "DRAW");
    lv_obj_set_style_text_color(label_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_title, &font_bold_32, 0);
    lv_obj_set_align(label_title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(label_title, 6);

    /* Área de dibujo mejorada - con márgenes más conservadores */
    s_draw_area = lv_obj_create(s_draw_screen);
    lv_obj_remove_style_all(s_draw_area);
    lv_obj_clear_flag(s_draw_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(s_draw_area, LV_OPA_TRANSP, 0);
    
    /* Dimensiones más conservadoras para evitar cortes */
    lv_coord_t scr_w = lv_disp_get_hor_res(NULL);
    lv_coord_t scr_h = lv_disp_get_ver_res(NULL);
    
    // Dejar más espacio para el botón (80px en lugar de 70px)
    lv_coord_t draw_h = scr_h - 80;   // más margen inferior
    lv_coord_t draw_y = 45;           // más margen superior
    
    if (draw_h < 100) draw_h = scr_h - 80;  // mínimo seguro
    
    lv_obj_set_size(s_draw_area, scr_w - 20, draw_h); // margen lateral también
    lv_obj_align(s_draw_area, LV_ALIGN_TOP_MID, 0, draw_y);

    lv_obj_add_flag(s_draw_area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_draw_area, draw_area_event_cb, LV_EVENT_ALL, NULL);

    /* Botón CLEAR mejorado - más grande y mejor posicionado */
    s_btn_clear = lv_btn_create(s_draw_screen);
    lv_obj_set_size(s_btn_clear, 90, 40);  // Más grande
    lv_obj_align(s_btn_clear, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
    
    // Estilo del botón para mejor visibilidad
    lv_obj_set_style_bg_color(s_btn_clear, lv_color_hex(0x404040), 0);
    lv_obj_set_style_bg_opa(s_btn_clear, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_btn_clear, 8, 0);
    
    lv_obj_add_event_cb(s_btn_clear, btn_clear_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_clear = lv_label_create(s_btn_clear);
    lv_label_set_text(lbl_clear, "Clear");
    lv_obj_set_style_text_color(lbl_clear, lv_color_white(), 0);
    
    // 👈 CORRECCIÓN: Usar una fuente que exista
    // Opción 1: Si tienes font_bold_16 en ui_fonts.h
    lv_obj_set_style_text_font(lbl_clear, &font_bold_16, 0);
    // Opción 2: Si no tienes font_bold_16, usa montserrat_26 o prueba con:
    // lv_obj_set_style_text_font(lbl_clear, &lv_font_montserrat_14, 0);
    // Opción 3: Usar la fuente por defecto
    // lv_obj_set_style_text_font(lbl_clear, LV_FONT_DEFAULT, 0);
    
    lv_obj_center(lbl_clear);

    ESP_LOGI(TAG, "Pantalla DRAW creada - Resolución: %dx%d", scr_w, scr_h);
    
    return s_draw_screen;
}