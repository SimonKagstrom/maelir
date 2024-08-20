#include "gps_reader.hh"
#include "sdkconfig.h"
#include "target_display.hh"
#include "tile_producer.hh"
#include "tile_utils.hh"
#include "ui.hh"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class DummyGps : public hal::IGps, public os::BaseThread
{
public:
    DummyGps()
        : os::BaseThread(0)
    {
        m_current_position = {kCornerLatitude, kCornerLongitude};
    }

    std::optional<milliseconds> OnActivation() final
    {
        m_current_position.latitude += 0.00001;
        m_current_position.longitude += 0.00001;

        m_has_data_semaphore.release();
        return 1ms;
    }

    GpsData WaitForData(os::binary_semaphore& semaphore) final
    {
        m_has_data_semaphore.acquire();
        semaphore.release();

        auto [x, y] = PositionToPoint(m_current_position);

        printf("POS now %f, %f -> pixels %d, %d\n",
               m_current_position.latitude,
               m_current_position.longitude,
               x,
               y);

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

    gps->Start();
    gps_reader->Start();
    producer->Start();
    ui->Start();

    while (true)
    {
        vTaskSuspend(nullptr);
    }
}
