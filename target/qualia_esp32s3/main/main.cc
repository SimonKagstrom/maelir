#include "encoder_input.hh"
#include "gps_mux.hh"
#include "gps_reader.hh"
#include "gps_simulator.hh"
#include "i2c_gps.hh"
#include "route_service.hh"
#include "sdkconfig.h"
#include "storage.hh"
#include "target_display.hh"
#include "target_nvm.hh"
#include "tile_producer.hh"
#include "uart_gps.hh"
#include "ui.hh"

#include <esp_io_expander_tca9554.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_io_additions.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_partition.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace
{


#define PCA_TFT_BACKLIGHT 4

#define TFT_DE    2
#define TFT_VSYNC 42
#define TFT_HSYNC 41
#define TFT_PCLK  1
#define TFT_R1    11
#define TFT_R2    10
#define TFT_R3    9
#define TFT_R4    46
#define TFT_R5    3
#define TFT_G0    48
#define TFT_G1    47
#define TFT_G2    21
#define TFT_G3    14
#define TFT_G4    13
#define TFT_G5    12
#define TFT_B1    40
#define TFT_B2    39
#define TFT_B3    38
#define TFT_B4    0
#define TFT_B5    45

constexpr auto kI2cBus = I2C_NUM_0;
constexpr auto kI2cSdaPin = GPIO_NUM_8;
constexpr auto kI2cSclPin = GPIO_NUM_18;

constexpr auto kI2cExpanderAddress = ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_111;

constexpr auto kExpanderTftReset = IO_EXPANDER_PIN_NUM_2;
constexpr auto kExpanderTftCs = IO_EXPANDER_PIN_NUM_1;
constexpr auto kExpanderTftBacklight = IO_EXPANDER_PIN_NUM_4;
constexpr auto kExpanderTftSck = IO_EXPANDER_PIN_NUM_0;
constexpr auto kExpanderTftMosi = IO_EXPANDER_PIN_NUM_7;


std::unique_ptr<DisplayTarget>
CreateDisplay()
{
    i2c_master_bus_handle_t bus_handle = nullptr;
    esp_io_expander_handle_t expander_handle = nullptr;

    const i2c_master_bus_config_t i2c_master_config = {.i2c_port = kI2cBus,
                                                       .sda_io_num = kI2cSdaPin,
                                                       .scl_io_num = kI2cSclPin,
                                                       .clk_source = I2C_CLK_SRC_DEFAULT,
                                                       .glitch_ignore_cnt = 7,
                                                       .intr_priority = 0,
                                                       .trans_queue_depth = 0,
                                                       .flags {.enable_internal_pullup = true}};
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master_config, &bus_handle));

    ESP_ERROR_CHECK(
        esp_io_expander_new_i2c_tca9554(bus_handle, kI2cExpanderAddress, &expander_handle));

    esp_io_expander_set_dir(expander_handle, kExpanderTftReset, IO_EXPANDER_OUTPUT);

    esp_io_expander_set_dir(expander_handle, kExpanderTftCs, IO_EXPANDER_OUTPUT);
    esp_io_expander_set_dir(expander_handle, kExpanderTftSck, IO_EXPANDER_OUTPUT);
    esp_io_expander_set_dir(expander_handle, kExpanderTftMosi, IO_EXPANDER_OUTPUT);
    esp_io_expander_set_dir(expander_handle, kExpanderTftBacklight, IO_EXPANDER_OUTPUT);

    esp_io_expander_set_level(
        expander_handle, kExpanderTftReset | kExpanderTftMosi | kExpanderTftCs, 1);

    spi_line_config_t line_config = {
        .cs_io_type = IO_TYPE_EXPANDER,
        .cs_gpio_num = kExpanderTftCs,
        .scl_io_type = IO_TYPE_EXPANDER,
        .scl_gpio_num = kExpanderTftSck,
        .sda_io_type = IO_TYPE_EXPANDER,
        .sda_gpio_num = kExpanderTftMosi,
        .io_expander = expander_handle,
    };
    esp_lcd_panel_io_3wire_spi_config_t io_config = {
        .line_config = line_config,
        .expect_clk_speed = 500 * 1000U,
        .spi_mode = 0,
        .lcd_cmd_bytes = 1,
        .lcd_param_bytes = 1,
        .flags =
            {
                .use_dc_bit = 1,
                .dc_zero_on_data = 0,
                .lsb_first = 0,
                .cs_high_active = 0,
                .del_keep_cs_inactive = 1,
            },
    };


    const esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_PLL240M,
        .timings =
            {
                .pclk_hz = 17 * 1000 * 1000,
                .h_res = hal::kDisplayWidth,
                .v_res = hal::kDisplayHeight,
                .hsync_pulse_width = 2,
                .hsync_back_porch = 44,
                .hsync_front_porch = 50,
                .vsync_pulse_width = 2,
                .vsync_back_porch = 18,
                .vsync_front_porch = 50,
                .flags {
                    .hsync_idle_low = false,
                    .vsync_idle_low = false,
                    .de_idle_high = false,
                    .pclk_active_neg = false,
                    .pclk_idle_high = false,
                },
            },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 0,
        .bounce_buffer_size_px = hal::kDisplayWidth * 10,
        .sram_trans_align = 64, // Deprecated
        .dma_burst_size = 64,
        .hsync_gpio_num = TFT_HSYNC,
        .vsync_gpio_num = TFT_VSYNC,
        .de_gpio_num = TFT_DE,
        .pclk_gpio_num = TFT_PCLK,
        .disp_gpio_num = GPIO_NUM_NC,
        .data_gpio_nums =
            {
                TFT_B1,
                TFT_B2,
                TFT_B3,
                TFT_B4,
                TFT_B5,
                TFT_G0,
                TFT_G1,
                TFT_G2,
                TFT_G3,
                TFT_G4,
                TFT_G5,
                TFT_R1,
                TFT_R2,
                TFT_R3,
                TFT_R4,
                TFT_R5,
            },
        .flags {
            .disp_active_low = 0,
            .refresh_on_demand = 0,
            .fb_in_psram = 0,
            .double_fb = 0,
            .no_fb = 1,
            .bb_invalidate_cache = 0,
        },
    };

    auto out = std::make_unique<DisplayTarget>(io_config, rgb_config);

    // Turn on the backlight
    esp_io_expander_set_level(expander_handle, kExpanderTftBacklight, 1);
    // For now, maybe keep it in the future
    esp_io_expander_del(expander_handle);
    i2c_del_master_bus(bus_handle);

    return out;
}

} // namespace

extern "C" void
app_main(void)
{
    auto map_partition =
        esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, "map_data");
    assert(map_partition);

    const void* p = nullptr;
    esp_partition_mmap_handle_t handle = 0;
    if (esp_partition_mmap(esp_partition_get(map_partition),
                           0,
                           esp_partition_get(map_partition)->size,
                           ESP_PARTITION_MMAP_DATA,
                           &p,
                           &handle) != ESP_OK)
    {
        assert(false);
    }
    esp_partition_iterator_release(map_partition);
    srand(esp_random());

    auto map_metadata = reinterpret_cast<const MapMetadata*>(p);

    auto target_nvm = std::make_unique<NvmTarget>();

    ApplicationState state;

    state.Checkout()->demo_mode = true;

    auto encoder_input = std::make_unique<EncoderInput>(6,   // Pin A -> 6 (MISO)
                                                        7,   // Pin B -> 7 (MOSI)
                                                        5,   // Button -> 5(SCK)
                                                        16); // Switch up -> 16 (A1)
    auto display = CreateDisplay();
    auto gps_device = std::make_unique<UartGps>(UART_NUM_1,
                                                17, // RX -> A0
                                                8); // TX -> CS (not used)

    //    auto gps_device = std::make_unique<I2cGps>(
    //            18, // SCL
    //            8); // SDA


    // Threads
    auto route_service = std::make_unique<RouteService>(*map_metadata);
    auto storage = std::make_unique<Storage>(*target_nvm, state, route_service->AttachListener());
    auto producer = std::make_unique<TileProducer>(state, *map_metadata);
    auto gps_simulator = std::make_unique<GpsSimulator>(*map_metadata, state, *route_service);

    // Selects between the real and demo GPS
    auto gps_mux = std::make_unique<GpsMux>(state, *gps_device, *gps_simulator);

    auto gps_reader = std::make_unique<GpsReader>(*map_metadata, *gps_mux);
    auto ui = std::make_unique<UserInterface>(state,
                                              *map_metadata,
                                              *producer,
                                              *display,
                                              *encoder_input,
                                              *route_service,
                                              gps_reader->AttachListener(),
                                              route_service->AttachListener());

    storage->Start(0);
    gps_simulator->Start(0);
    gps_reader->Start(0);
    producer->Start(1, os::ThreadPriority::kNormal);
    route_service->Start(0, os::ThreadPriority::kNormal, 5000);

    // Time for the storage to read the home position
    os::Sleep(10ms);
    ui->Start(1, os::ThreadPriority::kHigh, 8192);

    while (true)
    {
        vTaskSuspend(nullptr);
    }
}
