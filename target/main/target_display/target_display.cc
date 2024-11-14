#include "target_display.hh"

extern "C" {
#include "MyWire.h"
#include "PCA9554.h"
}

#include "init_operations.hpp"

#include <cstring>
#include <esp_attr.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_lcd_panel_rgb.h>
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

constexpr auto init_operations =
    hal::kDisplayWidth == 480 ? TL028WVC01_init_operations : hd40015c40_init_operations;
constexpr auto init_operations_size = hal::kDisplayWidth == 480
                                          ? sizeof(TL028WVC01_init_operations)
                                          : sizeof(hd40015c40_init_operations);

bool
OnVsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* edata, void* user_ctx);

DisplayTarget::DisplayTarget()
{
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

    m_frame_buffers[0] = static_cast<uint16_t*>(
        heap_caps_malloc(hal::kDisplayWidth * hal::kDisplayHeight * 2, MALLOC_CAP_SPIRAM));
    m_frame_buffers[1] = static_cast<uint16_t*>(
        heap_caps_malloc(hal::kDisplayWidth * hal::kDisplayHeight * 2, MALLOC_CAP_SPIRAM));

    assert(m_frame_buffers[0]);
    assert(m_frame_buffers[1]);

    memset(m_frame_buffers[0], 0xff, hal::kDisplayWidth * hal::kDisplayHeight * 2);
    memset(m_frame_buffers[1], 0xff, hal::kDisplayWidth * hal::kDisplayHeight * 2);

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &m_panel_handle));

    esp_lcd_rgb_panel_event_callbacks_t callbacks = {};
    callbacks.on_bounce_empty = DisplayTarget::OnBounceBufferFillStatic;
    callbacks.on_vsync = OnVsync;
    callbacks.on_bounce_frame_finish = DisplayTarget::OnBounceBufferFinishStatic;

    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(m_panel_handle, &callbacks, this));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(m_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(m_panel_handle));

    my_PCA9554_pin_mode(&handle, PCA_TFT_BACKLIGHT, OUTPUT);
    my_PCA9554_digital_write(&handle, PCA_TFT_BACKLIGHT, HIGH);
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
    printf("FLIP! %d, %d. Vsync %d\n", fill_cnt.load(), finish_cnt.load(), vsync_cnt.load());
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
