idf_component_register(

    SRCS
    "src/hexnet_io_profile.c"
    "src/hexnet_bluetooth.c"
    "src/hexnet_wifi.c"
    "src/hexnet_wifi_portal.c"
    "src/hexnet_http.c"
    "src/hexnet_debug.c"
    "src/dns_server.c"
    "src/hexnet_nvs.c"
    "src/hexnet_canbus.c"

    REQUIRES
    esp_timer
    driver
    freertos
    json
    nvs_flash
    esp_event
    esp_netif
    esp_wifi
    esp_http_server
    esp_http_client
    app_update
    lwip
    bt

    INCLUDE_DIRS
    "inc"

    EMBED_FILES
    "favicon.ico" "upload_script.html"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    HEXNET_LOG_WIFI=1
)
