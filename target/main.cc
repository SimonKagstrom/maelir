#include "gps_reader.hh"
#include "sdkconfig.h"
#include "target_display.hh"
#include "tile_producer.hh"
#include "ui.hh"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class DummyGps : public hal::IGps, os::BaseThread
{
public:
    std::optional<milliseconds> OnActivation() final
    {
        m_current_position.latitude += 0.00001;
        m_current_position.longitude += 0.00001;

        m_has_data_semaphore.release();
        return 250ms;
    }

    GpsData WaitForData(os::binary_semaphore& semaphore) final
    {
        m_has_data_semaphore.acquire();
        semaphore.release();

        printf("POS now %f, %f\n", m_current_position.latitude, m_current_position.longitude);  
        return m_current_position;
    }

    GpsData m_current_position;
    os::binary_semaphore m_has_data_semaphore {0};
};


extern "C" void
app_main(void)
{
    printf("Phnom world!\n");

    auto display = std::make_unique<DisplayTarget>();
    auto gps = std::make_unique<DummyGps>();

    auto gps_reader = std::make_unique<GpsReader>(*gps);
    auto producer = std::make_unique<TileProducer>(gps_reader->AttachListener());
    auto ui = std::make_unique<UserInterface>(*producer, *display, gps_reader->AttachListener());

    gps_reader->Start();
    producer->Start();
    ui->Start();

    while (true)
    {
        vTaskSuspend(nullptr);
    }
}
