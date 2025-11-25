idf_component_register(
    SRCS "display_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl ble_sync settings bsp_extra
)
