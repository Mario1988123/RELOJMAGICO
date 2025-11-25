idf_component_register(
    SRCS "ble_sync.c"
    INCLUDE_DIRS "include"
    REQUIRES bt nvs_flash bsp_extra esp_event json sensors gui nimble-nordic-uart
)
