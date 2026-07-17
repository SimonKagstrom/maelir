add_library(target_httpd_ota_updater EXCLUDE_FROM_ALL
    target_httpd_ota_updater.cc
)

target_include_directories(target_httpd_ota_updater
PUBLIC
    include
)

target_link_libraries(target_httpd_ota_updater
PUBLIC
    idf::app_update
    idf::driver
    idf::esp_http_server
    idf::esp_netif
    idf::esp_partition
    idf::esp_wifi
    idf::esp_eth
    idf::freertos
    idf::nvs_flash
    idf::esp-tls
    idf::wifi_provisioning
    maelir_interface
)
