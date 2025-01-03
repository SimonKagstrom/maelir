#pragma once
#include "hal/i_display.hh"
#include "semaphore.hh"

#include <atomic>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>

class DisplayTarget : public hal::IDisplay
{
public:
    DisplayTarget();

    uint16_t* GetFrameBuffer(hal::IDisplay::Owner owner) final;
    void Flip() final;

private:
    enum class FrameBufferOwner
    {
        kDriver,
        kHardware,
    };

    void OnBounceBufferFill(void* bounce_buf, int pos_px, int len_bytes);
    void OnBounceBufferFinish();

    static bool OnBounceBufferFillStatic(
        esp_lcd_panel_handle_t panel, void* bounce_buf, int pos_px, int len_bytes, void* user_ctx);
    static bool OnBounceBufferFinishStatic(esp_lcd_panel_handle_t panel,
                                           const esp_lcd_rgb_panel_event_data_t* edata,
                                           void* user_ctx);

    esp_lcd_panel_handle_t m_panel_handle {nullptr};
    uint16_t* m_frame_buffers[2] {nullptr, nullptr};
    uint8_t m_current_update_frame {0};

    std::atomic<FrameBufferOwner> m_owner {FrameBufferOwner::kDriver};
    std::atomic_bool m_flip_requested {false};
    std::atomic_bool m_vsync_requested {false};
    os::binary_semaphore m_bounce_copy_end {0};
};
