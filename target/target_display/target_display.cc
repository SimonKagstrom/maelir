#include "target_display.hh"

#include <cstring>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <esp_attr.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_io_additions.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_st7701.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hal/gpio_types.h>
#include <utility>

constexpr auto kTag = "target_display";

bool
OnVsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* edata, void* user_ctx);


// Define the kSt7701InitOperations list (from Arduino, mapped to st7701_lcd_init_cmd_t)
const st7701_lcd_init_cmd_t kSt7701InitOperations[] = {
    {0x01, NULL, 0, 60}, // Reset
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
    {0x3A, (uint8_t[]) {0x60}, 1, 0}, // COLMOD
                                      //    {0x36, (uint8_t[]) {0x00}, 1, 0},
    {0x11, NULL, 0, 10},
    {0x20, NULL, 0, 0},
    {0x29, NULL, 0, 0},
};

DisplayTarget::DisplayTarget(const esp_lcd_panel_io_3wire_spi_config_t& io_config,
                             const esp_lcd_rgb_panel_config_t& rgb_config)
{
    m_frame_buffers[0] = static_cast<uint16_t*>(
        heap_caps_aligned_calloc(64,
                                 1,
                                 hal::kDisplayWidth * hal::kDisplayHeight * 2,
                                 MALLOC_CAP_SPIRAM | MALLOC_CAP_CACHE_ALIGNED));
    m_frame_buffers[1] = static_cast<uint16_t*>(
        heap_caps_aligned_calloc(64,
                                 1,
                                 hal::kDisplayWidth * hal::kDisplayHeight * 2,
                                 MALLOC_CAP_SPIRAM | MALLOC_CAP_CACHE_ALIGNED));

    assert(m_frame_buffers[0]);
    assert(m_frame_buffers[1]);

    constexpr async_memcpy_config_t async_mem_cfg = {
        .backlog = 16, .sram_trans_align = 64, .psram_trans_align = 64, .flags = 0};
    esp_async_memcpy_install(&async_mem_cfg, &m_async_mem_handle);

    // Turn on the backlight
    esp_lcd_panel_io_handle_t panel_io = nullptr;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &panel_io));
    ESP_LOGI(kTag, "Install ST7701 panel driver");

    st7701_vendor_config_t vendor_config = {
        .init_cmds = kSt7701InitOperations,
        .init_cmds_size = sizeof(kSt7701InitOperations) / sizeof(st7701_lcd_init_cmd_t),
        .rgb_config = &rgb_config,
        .flags {.enable_io_multiplex = 0},
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 18,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(panel_io, &panel_config, &m_panel_handle));

    esp_lcd_rgb_panel_event_callbacks_t callbacks = {};
    callbacks.on_bounce_empty = DisplayTarget::OnBounceBufferFillStatic;
    callbacks.on_vsync = OnVsync;
    callbacks.on_bounce_frame_finish = DisplayTarget::OnBounceBufferFinishStatic;

    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(m_panel_handle, &callbacks, this));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(m_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(m_panel_handle));
    ESP_LOGI(kTag, "Display init done");
}

uint16_t*
DisplayTarget::GetFrameBuffer(hal::IDisplay::Owner owner)
{
    return m_frame_buffers[std::to_underlying(owner)];
}

void IRAM_ATTR
DisplayTarget::Flip()
{
    m_flip_requested = true;
    m_bounce_copy_end.acquire();
}


bool IRAM_ATTR
OnVsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* edata, void* user_ctx)
{
    return false;
}

void IRAM_ATTR
DisplayTarget::OnBounceBufferFill(void* bounce_buf, int pos_px, int len_bytes)
{
    // Fill bounce buffer with the frame buffer data
    esp_async_memcpy(m_async_mem_handle,
                     bounce_buf,
                     reinterpret_cast<void*>(m_frame_buffers[!m_current_update_frame] + pos_px),
                     len_bytes,
                     nullptr,
                     nullptr);
}

void IRAM_ATTR
DisplayTarget::OnBounceBufferFinish()
{
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
