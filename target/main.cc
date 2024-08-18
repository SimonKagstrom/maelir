#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "rom/cache.h"
#include "sdkconfig.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "MyPCA9554.h"
#include "MyUtils.h"
#include "MyWire.h"
}

#include "tile_producer.hh"


#define PCA_TFT_BACKLIGHT 4
#define PCA_BUTTON_DOWN   6
#define PCA_BUTTON_UP     5

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

#define WIDTH  720
#define HEIGHT 720

#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))


class GpsPort : public IGpsPort
{
public:
    void AwakeOn(os::binary_semaphore& semaphore) final
    {
    }

    std::optional<GpsData> Poll() final
    {
        return std::nullopt;
    }
};


void
fill_rect(uint16_t* frame_buffer, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
    uint16_t* p = frame_buffer + WIDTH * y + x;
    uint16_t* first_line = p;

    uint32_t iw = w;

    if (h--)
    {
        while (iw--)
            *p++ = color;
    }

    p = first_line;
    while (h--)
    {
        p += WIDTH;
        memcpy(p, first_line, w << 1);
    }
}

int
clamp(int v, int low, int high)
{
    if (v < low)
    {
        v = low;
    }
    if (v >= high)
    {
        v = high;
    }
    return v;
}


static const uint8_t hd40015c40_init_operations[] = {BEGIN_WRITE,
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

// Semaphore used to synchronize frame buffers swap
SemaphoreHandle_t sem_vsync_end;
SemaphoreHandle_t sem_frame_buffer_ready;

uint16_t* frame_buffers[2];
int current_frame_buffer_idx = 0;


bool
on_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* data, void* user_ctx)
{
    BaseType_t high_task_awoken0 = pdFALSE;
    xSemaphoreGiveFromISR(sem_vsync_end, &high_task_awoken0);
    return (high_task_awoken0 == pdTRUE);
}

void
wait_vsync_and_show_frame_buffer(esp_lcd_panel_handle_t panel, uint16_t* frame_buffer)
{

    xSemaphoreTake(sem_vsync_end, portMAX_DELAY);

    // If the last esp_lcd_panel_draw_bitmap arg is a frame buffer allocated in PSRAM,
    // then esp_lcd_panel_draw_bitmap does not make a copy but switches to this frame buffer
    esp_lcd_panel_draw_bitmap(panel, 0, 0, WIDTH, HEIGHT, frame_buffer);

    xSemaphoreGive(sem_frame_buffer_ready);
}

//Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
//    TFT_DE, TFT_VSYNC, TFT_HSYNC, TFT_PCLK,
//    TFT_R1, TFT_R2, TFT_R3, TFT_R4, TFT_R5,
//    TFT_G0, TFT_G1, TFT_G2, TFT_G3, TFT_G4, TFT_G5,
//    TFT_B1, TFT_B2, TFT_B3, TFT_B4, TFT_B5,
//    1 /* hsync_polarity */, 50 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */,
//    1 /* vsync_polarity */, 16 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 18 /* vsync_back_porch */
////    ,1, 30000000
//    );

void
create_lcd_panel(esp_lcd_panel_handle_t* panel_handle)
{
    esp_lcd_rgb_panel_config_t panel_config;

    memset(&panel_config, 0, sizeof(esp_lcd_rgb_panel_config_t));

    panel_config.data_width = 16; // RGB565 in parallel mode, thus 16bit in width
    panel_config.psram_trans_align = 64;
    panel_config.clk_src = LCD_CLK_SRC_PLL160M;
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

    panel_config.timings.pclk_hz = 16000000;
    panel_config.timings.h_res = WIDTH;
    panel_config.timings.v_res = HEIGHT;
    panel_config.timings.hsync_back_porch = 44;
    panel_config.timings.hsync_front_porch = 46;
    panel_config.timings.hsync_pulse_width = 2;
    panel_config.timings.vsync_back_porch = 16;
    panel_config.timings.vsync_front_porch = 50;
    panel_config.timings.vsync_pulse_width = 16;
    panel_config.timings.flags.pclk_active_neg = 0;

    //    panel_config.timings.pclk_hz = 30000000;
    //    panel_config.timings.hsync_back_porch = 44;
    //    panel_config.timings.hsync_front_porch = 50;
    //    panel_config.timings.hsync_pulse_width = 2;
    //    panel_config.timings.vsync_back_porch = 18;
    //    panel_config.timings.vsync_front_porch = 15;
    //    panel_config.timings.vsync_pulse_width = 2;
    //    panel_config.timings.flags.pclk_active_neg = 0;

    panel_config.timings.flags.vsync_idle_low = 1;

    // Request 2 frame buffers in PSRAM
    panel_config.num_fbs = 2;
    panel_config.flags.fb_in_psram = true;

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, panel_handle));

    esp_lcd_rgb_panel_event_callbacks_t callbacks;
    memset(&callbacks, 0, sizeof(esp_lcd_rgb_panel_event_callbacks_t));
    callbacks.on_vsync = on_vsync;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(*panel_handle, &callbacks, NULL));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*panel_handle));

    // Retrieve allocated frame buffers
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(
        *panel_handle, 2, (void**)&frame_buffers[0], (void**)&frame_buffers[1]));
}

extern "C" void
app_main(void)
{
    printf("Phnom world!\n");

    sem_vsync_end = xSemaphoreCreateBinary();
    assert(sem_vsync_end);
    sem_frame_buffer_ready = xSemaphoreCreateBinary();
    assert(sem_frame_buffer_ready);

    my_wire_create();
    MY_PCA9554_HANDLE handle;
    my_PCA9554_create(&handle);

    my_PCA9554_send_command(&handle, 0x01);
    my_delay(120);
    my_PCA9554_batch(&handle, hd40015c40_init_operations, sizeof(hd40015c40_init_operations));

    esp_lcd_panel_handle_t panel_handle = NULL;
    create_lcd_panel(&panel_handle);

    my_PCA9554_pin_mode(&handle, PCA_TFT_BACKLIGHT, OUTPUT);
    my_PCA9554_digital_write(&handle, PCA_TFT_BACKLIGHT, HIGH);

    auto producer = std::make_unique<TileProducer>(std::make_unique<GpsPort>());
    producer->Start();

    int fps_counter = 0;
    unsigned long fps_time = my_millis();

    uint16_t* frame_buffer;
    frame_buffer = frame_buffers[current_frame_buffer_idx];

    // Clear frame buffer and draw squares
    fill_rect(frame_buffer, 0, 0, WIDTH, HEIGHT, RGB565(0, 255, 0));

    auto tile0 = producer->LockTile(0,0);
    auto tile1 = producer->LockTile(240,0);
    auto tile2 = producer->LockTile(480,0);
    auto tile3 = producer->LockTile(0,240);
    auto i0 = tile0->GetImage();
    auto i1 = tile1->GetImage();
    auto i2 = tile2->GetImage();
    auto i3 = tile3->GetImage();
    //auto image = producer->DecodeTile(0);

    for (auto row = 0; row < 240; row++)
    {
        memcpy(frame_buffer + row * 720, i0.data.data() + row * 240, 240 * 2);
        memcpy(frame_buffer + row * 720 + 240, i1.data.data() + row * 240, 240 * 2);
        memcpy(frame_buffer + row * 720 + 480, i2.data.data() + row * 240, 240 * 2);
        memcpy(frame_buffer + row * 720 + 720, i3.data.data() + row * 240, 240 * 2);
    }

    wait_vsync_and_show_frame_buffer(panel_handle, frame_buffer);
    current_frame_buffer_idx = !current_frame_buffer_idx;

    while (true)
    {

        //        uint16_t* frame_buffer;
        //        frame_buffer = frame_buffers[current_frame_buffer_idx];
        //
        //        // Clear frame buffer and draw squares
        //        fill_rect(frame_buffer, 0, 0, WIDTH, HEIGHT, RGB565(0, 255, 0));
        //        for (int i = 0; i < SQUARE_COUNT; i++)
        //        {
        //            //            vTaskDelay(109);
        //
        //            draw_square(frame_buffer, &squares[i]);
        //        }
        //
        //        wait_vsync_and_show_frame_buffer(panel_handle, frame_buffer);
        //        current_frame_buffer_idx = !current_frame_buffer_idx;
        //
        //        // Update squares positions
        //        for (int i = 0; i < SQUARE_COUNT; i++)
        //        {
        //            update_square(&squares[i]);
        //        }
        //
        //        // Update FPS counter and print FPS
        //        fps_counter++;
        //        unsigned long t = my_millis();
        //        if (t - fps_time >= 1000)
        //        {
        //            printf("FPS: %d\n", fps_counter);
        //            fps_counter = 0;
        //            fps_time = t;
        //        }
        vTaskDelay(1);
    }

    my_PCA9554_delete(&handle);
    my_wire_delete();
}
