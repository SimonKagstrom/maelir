cmake_minimum_required (VERSION 3.21)

project (maelir_esp32s3 LANGUAGES CXX C ASM)

# Include for ESP-IDF build system functions
include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

# Create idf::{target} and idf::freertos static libraries
idf_build_process(esp32s3
    # try and trim the build; additional components
    # will be included as needed based on dependency tree
    #
    # although esptool_py does not generate static library,
    # processing the component is needed for flashing related
    # targets and file generation
    COMPONENTS freertos esptool_py app_trace app_update bootloader bootloader_support bt cmock console cxx driver efuse esp-tls esp_adc esp_app_format esp_bootloader_format esp_coex esp_common esp_driver_ana_cmpr esp_driver_cam esp_driver_dac esp_driver_gpio esp_driver_gptimer esp_driver_i2c esp_driver_i2s esp_driver_isp esp_driver_jpeg esp_driver_ledc esp_driver_mcpwm esp_driver_parlio esp_driver_pcnt esp_driver_ppa esp_driver_rmt esp_driver_sdio esp_driver_sdm esp_driver_sdmmc esp_driver_sdspi esp_driver_spi esp_driver_tsens esp_driver_uart esp_driver_usb_serial_jtag esp_eth esp_event esp_gdbstub esp_hid esp_http_client esp_http_server esp_https_ota esp_https_server esp_hw_support esp_lcd esp_local_ctrl esp_mm esp_netif esp_netif_stack esp_partition esp_phy esp_pm esp_psram esp_ringbuf esp_rom esp_system esp_timer esp_vfs_console esp_wifi espcoredump esptool_py fatfs freertos hal heap http_parser idf_test ieee802154 json log lwip mbedtls mqtt newlib nvs_flash nvs_sec_provider openthread partition_table perfmon protobuf-c protocomm pthread sdmmc soc spi_flash spiffs tcp_transport touch_element ulp unity usb vfs wear_levelling wifi_provisioning wpa_supplicant xtensa esp_lcd
    SDKCONFIG ${CMAKE_CURRENT_LIST_DIR}/sdkconfig
    BUILD_DIR ${CMAKE_BINARY_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

find_package(etl REQUIRED)

add_subdirectory(.. maelir)
add_subdirectory(target_display)


set(elf_file ${CMAKE_PROJECT_NAME}.elf)
add_executable(${elf_file}
    main.cc

    os/base_thread_freertos.cc
    os/semaphore_freertos.cc
)

target_link_libraries(${elf_file}
    idf::freertos
    idf::spi_flash
    tile_producer
    ui
    gps_reader
    target_display
)

# Attach additional targets to the executable file for flashing,
# linker script generation, partition_table generation, etc.
idf_build_executable(${elf_file})
