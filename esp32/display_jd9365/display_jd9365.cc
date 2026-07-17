#include "display_jd9365.hh"

#include <esp_lcd_panel_ops.h>

DisplayJd9365::DisplayJd9365(esp_lcd_panel_io_handle_t io_handle,
                             const esp_lcd_panel_dev_config_t& panel_config)
{
    ESP_ERROR_CHECK(esp_lcd_new_panel_jd9365(io_handle, &panel_config, &m_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(m_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(m_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(m_panel_handle, true));


    ESP_ERROR_CHECK(esp_lcd_dpi_panel_get_frame_buffer(
        m_panel_handle, 2, (void**)&m_frame_buffers[0], (void**)&m_frame_buffers[1]));

    esp_lcd_dpi_panel_event_callbacks_t callbacks = {};
    callbacks.on_refresh_done =
        [](esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t* edata, void* user_ctx) {
            auto p = static_cast<DisplayJd9365*>(user_ctx);
            p->OnVsync();
            return false;
        };

    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(m_panel_handle, &callbacks, this));
}


uint16_t*
DisplayJd9365::GetFrameBuffer(hal::IDisplay::Owner owner)
{
    return m_frame_buffers[static_cast<int>(owner)];
}

void
DisplayJd9365::Flip()
{
    // If the last esp_lcd_panel_draw_bitmap arg is a frame buffer allocated in PSRAM,
    // then esp_lcd_panel_draw_bitmap does not make a copy but switches to this frame buffer
    esp_lcd_panel_draw_bitmap(m_panel_handle,
                              0,
                              0,
                              hal::kDisplayWidth,
                              hal::kDisplayHeight,
                              m_frame_buffers[m_current_update_frame]);
    m_current_update_frame = !m_current_update_frame;
    m_flip_requested = true;
    m_vsync_done.acquire();
}

void
DisplayJd9365::SetActive(bool active)
{
}

void
DisplayJd9365::OnVsync()
{
    if (m_flip_requested)
    {
        m_flip_requested = false;
        m_vsync_done.release_from_isr();
    }
}
