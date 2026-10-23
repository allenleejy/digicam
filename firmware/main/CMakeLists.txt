idf_component_register(
    SRCS "main.c" "lens.c" "storage.c" "avi.c" "link.c" "phone.c"
         "display.c" "gfx.c" "ui.c" "ui_draw.c" "buttons.c" "battery.c" "font_spleen.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_uart fatfs sdmmc esp_driver_sdmmc json esp_timer esp_rom
             esp_lcd esp_driver_spi esp_driver_ledc esp_driver_gpio esp_adc nvs_flash
             esp_wifi esp_netif esp_event esp_http_server
    EMBED_TXTFILES "../web/phone.html"     # built from web/phone.src.html by web/build.py
    EMBED_FILES "../web/icon-180.png"
)
