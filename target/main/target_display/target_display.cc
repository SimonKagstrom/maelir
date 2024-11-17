#include "target_display.hh"

extern "C" {
#include "MyWire.h"
#include "PCA9554.h"
}

#include "init_operations.hpp"

#include <cstring>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <esp_attr.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_io_expander_tca9554.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_io_additions.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_st7701.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hal/gpio_types.h>

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

constexpr auto kTag = "target_display";

constexpr auto init_operations =
    hal::kDisplayWidth == 480 ? TL028WVC01_init_operations : hd40015c40_init_operations;
constexpr auto init_operations_size = hal::kDisplayWidth == 480
                                          ? sizeof(TL028WVC01_init_operations)
                                          : sizeof(hd40015c40_init_operations);

bool
OnVsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* edata, void* user_ctx);


// Define the TL028WVC01_init_operations list
const st7701_lcd_init_cmd_t xTL028WVC01_init_operations[] = {
    {0x01, NULL, 0, 120}, // Reset
    {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (uint8_t[]) {0x08}, 1, 0},
    {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t[]) {0x3B, 0x00}, 2, 0},
    {0xC1, (uint8_t[]) {0x10, 0x0C}, 2, 0},
    {0xC2, (uint8_t[]) {0x07, 0x0A}, 2, 0},
    {0xC7, (uint8_t[]) {0x00}, 1, 0},
    {0xCC, (uint8_t[]) {0x10}, 1, 0},
    {0xCD, (uint8_t[]) {0x08}, 1, 0},
    {0xB0,
     (uint8_t[]) {0x05,
                  0x12,
                  0x98,
                  0x0E,
                  0x0F,
                  0x07,
                  0x07,
                  0x09,
                  0x09,
                  0x23,
                  0x05,
                  0x52,
                  0x0F,
                  0x67,
                  0x2C,
                  0x11},
     16,
     0},
    {0xB1,
     (uint8_t[]) {0x0B,
                  0x11,
                  0x97,
                  0x0C,
                  0x12,
                  0x06,
                  0x06,
                  0x08,
                  0x08,
                  0x22,
                  0x03,
                  0x51,
                  0x11,
                  0x66,
                  0x2B,
                  0x0F},
     16,
     0},
    {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t[]) {0x5D}, 1, 0},
    {0xB1, (uint8_t[]) {0x2D}, 1, 0},
    {0xB2, (uint8_t[]) {0x81}, 1, 0},
    {0xB3, (uint8_t[]) {0x80}, 1, 0},
    {0xB5, (uint8_t[]) {0x4E}, 1, 0},
    {0xB7, (uint8_t[]) {0x85}, 1, 0},
    {0xB8, (uint8_t[]) {0x20}, 1, 0},
    {0xC1, (uint8_t[]) {0x78}, 1, 0},
    {0xC2, (uint8_t[]) {0x78}, 1, 0},
    {0xD0, (uint8_t[]) {0x88}, 1, 0},
    {0xE0, (uint8_t[]) {0x00, 0x00, 0x02}, 3, 0},
    {0xE1, (uint8_t[]) {0x06, 0x30, 0x08, 0x30, 0x05, 0x30, 0x07, 0x30, 0x00, 0x33, 0x33}, 11, 0},
    {0xE2,
     (uint8_t[]) {0x11, 0x11, 0x33, 0x33, 0xF4, 0x00, 0x00, 0x00, 0xF4, 0x00, 0x00, 0x00},
     12,
     0},
    {0xE3, (uint8_t[]) {0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE4, (uint8_t[]) {0x44, 0x44}, 2, 0},
    {0xE5,
     (uint8_t[]) {0x0D,
                  0xF5,
                  0x30,
                  0xF0,
                  0x0F,
                  0xF7,
                  0x30,
                  0xF0,
                  0x09,
                  0xF1,
                  0x30,
                  0xF0,
                  0x0B,
                  0xF3,
                  0x30,
                  0xF0},
     16,
     0},
    {0xE6, (uint8_t[]) {0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE7, (uint8_t[]) {0x44, 0x44}, 2, 0},
    {0xE8,
     (uint8_t[]) {0x0C,
                  0xF4,
                  0x30,
                  0xF0,
                  0x0E,
                  0xF6,
                  0x30,
                  0xF0,
                  0x08,
                  0xF0,
                  0x30,
                  0xF0,
                  0x0A,
                  0xF2,
                  0x30,
                  0xF0},
     16,
     0},
    {0xE9, (uint8_t[]) {0x36, 0x01}, 2, 0},
    {0xEB, (uint8_t[]) {0x00, 0x01, 0xE4, 0xE4, 0x44, 0x88, 0x40}, 7, 0},
    {0xED,
     (uint8_t[]) {0xFF,
                  0x10,
                  0xAF,
                  0x76,
                  0x54,
                  0x2B,
                  0xCF,
                  0xFF,
                  0xFF,
                  0xFC,
                  0xB2,
                  0x45,
                  0x67,
                  0xFA,
                  0x01,
                  0xFF},
     16,
     0},
    {0xEF, (uint8_t[]) {0x08, 0x08, 0x08, 0x45, 0x3F, 0x54}, 6, 0},
    {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    //{0x23, NULL, 0, 0},
    {0x11, NULL, 0, 120},
    //    {0x3A, (uint8_t[]){0x66}, 1, 0},
    //    {0x36, (uint8_t[]){0x00}, 1, 0},
    //    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x29, NULL, 0, 50},

    // {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x12}, 5, 0}, /* This part of the parameters can be used for screen self-test */
    //{0xD1, (uint8_t []){0x81}, 1, 0},
    //{0xD2, (uint8_t []){0x08}, 1, 0},


    //    {0x00, NULL, 0, 0} // End of commands
};

DisplayTarget::DisplayTarget()
{
    m_frame_buffers[0] = static_cast<uint16_t*>(
        heap_caps_malloc(hal::kDisplayWidth * hal::kDisplayHeight * 2, MALLOC_CAP_SPIRAM));
    m_frame_buffers[1] = static_cast<uint16_t*>(
        heap_caps_malloc(hal::kDisplayWidth * hal::kDisplayHeight * 2, MALLOC_CAP_SPIRAM));

    assert(m_frame_buffers[0]);
    assert(m_frame_buffers[1]);

    memset(m_frame_buffers[0], 0xff, hal::kDisplayWidth * hal::kDisplayHeight * 2);
    memset(m_frame_buffers[1], 0xff, hal::kDisplayWidth * hal::kDisplayHeight * 2);

    i2c_master_bus_handle_t bus_handle = nullptr;
    esp_io_expander_handle_t expander_handle = nullptr;
    esp_lcd_panel_io_handle_t panel_io = nullptr;


    const i2c_master_bus_config_t i2c_master_config = {.i2c_port = kI2cBus,
                                                       .sda_io_num = kI2cSdaPin,
                                                       .scl_io_num = kI2cSclPin,
                                                       .clk_source = I2C_CLK_SRC_DEFAULT,
                                                       .glitch_ignore_cnt = 7,
                                                       .trans_queue_depth = 0,
                                                       .flags {.enable_internal_pullup = true}};
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master_config, &bus_handle));

    ESP_LOGI(kTag, "Create TCA9554 IO expander");
    ESP_ERROR_CHECK(
        esp_io_expander_new_i2c_tca9554(bus_handle, kI2cExpanderAddress, &expander_handle));

    esp_io_expander_set_dir(expander_handle, kExpanderTftReset, IO_EXPANDER_OUTPUT);

    esp_io_expander_set_level(expander_handle, kExpanderTftReset, 0);
    os::Sleep(10ms);
    esp_io_expander_set_level(expander_handle, kExpanderTftReset, 1);
    os::Sleep(100ms);

    esp_io_expander_set_dir(expander_handle, kExpanderTftCs, IO_EXPANDER_OUTPUT);
    esp_io_expander_set_dir(expander_handle, kExpanderTftSck, IO_EXPANDER_OUTPUT);
    esp_io_expander_set_dir(expander_handle, kExpanderTftMosi, IO_EXPANDER_OUTPUT);
    esp_io_expander_set_dir(expander_handle, kExpanderTftBacklight, IO_EXPANDER_OUTPUT);

    esp_io_expander_set_level(
        expander_handle, kExpanderTftReset | kExpanderTftMosi | kExpanderTftCs, 1);

    // Turn on the backlight
    ESP_LOGI(kTag, "Install 3-wire SPI panel IO");
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
        .expect_clk_speed = 100 * 1000U,
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
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &panel_io));
    ESP_LOGI(kTag, "Install ST7701 panel driver");


    esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
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
        .bounce_buffer_size_px = hal::kDisplayWidth * 8,
        .psram_trans_align = 64,
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
            .no_fb = 1,
            .bb_invalidate_cache = 0,
        },
    };
    st7701_vendor_config_t vendor_config = {
        .init_cmds = xTL028WVC01_init_operations,
        .init_cmds_size = sizeof(xTL028WVC01_init_operations) / sizeof(st7701_lcd_init_cmd_t),
        .rgb_config = &rgb_config,
        .flags {.enable_io_multiplex = 0},
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(panel_io, &panel_config, &m_panel_handle));

#if 0
    my_wire_create();

    MY_PCA9554_HANDLE handle;
    my_PCA9554_create(&handle);

    my_PCA9554_send_command(&handle, 0x01);
    vTaskDelay(100);

    my_PCA9554_batch(&handle, init_operations, init_operations_size);

    esp_lcd_rgb_panel_config_t panel_config = {};

    panel_config.data_width = 16; // RGB565 in parallel mode, thus 16bit in width
    panel_config.psram_trans_align = 64;
    panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
    panel_config.disp_gpio_num = GPIO_NUM_NC;
    panel_config.pclk_gpio_num = TFT_PCLK;
    panel_config.vsync_gpio_num = TFT_VSYNC;
    panel_config.hsync_gpio_num = TFT_HSYNC;
    panel_config.de_gpio_num = TFT_DE;
    panel_config.data_gpio_nums[0] = TFT_B1;
    panel_config.data_gpio_nums[1] = TFT_B2;
    panel_config.data_gpio_nums[2] = TFT_B3;
    panel_config.data_gpio_nums[3] = TFT_B4;
    panel_config.data_gpio_nums[4] = TFT_B5;
    panel_config.data_gpio_nums[5] = TFT_G0;
    panel_config.data_gpio_nums[6] = TFT_G1;
    panel_config.data_gpio_nums[7] = TFT_G2;
    panel_config.data_gpio_nums[8] = TFT_G3;
    panel_config.data_gpio_nums[9] = TFT_G4;
    panel_config.data_gpio_nums[10] = TFT_G5;
    panel_config.data_gpio_nums[11] = TFT_R1;
    panel_config.data_gpio_nums[12] = TFT_R2;
    panel_config.data_gpio_nums[13] = TFT_R3;
    panel_config.data_gpio_nums[14] = TFT_R4;
    panel_config.data_gpio_nums[15] = TFT_R5;

    /*
     * Ref https://github.com/espressif/esp-idf/[...]/i80_controller_example_main.c
     *
     *   "PCLK frequency can't go too high as the limitation of PSRAM bandwidth"
     * 
     * However, with the fill callback it can go up to high frequencies. For some
     * reason, this also affect backlight flickering.
     */
    panel_config.timings.pclk_hz = 17 * 1000 * 1000;
    panel_config.timings.h_res = hal::kDisplayWidth;
    panel_config.timings.v_res = hal::kDisplayHeight;
    panel_config.timings.hsync_back_porch = 44;
    panel_config.timings.hsync_front_porch = 50;
    panel_config.timings.hsync_pulse_width = 2;
    panel_config.timings.vsync_back_porch = 18;
    panel_config.timings.vsync_front_porch = 16;
    panel_config.timings.vsync_pulse_width = 2;
    panel_config.timings.flags.pclk_active_neg = 0;

    panel_config.timings.flags.hsync_idle_low = 1;
    panel_config.timings.flags.vsync_idle_low = 1;

    // Bounce buffer
    panel_config.bounce_buffer_size_px = hal::kDisplayWidth * 8;
    panel_config.flags.no_fb = 1;
    panel_config.flags.bb_invalidate_cache = 0;
#endif


    //    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &m_panel_handle));

    esp_lcd_rgb_panel_event_callbacks_t callbacks = {};
    callbacks.on_bounce_empty = DisplayTarget::OnBounceBufferFillStatic;
    callbacks.on_vsync = OnVsync;
    callbacks.on_bounce_frame_finish = DisplayTarget::OnBounceBufferFinishStatic;

    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(m_panel_handle, &callbacks, this));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(m_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(m_panel_handle));


    // Turn on the backlight
    esp_io_expander_set_level(expander_handle, kExpanderTftBacklight, 1);


    //    my_PCA9554_pin_mode(&handle, PCA_TFT_BACKLIGHT, OUTPUT);
    //    my_PCA9554_digital_write(&handle, PCA_TFT_BACKLIGHT, HIGH);
}

uint16_t*
DisplayTarget::GetFrameBuffer()
{
    return m_frame_buffers[m_current_update_frame];
}

static std::atomic<int> fill_cnt = 0;
static std::atomic<int> finish_cnt = 0;
static std::atomic<int> vsync_cnt = 0;
void IRAM_ATTR
DisplayTarget::Flip()
{
    m_flip_requested = true;
    m_bounce_copy_end.acquire();
}


bool IRAM_ATTR
OnVsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* edata, void* user_ctx)
{
    vsync_cnt++;
    return false;
}

void IRAM_ATTR
DisplayTarget::OnBounceBufferFill(void* bounce_buf, int pos_px, int len_bytes)
{
    fill_cnt++;
    // Fill bounce buffer with the frame buffer data
    memcpy(bounce_buf,
           reinterpret_cast<const uint8_t*>(m_frame_buffers[!m_current_update_frame] + pos_px),
           len_bytes);
}

void IRAM_ATTR
DisplayTarget::OnBounceBufferFinish()
{
    finish_cnt++;
    if (m_flip_requested)
    {
        m_flip_requested = false;
        m_current_update_frame = !m_current_update_frame;
        m_bounce_copy_end.release_from_isr();
    }
}


bool IRAM_ATTR
DisplayTarget::OnBounceBufferFillStatic(
    esp_lcd_panel_handle_t panel, void* bounce_buf, int pos_px, int len_bytes, void* user_ctx)
{
    auto p = static_cast<DisplayTarget*>(user_ctx);

    p->OnBounceBufferFill(bounce_buf, pos_px, len_bytes);

    return false;
}

bool IRAM_ATTR
DisplayTarget::OnBounceBufferFinishStatic(esp_lcd_panel_handle_t panel,
                                          const esp_lcd_rgb_panel_event_data_t* edata,
                                          void* user_ctx)
{
    auto p = static_cast<DisplayTarget*>(user_ctx);

    p->OnBounceBufferFinish();

    return false;
}
