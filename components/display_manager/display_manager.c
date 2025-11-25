#include "display_manager.h"
#include "bsp/display.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
//#include "nimble-nordic-uart.h"
#include "settings.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
// Power management
#include "esp_sleep.h"
#include "sdkconfig.h"
#if CONFIG_PM_ENABLE
#include "esp_pm.h"
#include "esp_err.h"
#include "bsp_power.h"
#include "bsp/display.h"

// Forward declarations por si el BSP no las declara en un header visible
esp_err_t bsp_display_sleep(void);
esp_err_t bsp_display_wake(void);
esp_err_t bsp_display_clear_black(void);
#endif

// If the board provides simple GPIO buttons, use one as wake key.
// On this hardware BSP_CAPS_BUTTONS is 0, so we will use the PMU PWR key
// instead.
#define DISPLAY_BUTTON GPIO_NUM_0

static const char *TAG = "DISPLAY_MGR";

static bool display_on = true;
static uint32_t timeout_ms;
#if CONFIG_PM_ENABLE
static esp_pm_lock_handle_t s_no_ls_lock = NULL;
#endif

static void display_turn_off_internal(void) {
  if (!display_on) {
    return;
  }
  ESP_LOGI(TAG, "Turning display off");
  // Stop LVGL timers to pause flushing while panel sleeps. Take LVGL lock to
  // avoid in-flight flush.
  if (lvgl_port_lock(200)) {
    lvgl_port_stop();
    lvgl_port_unlock();
  } else {
    lvgl_port_stop();
  }
  // Disable touch input polling; keep touch powered to allow IRQ wake
  lv_indev_t *indev = bsp_display_get_input_dev();
  if (indev) {
    lv_indev_enable(indev, false);
  }
  // Put panel into low-power sleep and ensure backlight is off
  bsp_display_sleep();
  bsp_display_brightness_set(0);
  // Hint BLE to prefer low-power connection parameters while screen is off
  //nordic_uart_set_low_power_mode(true);
  // If you rely on GPIO wake (touch or PMU IRQ), you may allow light sleep.
  // If wake via polling is required, DO NOT release the lock here.
  // For stability, keep CPU out of light sleep while screen is off.
  // This avoids missing wake events on boards without IRQ wiring.
  (void)0;
  display_on = false;
}

void display_manager_turn_off(void) { display_turn_off_internal(); }

void display_manager_turn_on(void) {
  if (!display_on) {
    ESP_LOGI(TAG, "Turning display on");
    // Wake the panel first, clear panel, then resume LVGL and restore brightness
    bsp_display_wake();
    (void)bsp_display_clear_black();
    lvgl_port_resume();

    if (lvgl_port_lock(200)) {
#if LVGL_VERSION_MAJOR >= 9
      lv_display_t *disp = lv_display_get_default();
      if (disp) {
        lv_obj_t *scr = lv_scr_act();
        if (scr) {
          lv_obj_invalidate(scr);
        }
      }
#else
      lv_disp_t *disp = lv_disp_get_default();
      if (disp) {
        lv_obj_t *scr = lv_disp_get_scr_act(disp);
        if (scr) {
          lv_obj_invalidate(scr);
        }
      }
#endif
      lvgl_port_unlock();
    }

    bsp_display_brightness_set(settings_get_brightness());
    // Re-enable touch input and release touch reset
    lv_indev_t *indev = bsp_display_get_input_dev();
    if (indev) {
      lv_indev_enable(indev, true);
    }
#if defined(BSP_LCD_TOUCH_RST)
    gpio_set_direction(BSP_LCD_TOUCH_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(BSP_LCD_TOUCH_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(5));
#endif
    display_on = true;
  }
  // Prevent light sleep while actively displaying UI for responsiveness
#if CONFIG_PM_ENABLE
  if (s_no_ls_lock) {
    (void)esp_pm_lock_acquire(s_no_ls_lock);
  }
#endif
  // Restore more responsive BLE params when screen is on
 // nordic_uart_set_low_power_mode(false);
  display_manager_reset_timer();
}

bool display_manager_is_on(void) { return display_on; }

void display_manager_reset_timer(void) { lv_disp_trig_activity(NULL); }

static void touch_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  switch (code) {
  case LV_EVENT_PRESSED:
  case LV_EVENT_PRESSING:
  case LV_EVENT_RELEASED:
  case LV_EVENT_CLICKED:
  case LV_EVENT_LONG_PRESSED:
  case LV_EVENT_LONG_PRESSED_REPEAT:
  case LV_EVENT_GESTURE:
    display_manager_reset_timer();
    break;
  default:
    break; // ignore non-input/render events
  }
}

static bool wake_button_pressed(void) {
#if BSP_CAPS_BUTTONS
  return gpio_get_level(DISPLAY_BUTTON) == 0;
#else
  // Poll AXP2101 power key short-press event
  return bsp_power_poll_pwr_button_short();
#endif
}

static void display_manager_task(void *arg) {
  ESP_LOGI(TAG, "Display manager task started");
  TickType_t last = xTaskGetTickCount();
  while (1) {
    // Refresh timeout from settings to apply changes immediately
    timeout_ms = settings_get_display_timeout();
    if (display_on) {
      uint32_t inactive = lv_disp_get_inactive_time(NULL);
      if (inactive >= timeout_ms) {
        display_turn_off_internal();
      }
      if (wake_button_pressed()) {
        display_manager_reset_timer();
        vTaskDelay(pdMS_TO_TICKS(100));
        last = xTaskGetTickCount();
      }
    } else {
      if (wake_button_pressed()) {
        display_manager_turn_on();
        vTaskDelay(pdMS_TO_TICKS(100));
        last = xTaskGetTickCount();
      }
    }
    vTaskDelayUntil(&last, pdMS_TO_TICKS(50));
  }
}

void display_manager_init(void) {
  timeout_ms = settings_get_display_timeout();
#if BSP_CAPS_BUTTONS
  gpio_config_t io_conf = {
      .pin_bit_mask = 1ULL << DISPLAY_BUTTON,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&io_conf);
#else
  ESP_LOGI(TAG, "Using PMU PWR key to wake display");
#endif

  lv_obj_add_event_cb(lv_scr_act(), touch_event_cb, 
    LV_EVENT_PRESSED | LV_EVENT_PRESSING | LV_EVENT_RELEASED | LV_EVENT_CLICKED | LV_EVENT_LONG_PRESSED | LV_EVENT_LONG_PRESSED_REPEAT | LV_EVENT_GESTURE, NULL);

  // PM lock may be created in early init; if not, create and acquire now
#if CONFIG_PM_ENABLE
  if (!s_no_ls_lock) {
    (void)esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "display",
                             &s_no_ls_lock);
  }
  if (s_no_ls_lock) {
    (void)esp_pm_lock_acquire(s_no_ls_lock);
  }
#endif
/*
  // Configure GPIO wake-ups so user input can wake CPU from light sleep
  // Only meaningful if PM/light-sleep is enabled and wake pins are wired.
#if CONFIG_PM_ENABLE
  // Touch INT is active-low on this board
  gpio_config_t touch_io = {
      .pin_bit_mask = 1ULL << BSP_LCD_TOUCH_INT,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  (void)gpio_config(&touch_io);
  (void)gpio_wakeup_enable(BSP_LCD_TOUCH_INT, GPIO_INTR_LOW_LEVEL);
#ifdef CONFIG_PMU_INTERRUPT_PIN
  // PMU IRQ (e.g., power key) also active-low
  gpio_config_t pmu_io = {
      .pin_bit_mask = 1ULL << CONFIG_PMU_INTERRUPT_PIN,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  (void)gpio_config(&pmu_io);
  (void)gpio_wakeup_enable(CONFIG_PMU_INTERRUPT_PIN, GPIO_INTR_LOW_LEVEL);
#endif
  (void)esp_sleep_enable_gpio_wakeup();
#endif // CONFIG_PM_ENABLE
*/
  // Higher priority so UI updates aren't delayed by other workloads
  xTaskCreate(display_manager_task, "display_mgr", 4000, NULL, 3, NULL);
}

void display_manager_pm_early_init(void) {
#if CONFIG_PM_ENABLE
  if (!s_no_ls_lock) {
    (void)esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "display",
                             &s_no_ls_lock);
  }
  if (s_no_ls_lock) {
    (void)esp_pm_lock_acquire(s_no_ls_lock);
  }
#else
  // No power management; nothing to do
  (void)0;
#endif
}

// -----------------------------------------------------------------------------
// Weak stub implementations for optional BSP / RTC helpers
// -----------------------------------------------------------------------------
//
// Some Waveshare BSP variants provide these helpers in additional components.
// On this board / project they may be missing, which would cause undefined
// reference linker errors.  We provide minimal weak fallbacks here so the
// firmware links even if the "real" implementations are not present.
// If you later add proper implementations in another component, the linker
// will prefer them over these weak versions.

__attribute__((weak)) esp_err_t bsp_display_sleep(void)
{
    ESP_LOGI(TAG, "bsp_display_sleep() stub – no real low‑power panel sleep");
    // Nothing special to do; panel driver (esp_lcd) will normally be powered down
    // together with the rest of the system.
    return ESP_OK;
}

__attribute__((weak)) esp_err_t bsp_display_wake(void)
{
    ESP_LOGI(TAG, "bsp_display_wake() stub – no real panel wake sequence");
    return ESP_OK;
}

__attribute__((weak)) esp_err_t bsp_display_clear_black(void)
{
    ESP_LOGI(TAG, "bsp_display_clear_black() stub – clear LVGL screen to black");
#if CONFIG_BSP_DISPLAY_LVGL_PORT
    if (lvgl_port_lock(0)) {
        lv_obj_t *scr = lv_scr_act();
        if (scr) {
            lv_obj_clean(scr);
            lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
        }
        lvgl_port_unlock();
    }
#endif
    return ESP_OK;
}

// Extra BSP glue that some code expects but is not mandatory on this build.
// Keeping the implementations here avoids pulling in more dependencies while
// still letting the rest of the code call into them.
__attribute__((weak)) esp_err_t bsp_extra_init(void)
{
    ESP_LOGI(TAG, "bsp_extra_init() stub – nothing to initialise");
    return ESP_OK;
}

// Simple I2C register helpers for the external PCF85063 RTC.
// The real project may provide more complete versions; these just make the
// driver happy and always report success.
__attribute__((weak)) esp_err_t rtc_register_read(uint8_t reg, uint8_t *value)
{
    ESP_LOGW(TAG, "rtc_register_read() stub – reg=0x%02x, returning 0", reg);
    if (value) {
        *value = 0;
    }
    return ESP_OK;
}

__attribute__((weak)) esp_err_t rtc_register_write(uint8_t reg, uint8_t value)
{
    ESP_LOGW(TAG, "rtc_register_write() stub – reg=0x%02x, val=0x%02x", reg, value);
    (void)reg;
    (void)value;
    return ESP_OK;
}
