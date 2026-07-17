#include "display_jd9365.hh"
#include "encoder_input.hh"
#include "gps_mux.hh"
#include "gps_reader.hh"
#include "gps_simulator.hh"
#include "ota_updater.hh"
#include "route_service.hh"
#include "route_utils.hh"
#include "sdkconfig.h"
#include "storage.hh"
#include "target_httpd_ota_updater.hh"
#include "target_nvm.hh"
#include "target_uart.hh"
#include "tile_producer.hh"
#include "trip_computer.hh"
#include "uart_event_listener.hh"
#include "ui.hh"

#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <esp_ldo_regulator.h>
#include <esp_partition.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace
{

constexpr auto kTftBacklight = GPIO_NUM_26;


#define TEST_LCD_BIT_PER_PIXEL (24)
#define TEST_PIN_NUM_LCD_RST   (27)
#define TEST_PIN_NUM_BK_LIGHT  (26) // set to -1 if not used
#define TEST_MIPI_DSI_LANE_NUM (2)

#define TEST_MIPI_DPI_PX_FORMAT (LCD_COLOR_PIXEL_FORMAT_RGB565)

#define TEST_DELAY_TIME_MS (3000)

#define TEST_MIPI_DSI_PHY_PWR_LDO_CHAN       (3)
#define TEST_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV (2500)

constexpr jd9365_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xE0, (uint8_t[]) {0x00}, 1, 0},

    {0xE1, (uint8_t[]) {0x93}, 1, 0},   {0xE2, (uint8_t[]) {0x65}, 1, 0},
    {0xE3, (uint8_t[]) {0xF8}, 1, 0},   {0x80, (uint8_t[]) {0x01}, 1, 0},

    {0xE0, (uint8_t[]) {0x01}, 1, 0},

    {0x00, (uint8_t[]) {0x00}, 1, 0},   {0x01, (uint8_t[]) {0x41}, 1, 0},
    {0x03, (uint8_t[]) {0x10}, 1, 0},   {0x04, (uint8_t[]) {0x44}, 1, 0},

    {0x17, (uint8_t[]) {0x00}, 1, 0},   {0x18, (uint8_t[]) {0xD0}, 1, 0},
    {0x19, (uint8_t[]) {0x00}, 1, 0},   {0x1A, (uint8_t[]) {0x00}, 1, 0},
    {0x1B, (uint8_t[]) {0xD0}, 1, 0},   {0x1C, (uint8_t[]) {0x00}, 1, 0},

    {0x24, (uint8_t[]) {0xFE}, 1, 0},   {0x35, (uint8_t[]) {0x26}, 1, 0},

    {0x37, (uint8_t[]) {0x09}, 1, 0},

    {0x38, (uint8_t[]) {0x04}, 1, 0},   {0x39, (uint8_t[]) {0x08}, 1, 0},
    {0x3A, (uint8_t[]) {0x0A}, 1, 0},   {0x3C, (uint8_t[]) {0x78}, 1, 0},
    {0x3D, (uint8_t[]) {0xFF}, 1, 0},   {0x3E, (uint8_t[]) {0xFF}, 1, 0},
    {0x3F, (uint8_t[]) {0xFF}, 1, 0},

    {0x40, (uint8_t[]) {0x00}, 1, 0},   {0x41, (uint8_t[]) {0x64}, 1, 0},
    {0x42, (uint8_t[]) {0xC7}, 1, 0},   {0x43, (uint8_t[]) {0x18}, 1, 0},
    {0x44, (uint8_t[]) {0x0B}, 1, 0},   {0x45, (uint8_t[]) {0x14}, 1, 0},

    {0x55, (uint8_t[]) {0x02}, 1, 0},   {0x57, (uint8_t[]) {0x49}, 1, 0},
    {0x59, (uint8_t[]) {0x0A}, 1, 0},   {0x5A, (uint8_t[]) {0x1B}, 1, 0},
    {0x5B, (uint8_t[]) {0x19}, 1, 0},

    {0x5D, (uint8_t[]) {0x7F}, 1, 0},   {0x5E, (uint8_t[]) {0x56}, 1, 0},
    {0x5F, (uint8_t[]) {0x43}, 1, 0},   {0x60, (uint8_t[]) {0x37}, 1, 0},
    {0x61, (uint8_t[]) {0x33}, 1, 0},   {0x62, (uint8_t[]) {0x25}, 1, 0},
    {0x63, (uint8_t[]) {0x2A}, 1, 0},   {0x64, (uint8_t[]) {0x16}, 1, 0},
    {0x65, (uint8_t[]) {0x30}, 1, 0},   {0x66, (uint8_t[]) {0x2F}, 1, 0},
    {0x67, (uint8_t[]) {0x32}, 1, 0},   {0x68, (uint8_t[]) {0x53}, 1, 0},
    {0x69, (uint8_t[]) {0x43}, 1, 0},   {0x6A, (uint8_t[]) {0x4C}, 1, 0},
    {0x6B, (uint8_t[]) {0x40}, 1, 0},   {0x6C, (uint8_t[]) {0x3D}, 1, 0},
    {0x6D, (uint8_t[]) {0x31}, 1, 0},   {0x6E, (uint8_t[]) {0x20}, 1, 0},
    {0x6F, (uint8_t[]) {0x0F}, 1, 0},

    {0x70, (uint8_t[]) {0x7F}, 1, 0},   {0x71, (uint8_t[]) {0x56}, 1, 0},
    {0x72, (uint8_t[]) {0x43}, 1, 0},   {0x73, (uint8_t[]) {0x37}, 1, 0},
    {0x74, (uint8_t[]) {0x33}, 1, 0},   {0x75, (uint8_t[]) {0x25}, 1, 0},
    {0x76, (uint8_t[]) {0x2A}, 1, 0},   {0x77, (uint8_t[]) {0x16}, 1, 0},
    {0x78, (uint8_t[]) {0x30}, 1, 0},   {0x79, (uint8_t[]) {0x2F}, 1, 0},
    {0x7A, (uint8_t[]) {0x32}, 1, 0},   {0x7B, (uint8_t[]) {0x53}, 1, 0},
    {0x7C, (uint8_t[]) {0x43}, 1, 0},   {0x7D, (uint8_t[]) {0x4C}, 1, 0},
    {0x7E, (uint8_t[]) {0x40}, 1, 0},   {0x7F, (uint8_t[]) {0x3D}, 1, 0},
    {0x80, (uint8_t[]) {0x31}, 1, 0},   {0x81, (uint8_t[]) {0x20}, 1, 0},
    {0x82, (uint8_t[]) {0x0F}, 1, 0},

    {0xE0, (uint8_t[]) {0x02}, 1, 0},   {0x00, (uint8_t[]) {0x5F}, 1, 0},
    {0x01, (uint8_t[]) {0x5F}, 1, 0},   {0x02, (uint8_t[]) {0x5E}, 1, 0},
    {0x03, (uint8_t[]) {0x5E}, 1, 0},   {0x04, (uint8_t[]) {0x50}, 1, 0},
    {0x05, (uint8_t[]) {0x48}, 1, 0},   {0x06, (uint8_t[]) {0x48}, 1, 0},
    {0x07, (uint8_t[]) {0x4A}, 1, 0},   {0x08, (uint8_t[]) {0x4A}, 1, 0},
    {0x09, (uint8_t[]) {0x44}, 1, 0},   {0x0A, (uint8_t[]) {0x44}, 1, 0},
    {0x0B, (uint8_t[]) {0x46}, 1, 0},   {0x0C, (uint8_t[]) {0x46}, 1, 0},
    {0x0D, (uint8_t[]) {0x5F}, 1, 0},   {0x0E, (uint8_t[]) {0x5F}, 1, 0},
    {0x0F, (uint8_t[]) {0x57}, 1, 0},   {0x10, (uint8_t[]) {0x57}, 1, 0},
    {0x11, (uint8_t[]) {0x77}, 1, 0},   {0x12, (uint8_t[]) {0x77}, 1, 0},
    {0x13, (uint8_t[]) {0x40}, 1, 0},   {0x14, (uint8_t[]) {0x42}, 1, 0},
    {0x15, (uint8_t[]) {0x5F}, 1, 0},

    {0x16, (uint8_t[]) {0x5F}, 1, 0},   {0x17, (uint8_t[]) {0x5F}, 1, 0},
    {0x18, (uint8_t[]) {0x5E}, 1, 0},   {0x19, (uint8_t[]) {0x5E}, 1, 0},
    {0x1A, (uint8_t[]) {0x50}, 1, 0},   {0x1B, (uint8_t[]) {0x49}, 1, 0},
    {0x1C, (uint8_t[]) {0x49}, 1, 0},   {0x1D, (uint8_t[]) {0x4B}, 1, 0},
    {0x1E, (uint8_t[]) {0x4B}, 1, 0},   {0x1F, (uint8_t[]) {0x45}, 1, 0},
    {0x20, (uint8_t[]) {0x45}, 1, 0},   {0x21, (uint8_t[]) {0x47}, 1, 0},
    {0x22, (uint8_t[]) {0x47}, 1, 0},   {0x23, (uint8_t[]) {0x5F}, 1, 0},
    {0x24, (uint8_t[]) {0x5F}, 1, 0},   {0x25, (uint8_t[]) {0x57}, 1, 0},
    {0x26, (uint8_t[]) {0x57}, 1, 0},   {0x27, (uint8_t[]) {0x77}, 1, 0},
    {0x28, (uint8_t[]) {0x77}, 1, 0},   {0x29, (uint8_t[]) {0x41}, 1, 0},
    {0x2A, (uint8_t[]) {0x43}, 1, 0},   {0x2B, (uint8_t[]) {0x5F}, 1, 0},

    {0x2C, (uint8_t[]) {0x1E}, 1, 0},   {0x2D, (uint8_t[]) {0x1E}, 1, 0},
    {0x2E, (uint8_t[]) {0x1F}, 1, 0},   {0x2F, (uint8_t[]) {0x1F}, 1, 0},
    {0x30, (uint8_t[]) {0x10}, 1, 0},   {0x31, (uint8_t[]) {0x07}, 1, 0},
    {0x32, (uint8_t[]) {0x07}, 1, 0},   {0x33, (uint8_t[]) {0x05}, 1, 0},
    {0x34, (uint8_t[]) {0x05}, 1, 0},   {0x35, (uint8_t[]) {0x0B}, 1, 0},
    {0x36, (uint8_t[]) {0x0B}, 1, 0},   {0x37, (uint8_t[]) {0x09}, 1, 0},
    {0x38, (uint8_t[]) {0x09}, 1, 0},   {0x39, (uint8_t[]) {0x1F}, 1, 0},
    {0x3A, (uint8_t[]) {0x1F}, 1, 0},   {0x3B, (uint8_t[]) {0x17}, 1, 0},
    {0x3C, (uint8_t[]) {0x17}, 1, 0},   {0x3D, (uint8_t[]) {0x17}, 1, 0},
    {0x3E, (uint8_t[]) {0x17}, 1, 0},   {0x3F, (uint8_t[]) {0x03}, 1, 0},
    {0x40, (uint8_t[]) {0x01}, 1, 0},   {0x41, (uint8_t[]) {0x1F}, 1, 0},

    {0x42, (uint8_t[]) {0x1E}, 1, 0},   {0x43, (uint8_t[]) {0x1E}, 1, 0},
    {0x44, (uint8_t[]) {0x1F}, 1, 0},   {0x45, (uint8_t[]) {0x1F}, 1, 0},
    {0x46, (uint8_t[]) {0x10}, 1, 0},   {0x47, (uint8_t[]) {0x06}, 1, 0},
    {0x48, (uint8_t[]) {0x06}, 1, 0},   {0x49, (uint8_t[]) {0x04}, 1, 0},
    {0x4A, (uint8_t[]) {0x04}, 1, 0},   {0x4B, (uint8_t[]) {0x0A}, 1, 0},
    {0x4C, (uint8_t[]) {0x0A}, 1, 0},   {0x4D, (uint8_t[]) {0x08}, 1, 0},
    {0x4E, (uint8_t[]) {0x08}, 1, 0},   {0x4F, (uint8_t[]) {0x1F}, 1, 0},
    {0x50, (uint8_t[]) {0x1F}, 1, 0},   {0x51, (uint8_t[]) {0x17}, 1, 0},
    {0x52, (uint8_t[]) {0x17}, 1, 0},   {0x53, (uint8_t[]) {0x17}, 1, 0},
    {0x54, (uint8_t[]) {0x17}, 1, 0},   {0x55, (uint8_t[]) {0x02}, 1, 0},
    {0x56, (uint8_t[]) {0x00}, 1, 0},   {0x57, (uint8_t[]) {0x1F}, 1, 0},

    {0xE0, (uint8_t[]) {0x02}, 1, 0},   {0x58, (uint8_t[]) {0x40}, 1, 0},
    {0x59, (uint8_t[]) {0x00}, 1, 0},   {0x5A, (uint8_t[]) {0x00}, 1, 0},
    {0x5B, (uint8_t[]) {0x30}, 1, 0},   {0x5C, (uint8_t[]) {0x01}, 1, 0},
    {0x5D, (uint8_t[]) {0x30}, 1, 0},   {0x5E, (uint8_t[]) {0x01}, 1, 0},
    {0x5F, (uint8_t[]) {0x02}, 1, 0},   {0x60, (uint8_t[]) {0x30}, 1, 0},
    {0x61, (uint8_t[]) {0x03}, 1, 0},   {0x62, (uint8_t[]) {0x04}, 1, 0},
    {0x63, (uint8_t[]) {0x04}, 1, 0},   {0x64, (uint8_t[]) {0xA6}, 1, 0},
    {0x65, (uint8_t[]) {0x43}, 1, 0},   {0x66, (uint8_t[]) {0x30}, 1, 0},
    {0x67, (uint8_t[]) {0x73}, 1, 0},   {0x68, (uint8_t[]) {0x05}, 1, 0},
    {0x69, (uint8_t[]) {0x04}, 1, 0},   {0x6A, (uint8_t[]) {0x7F}, 1, 0},
    {0x6B, (uint8_t[]) {0x08}, 1, 0},   {0x6C, (uint8_t[]) {0x00}, 1, 0},
    {0x6D, (uint8_t[]) {0x04}, 1, 0},   {0x6E, (uint8_t[]) {0x04}, 1, 0},
    {0x6F, (uint8_t[]) {0x88}, 1, 0},

    {0x75, (uint8_t[]) {0xD9}, 1, 0},   {0x76, (uint8_t[]) {0x00}, 1, 0},
    {0x77, (uint8_t[]) {0x33}, 1, 0},   {0x78, (uint8_t[]) {0x43}, 1, 0},

    {0xE0, (uint8_t[]) {0x00}, 1, 0},

    {0x11, (uint8_t[]) {0x00}, 1, 120},

    {0x29, (uint8_t[]) {0x00}, 1, 20},  {0x35, (uint8_t[]) {0x00}, 1, 0},
};


std::unique_ptr<DisplayJd9365>
CreateDisplay()
{
    static esp_ldo_channel_handle_t ldo_mipi_phy = nullptr;
    static esp_lcd_dsi_bus_handle_t mipi_dsi_bus = nullptr;
    static esp_lcd_panel_io_handle_t mipi_dbi_io = nullptr;

    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = TEST_MIPI_DSI_PHY_PWR_LDO_CHAN,
        .voltage_mv = TEST_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
        .flags = {},
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));

    static const esp_lcd_dsi_bus_config_t bus_config = JD9365_PANEL_BUS_DSI_2CH_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus));

    static const esp_lcd_dbi_io_config_t dbi_config = JD9365_PANEL_IO_DBI_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io));
    static const esp_lcd_dpi_panel_config_t dpi_config = {
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = 80,
        .pixel_format = TEST_MIPI_DPI_PX_FORMAT,
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB565,
        .num_fbs = 2,
        .video_timing =
            {
                .h_size = 800,
                .v_size = 800,
                .hsync_pulse_width = 20,
                .hsync_back_porch = 20,
                .hsync_front_porch = 40,
                .vsync_pulse_width = 4,
                .vsync_back_porch = 12,
                .vsync_front_porch = 24,
            },
        .flags =
            {
                .use_dma2d = true,
                .disable_lp = false,
            },
    };
    static jd9365_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .mipi_config =
            {
                .dsi_bus = mipi_dsi_bus,
                .dpi_config = &dpi_config,
                .lane_num = TEST_MIPI_DSI_LANE_NUM,
            },
    };
    static const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = TEST_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = TEST_LCD_BIT_PER_PIXEL,
        .flags =
            {
                .reset_active_high = 0,
            },
        .vendor_config = &vendor_config,
    };

    auto out = std::make_unique<DisplayJd9365>(mipi_dbi_io, panel_config);

    gpio_config_t io_conf = {};

    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1 << kTftBacklight;
    gpio_config(&io_conf);

    // Turn on the backlight
    gpio_set_level(kTftBacklight, 0);

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

    auto in_psram = std::make_unique<uint8_t[]>(esp_partition_get(map_partition)->size);
    memcpy(in_psram.get(), p, esp_partition_get(map_partition)->size);

    esp_partition_iterator_release(map_partition);
    srand(esp_random());

    auto map_metadata = reinterpret_cast<const MapMetadata*>(in_psram.get());

    auto target_nvm = std::make_unique<NvmTarget>();

    ApplicationState state;

    state.Checkout()->demo_mode = true;
    state.Checkout()->show_speedometer = true;

    auto io_board_uart = std::make_unique<TargetUart>(UART_NUM_0,
                                                      115200,
                                                      GPIO_NUM_44,  // RX
                                                      GPIO_NUM_43); // TX

    auto uart_event_listener = std::make_unique<UartEventListener>(*io_board_uart);
    auto display = CreateDisplay();
    auto ota_updater_device = std::make_unique<TargetHttpdOtaUpdater>(*display);

    // Threads
    auto route_service = std::make_unique<RouteService>(*map_metadata);
    auto storage = std::make_unique<Storage>(*target_nvm, state, route_service->AttachListener());
    auto producer = std::make_unique<TileProducer>(state, *map_metadata);
    auto gps_simulator = std::make_unique<GpsSimulator>(*map_metadata, state, *route_service);

    // Selects between the real and demo GPS
    auto gps_mux = std::make_unique<GpsMux>(state, *uart_event_listener, *gps_simulator);

    auto gps_reader = std::make_unique<GpsReader>(*map_metadata, state, *gps_mux);
    auto trip_computer = std::make_unique<TripComputer>(state,
                                                        gps_reader->AttachListener(),
                                                        route_service->AttachListener(),
                                                        LookupMetersPerPixel(*map_metadata),
                                                        map_metadata->land_mask_row_size);

    auto ota_updater = std::make_unique<OtaUpdater>(*ota_updater_device, state);
    auto ui = std::make_unique<UserInterface>(state,
                                              *map_metadata,
                                              *producer,
                                              *display,
                                              *uart_event_listener,
                                              *route_service,
                                              *ota_updater,
                                              gps_reader->AttachListener(),
                                              route_service->AttachListener());

    storage->Start("storage");
    uart_event_listener->Start("uart_event_listener");
    gps_simulator->Start("gps_simulator", 4096);
    gps_reader->Start("gps_reader");
    producer->Start("tile_producer", os::ThreadPriority::kHigh);
    route_service->Start("route_service", 4096);
    trip_computer->Start("trip_computer");

    // Time for the storage to read the home position
    os::Sleep(10ms);
    ui->Start("ui", os::ThreadCore::kCore1, os::ThreadPriority::kHigh, 8192);

    while (true)
    {
        vTaskSuspend(nullptr);
    }
}
