idf_component_register(
    SRCS
        "main.c"
        "tap_detector.c"
        "card_decoder.c"
        "wifi_beacon.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_wifi
        nvs_flash
        esp_timer
)
