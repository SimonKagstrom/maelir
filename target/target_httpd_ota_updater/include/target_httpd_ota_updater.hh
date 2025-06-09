#pragma once

#include "hal/i_display.hh"
#include "hal/i_ota_updater.hh"

#include <esp_http_server.h>
#include <memory>

class TargetHttpdOtaUpdater : public hal::IOtaUpdater
{
public:
    TargetHttpdOtaUpdater(hal::IDisplay& display);

    void Update(std::function<void(uint8_t)> progress) final;

    bool ApplicationHasBeenUpdated() const final;

    void MarkApplicationAsValid() final;

private:
    esp_err_t IndexGetHandler(httpd_req_t* req);
    esp_err_t UpdatePostHandler(httpd_req_t* req);

    esp_err_t HttpServerInit();
    esp_err_t SoftApInit();

    hal::IDisplay& m_display;
    std::unique_ptr<char[]> m_receive_buf;
    httpd_handle_t m_http_server {nullptr};

    std::function<void(uint8_t)> m_progress;
};

// TMP
void wifi_task(void* Param);