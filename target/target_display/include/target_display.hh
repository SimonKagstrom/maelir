#pragma once
#include "hal/i_display.hh"
#include "semaphore.hh"

#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>


class DisplayTarget : public hal::IDisplay
{
public:
    DisplayTarget();

    void Blit(const Image& image, Rect to, std::optional<Rect> from) final;
    void Flip() final;

private:
    static bool OnVsync(esp_lcd_panel_handle_t panel,
                        const esp_lcd_rgb_panel_event_data_t* data,
                        void* user_ctx);

    esp_lcd_panel_handle_t m_panel_handle {nullptr};
    uint16_t* m_frame_buffers[2] {nullptr, nullptr};
    uint8_t m_current_update_frame {1};

    os::binary_semaphore m_vsync_end {0};
};
