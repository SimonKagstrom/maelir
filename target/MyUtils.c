
#include "MyUtils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"


void my_delay(uint32_t ms) {
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

unsigned long my_millis(void) {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}