#include "target_display.hh"

extern "C" {
#include "MyWire.h"
#include "PCA9554.h"
}

#include <cstring>
#include <esp_err.h>
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

constexpr auto kWidth = 720;
constexpr auto kHeight = 720;


constexpr uint8_t hd40015c40_init_operations[] = {BEGIN_WRITE,
                                                  WRITE_C8_D8,
                                                  0xFF,
                                                  0x30,
                                                  WRITE_C8_D8,
                                                  0xFF,
                                                  0x52,
                                                  WRITE_C8_D8,
                                                  0xFF,
                                                  0x01,
                                                  WRITE_C8_D8,
                                                  0xE3,
                                                  0x00,
                                                  WRITE_C8_D8,
                                                  0x0A,
                                                  0x11,
                                                  WRITE_C8_D8,
                                                  0x23,
                                                  0xA0,
                                                  WRITE_C8_D8,
                                                  0x24,
                                                  0x32,
                                                  WRITE_C8_D8,
                                                  0x25,
                                                  0x12,
                                                  WRITE_C8_D8,
                                                  0x26,
                                                  0x2E,
                                                  WRITE_C8_D8,
                                                  0x27,
                                                  0x2E,
                                                  WRITE_C8_D8,
                                                  0x29,
                                                  0x02,
                                                  WRITE_C8_D8,
                                                  0x2A,
                                                  0xCF,
                                                  WRITE_C8_D8,
                                                  0x32,
                                                  0x34,
                                                  WRITE_C8_D8,
                                                  0x38,
                                                  0x9C,
                                                  WRITE_C8_D8,
                                                  0x39,
                                                  0xA7,
                                                  WRITE_C8_D8,
                                                  0x3A,
                                                  0x27,
                                                  WRITE_C8_D8,
                                                  0x3B,
                                                  0x94,
                                                  WRITE_C8_D8,
                                                  0x42,
                                                  0x6D,
                                                  WRITE_C8_D8,
                                                  0x43,
                                                  0x83,
                                                  WRITE_C8_D8,
                                                  0x81,
                                                  0x00,
                                                  WRITE_C8_D8,
                                                  0x91,
                                                  0x67,
                                                  WRITE_C8_D8,
                                                  0x92,
                                                  0x67,
                                                  WRITE_C8_D8,
                                                  0xA0,
                                                  0x52,
                                                  WRITE_C8_D8,
                                                  0xA1,
                                                  0x50,
                                                  WRITE_C8_D8,
                                                  0xA4,
                                                  0x9C,
                                                  WRITE_C8_D8,
                                                  0xA7,
                                                  0x02,
                                                  WRITE_C8_D8,
                                                  0xA8,
                                                  0x02,
                                                  WRITE_C8_D8,
                                                  0xA9,
                                                  0x02,
                                                  WRITE_C8_D8,
                                                  0xAA,
                                                  0xA8,
                                                  WRITE_C8_D8,
                                                  0xAB,
                                                  0x28,
                                                  WRITE_C8_D8,
                                                  0xAE,
                                                  0xD2,
                                                  WRITE_C8_D8,
                                                  0xAF,
                                                  0x02,
                                                  WRITE_C8_D8,
                                                  0xB0,
                                                  0xD2,
                                                  WRITE_C8_D8,
                                                  0xB2,
                                                  0x26,
                                                  WRITE_C8_D8,
                                                  0xB3,
                                                  0x26,

                                                  WRITE_C8_D8,
                                                  0xFF,
                                                  0x30,
                                                  WRITE_C8_D8,
                                                  0xFF,
                                                  0x52,
                                                  WRITE_C8_D8,
                                                  0xFF,
                                                  0x02,
                                                  WRITE_C8_D8,
                                                  0xB1,
                                                  0x0A,
                                                  WRITE_C8_D8,
                                                  0xD1,
                                                  0x0E,
                                                  WRITE_C8_D8,
                                                  0xB4,
                                                  0x2F,
                                                  WRITE_C8_D8,
                                                  0xD4,
                                                  0x2D,
                                                  WRITE_C8_D8,
                                                  0xB2,
                                                  0x0C,
                                                  WRITE_C8_D8,
                                                  0xD2,
                                                  0x0C,
                                                  WRITE_C8_D8,
                                                  0xB3,
                                                  0x30,
                                                  WRITE_C8_D8,
                                                  0xD3,
                                                  0x2A,
                                                  WRITE_C8_D8,
                                                  0xB6,
                                                  0x1E,
                                                  WRITE_C8_D8,
                                                  0xD6,
                                                  0x16,
                                                  WRITE_C8_D8,
                                                  0xB7,
                                                  0x3B,
                                                  WRITE_C8_D8,
                                                  0xD7,
                                                  0x35,
                                                  WRITE_C8_D8,
                                                  0xC1,
                                                  0x08,
                                                  WRITE_C8_D8,
                                                  0xE1,
                                                  0x08,
                                                  WRITE_C8_D8,
                                                  0xB8,
                                                  0x0D,
                                                  WRITE_C8_D8,
                                                  0xD8,
                                                  0x0D,
                                                  WRITE_C8_D8,
                                                  0xB9,
                                                  0x05,
                                                  WRITE_C8_D8,
                                                  0xD9,
                                                  0x05,
                                                  WRITE_C8_D8,
                                                  0xBD,
                                                  0x15,
                                                  WRITE_C8_D8,
                                                  0xDD,
                                                  0x15,
                                                  WRITE_C8_D8,
                                                  0xBC,
                                                  0x13,
                                                  WRITE_C8_D8,
                                                  0xDC,
                                                  0x13,
                                                  WRITE_C8_D8,
                                                  0xBB,
                                                  0x12,
                                                  WRITE_C8_D8,
                                                  0xDB,
                                                  0x10,
                                                  WRITE_C8_D8,
                                                  0xBA,
                                                  0x11,
                                                  WRITE_C8_D8,
                                                  0xDA,
                                                  0x11,
                                                  WRITE_C8_D8,
                                                  0xBE,
                                                  0x17,
                                                  WRITE_C8_D8,
                                                  0xDE,
                                                  0x17,
                                                  WRITE_C8_D8,
                                                  0xBF,
                                                  0x0F,
                                                  WRITE_C8_D8,
                                                  0xDF,
                                                  0x0F,
                                                  WRITE_C8_D8,
                                                  0xC0,
                                                  0x16,
                                                  WRITE_C8_D8,
                                                  0xE0,
                                                  0x16,
                                                  WRITE_C8_D8,
                                                  0xB5,
                                                  0x2E,
                                                  WRITE_C8_D8,
                                                  0xD5,
                                                  0x3F,
                                                  WRITE_C8_D8,
                                                  0xB0,
                                                  0x03,
                                                  WRITE_C8_D8,
                                                  0xD0,
                                                  0x02,

                                                  WRITE_C8_D8,
                                                  0xFF,
                                                  0x30,
                                                  WRITE_C8_D8,
                                                  0xFF,
                                                  0x52,
                                                  WRITE_C8_D8,
                                                  0xFF,
                                                  0x03,
                                                  WRITE_C8_D8,
                                                  0x08,
                                                  0x09,
                                                  WRITE_C8_D8,
                                                  0x09,
                                                  0x0A,
                                                  WRITE_C8_D8,
                                                  0x0A,
                                                  0x0B,
                                                  WRITE_C8_D8,
                                                  0x0B,
                                                  0x0C,
                                                  WRITE_C8_D8,
                                                  0x28,
                                                  0x22,
                                                  WRITE_C8_D8,
                                                  0x2A,
                                                  0xE9,
                                                  WRITE_C8_D8,
                                                  0x2B,
                                                  0xE9,
                                                  WRITE_C8_D8,
                                                  0x34,
                                                  0x51,
                                                  WRITE_C8_D8,
                                                  0x35,
                                                  0x01,
                                                  WRITE_C8_D8,
                                                  0x36,
                                                  0x26,
                                                  WRITE_C8_D8,
                                                  0x37,
                                                  0x13,
                                                  WRITE_C8_D8,
                                                  0x40,
                                                  0x07,
                                                  WRITE_C8_D8,
                                                  0x41,
                                                  0x08,
                                                  WRITE_C8_D8,
                                                  0x42,
                                                  0x09,
                                                  WRITE_C8_D8,
                                                  0x43,
                                                  0x0A,
                                                  WRITE_C8_D8,
                                                  0x44,
                                                  0x22,
                                                  WRITE_C8_D8,
                                                  0x45,
                                                  0xDB,
                                                  WRITE_C8_D8,
                                                  0x46,
                                                  0xdC,
                                                  WRITE_C8_D8,
                                                  0x47,
                                                  0x22,
                                                  WRITE_C8_D8,
                                                  0x48,
                                                  0xDD,
                                                  WRITE_C8_D8,
                                                  0x49,
                                                  0xDE,
                                                  WRITE_C8_D8,
                                                  0x50,
                                                  0x0B,
                                                  WRITE_C8_D8,
                                                  0x51,
                                                  0x0C,
                                                  WRITE_C8_D8,
                                                  0x52,
                                                  0x0D,
                                                  WRITE_C8_D8,
                                                  0x53,
                                                  0x0E,
                                                  WRITE_C8_D8,
                                                  0x54,
                                                  0x22,
                                                  WRITE_C8_D8,
                                                  0x55,
                                                  0xDF,
                                                  WRITE_C8_D8,
                                                  0x56,
                                                  0xE0,
                                                  WRITE_C8_D8,
                                                  0x57,
                                                  0x22,
                                                  WRITE_C8_D8,
                                                  0x58,
                                                  0xE1,
                                                  WRITE_C8_D8,
                                                  0x59,
                                                  0xE2,
                                                  WRITE_C8_D8,
                                                  0x80,
                                                  0x1E,
                                                  WRITE_C8_D8,
                                                  0x81,
                                                  0x1E,
                                                  WRITE_C8_D8,
                                                  0x82,
                                                  0x1F,
                                                  WRITE_C8_D8,
                                                  0x83,
                                                  0x1F,
                                                  WRITE_C8_D8,
                                                  0x84,
                                                  0x05,
                                                  WRITE_C8_D8,
                                                  0x85,
                                                  0x0A,
                                                  WRITE_C8_D8,
                                                  0x86,
                                                  0x0A,
                                                  WRITE_C8_D8,
                                                  0x87,
                                                  0x0C,
                                                  WRITE_C8_D8,
                                                  0x88,
                                                  0x0C,
                                                  WRITE_C8_D8,
                                                  0x89,
                                                  0x0E,
                                                  WRITE_C8_D8,
                                                  0x8A,
                                                  0x0E,
                                                  WRITE_C8_D8,
                                                  0x8B,
                                                  0x10,
                                                  WRITE_C8_D8,
                                                  0x8C,
                                                  0x10,
                                                  WRITE_C8_D8,
                                                  0x8D,
                                                  0x00,
                                                  WRITE_C8_D8,
                                                  0x8E,
                                                  0x00,
                                                  WRITE_C8_D8,
                                                  0x8F,
                                                  0x1F,
                                                  WRITE_C8_D8,
                                                  0x90,
                                                  0x1F,
                                                  WRITE_C8_D8,
                                                  0x91,
                                                  0x1E,
                                                  WRITE_C8_D8,
                                                  0x92,
                                                  0x1E,
                                                  WRITE_C8_D8,
                                                  0x93,
                                                  0x02,
                                                  WRITE_C8_D8,
                                                  0x94,
                                                  0x04,
                                                  WRITE_C8_D8,
                                                  0x96,
                                                  0x1E,
                                                  WRITE_C8_D8,
                                                  0x97,
                                                  0x1E,
                                                  WRITE_C8_D8,
                                                  0x98,
                                                  0x1F,
                                                  WRITE_C8_D8,
                                                  0x99,
                                                  0x1F,
                                                  WRITE_C8_D8,
                                                  0x9A,
                                                  0x05,
                                                  WRITE_C8_D8,
                                                  0x9B,
                                                  0x09,
                                                  WRITE_C8_D8,
                                                  0x9C,
                                                  0x09,
                                                  WRITE_C8_D8,
                                                  0x9D,
                                                  0x0B,
                                                  WRITE_C8_D8,
                                                  0x9E,
                                                  0x0B,
                                                  WRITE_C8_D8,
                                                  0x9F,
                                                  0x0D,
                                                  WRITE_C8_D8,
                                                  0xA0,
                                                  0x0D,
                                                  WRITE_C8_D8,
                                                  0xA1,
                                                  0x0F,
                                                  WRITE_C8_D8,
                                                  0xA2,
                                                  0x0F,
                                                  WRITE_C8_D8,
                                                  0xA3,
                                                  0x00,
                                                  WRITE_C8_D8,
                                                  0xA4,
                                                  0x00,
                                                  WRITE_C8_D8,
                                                  0xA5,
                                                  0x1F,
                                                  WRITE_C8_D8,
                                                  0xA6,
                                                  0x1F,
                                                  WRITE_C8_D8,
                                                  0xA7,
                                                  0x1E,
                                                  WRITE_C8_D8,
                                                  0xA8,
                                                  0x1E,
                                                  WRITE_C8_D8,
                                                  0xA9,
                                                  0x01,
                                                  WRITE_C8_D8,
                                                  0xAA,
                                                  0x03,

                                                  WRITE_C8_D8,
                                                  0xFF,
                                                  0x30,
                                                  WRITE_C8_D8,
                                                  0xFF,
                                                  0x52,
                                                  WRITE_C8_D8,
                                                  0xFF,
                                                  0x00,
                                                  WRITE_C8_D8,
                                                  0x36,
                                                  0x0A,

                                                  WRITE_COMMAND_8,
                                                  0x11, // Sleep Out
                                                  END_WRITE,

                                                  DELAY,
                                                  100,

                                                  BEGIN_WRITE,
                                                  WRITE_COMMAND_8,
                                                  0x29, // Display On
                                                  END_WRITE,

                                                  DELAY,
                                                  50};


DisplayTarget::DisplayTarget()
{
    my_wire_create();

    MY_PCA9554_HANDLE handle;
    my_PCA9554_create(&handle);

    my_PCA9554_send_command(&handle, 0x01);
    vTaskDelay(1);
    my_PCA9554_batch(&handle, hd40015c40_init_operations, sizeof(hd40015c40_init_operations));

    esp_lcd_rgb_panel_config_t panel_config;

    memset(&panel_config, 0, sizeof(esp_lcd_rgb_panel_config_t));

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
     */
    panel_config.timings.pclk_hz = 5.5 * 1000 * 1000;
    panel_config.timings.h_res = kWidth;
    panel_config.timings.v_res = kHeight;
    panel_config.timings.hsync_back_porch = 44;
    panel_config.timings.hsync_front_porch = 46;
    panel_config.timings.hsync_pulse_width = 2;
    panel_config.timings.vsync_back_porch = 16;
    panel_config.timings.vsync_front_porch = 50;
    panel_config.timings.vsync_pulse_width = 2;
    panel_config.timings.flags.pclk_active_neg = 0;

    panel_config.timings.flags.vsync_idle_low = 1;

    // Request 2 frame buffers in PSRAM
    panel_config.num_fbs = 2;
    panel_config.flags.fb_in_psram = true;

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &m_panel_handle));

    esp_lcd_rgb_panel_event_callbacks_t callbacks;
    memset(&callbacks, 0, sizeof(esp_lcd_rgb_panel_event_callbacks_t));
    callbacks.on_vsync = DisplayTarget::OnVsyncStatic;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(m_panel_handle, &callbacks, this));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(m_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(m_panel_handle));

    // Retrieve allocated frame buffers
    ESP_ERROR_CHECK(
        esp_lcd_rgb_panel_get_frame_buffer(m_panel_handle,
                                           2,
                                           reinterpret_cast<void**>(&m_frame_buffers[0]),
                                           reinterpret_cast<void**>(&m_frame_buffers[1])));

    memset(m_frame_buffers[0], 0, kWidth * kHeight * 2);
    memset(m_frame_buffers[1], 0, kWidth * kHeight * 2);

    my_PCA9554_pin_mode(&handle, PCA_TFT_BACKLIGHT, OUTPUT);
    my_PCA9554_digital_write(&handle, PCA_TFT_BACKLIGHT, HIGH);
}

uint16_t*
DisplayTarget::GetFrameBuffer()
{
    return m_frame_buffers[m_current_update_frame];
}


void
DisplayTarget::Flip()
{
    m_vsync_end.acquire();

    // If the last esp_lcd_panel_draw_bitmap arg is a frame buffer allocated in PSRAM,
    // then esp_lcd_panel_draw_bitmap does not make a copy but switches to this frame buffer
    esp_lcd_panel_draw_bitmap(
        m_panel_handle, 0, 0, kWidth, kHeight, m_frame_buffers[m_current_update_frame]);
    m_current_update_frame = !m_current_update_frame;
    m_vsync_end.acquire();
}

void
DisplayTarget::OnVsync()
{
    m_vsync_end.release();
}

bool
DisplayTarget::OnVsyncStatic(esp_lcd_panel_handle_t panel,
                             const esp_lcd_rgb_panel_event_data_t* data,
                             void* user_ctx)
{
    auto p = static_cast<DisplayTarget*>(user_ctx);

    p->OnVsync();

    return false;
}
