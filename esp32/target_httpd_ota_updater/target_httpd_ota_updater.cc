// From https://github.com/Jeija/esp32-softap-ota
#include "target_httpd_ota_updater.hh"

#include "time.hh"

#include <esp_mac.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <string.h>
#include <sys/param.h>

#define WIFI_SSID "ESP32 OTA Update"

constexpr auto kBufSize = 256;

/*
 * Serve OTA update portal (index.html)
 */
constexpr const char* index_html = R"VOBB(<!DOCTYPE html>
<html>
	<head>
		<meta http-equiv="content-type" content="text/html; charset=utf-8" />
		<title>Maelir OTA Update</title>
		<script>
			function startUpload() {
				var otafile = document.getElementById("otafile").files;

				if (otafile.length == 0) {
					alert("No file selected!");
				} else {
					document.getElementById("otafile").disabled = true;
					document.getElementById("upload").disabled = true;

					var file = otafile[0];
					var xhr = new XMLHttpRequest();
					xhr.onreadystatechange = function() {
						if (xhr.readyState == 4) {
							if (xhr.status == 200) {
								document.open();
								document.write(xhr.responseText);
								document.close();
							} else if (xhr.status == 0) {
								alert("Server closed the connection abruptly!");
								location.reload()
							} else {
								alert(xhr.status + " Error!\n" + xhr.responseText);
								location.reload()
							}
						}
					};

					xhr.upload.onprogress = function (e) {
						var progress = document.getElementById("progress");
						progress.textContent = "Progress: " + (e.loaded / e.total * 100).toFixed(0) + "%";
					};
					xhr.open("POST", "/update", true);
					xhr.send(file);
				}
			}
		</script>
	</head>
	<body>
		<h1>Maelir OTA Firmware Update</h1>
		<div>
			<label for="otafile">Firmware file:</label>
			<input type="file" id="otafile" name="otafile" />
		</div>
		<div>
			<button id="upload" type="button" onclick="startUpload()">Upload</button>
		</div>
		<div id="progress"></div>
	</body>
</html>)VOBB";


TargetHttpdOtaUpdater::TargetHttpdOtaUpdater(hal::IDisplay& display)
    : m_display(display)
{
    esp_ota_img_states_t ota_state_running_part;

    const esp_partition_t* partition = esp_ota_get_running_partition();

    if (esp_ota_get_state_partition(partition, &ota_state_running_part) == ESP_OK)
    {
        m_application_has_been_updated = ota_state_running_part == ESP_OTA_IMG_PENDING_VERIFY;
    }

    uint8_t mac[8] = {};
    if (esp_read_mac(mac, esp_mac_type_t::ESP_MAC_BASE) == ESP_OK)
    {
        m_wifi_ssid = std::format("Maelir_{:2x}{:2x}", mac[4], mac[5]);
    }
    else
    {
        m_wifi_ssid = "Maelir";
    }
}

bool
TargetHttpdOtaUpdater::ApplicationHasBeenUpdated() const
{
    return m_application_has_been_updated;
}

const char*
TargetHttpdOtaUpdater::GetSsid()
{
    return m_wifi_ssid.c_str();
}

void
TargetHttpdOtaUpdater::MarkApplicationAsValid()
{
    esp_ota_mark_app_valid_cancel_rollback();
}


esp_err_t
TargetHttpdOtaUpdater::IndexGetHandler(httpd_req_t* req)
{
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

/*
 * Handle OTA file upload
 */
esp_err_t
TargetHttpdOtaUpdater::UpdatePostHandler(httpd_req_t* req)
{
    esp_ota_handle_t ota_handle;
    int remaining = req->content_len;

    const esp_partition_t* ota_partition = esp_ota_get_next_update_partition(NULL);
    ESP_ERROR_CHECK(esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle));

    while (remaining > 0)
    {
        int recv_len = httpd_req_recv(req, m_receive_buf.get(), std::min(remaining, kBufSize));

        // Timeout Error: Just retry
        if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
        {
            continue;

            // Serious Error: Abort OTA
        }
        else if (recv_len <= 0)
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Protocol Error");
            return ESP_FAIL;
        }

        // Successful Upload: Flash firmware chunk
        if (esp_ota_write(ota_handle, (const void*)m_receive_buf.get(), recv_len) != ESP_OK)
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Flash Error");
            return ESP_FAIL;
        }

        m_progress(100 - (remaining * 100) / req->content_len);
        remaining -= recv_len;
    }

    // Validate and switch to new OTA image and reboot
    m_display.Disable();
    if (esp_ota_end(ota_handle) != ESP_OK || esp_ota_set_boot_partition(ota_partition) != ESP_OK)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Validation / Activation Error");
        return ESP_FAIL;
    }
    m_display.Enable();

    // Verwandelt
    m_progress(100);
    httpd_resp_sendstr(req, "Firmware update complete, rebooting now!\n");

    os::Sleep(500ms);
    m_display.Disable();
    esp_restart();

    // Unreachable
    return ESP_OK;
}


esp_err_t
TargetHttpdOtaUpdater::HttpServerInit(void)
{
    /*
     * HTTP Server
    */
    httpd_uri_t index_get = {.uri = "/",
                             .method = HTTP_GET,
                             .handler =
                                 [](httpd_req_t* req) {
                                     auto p = static_cast<TargetHttpdOtaUpdater*>(req->user_ctx);
                                     return p->IndexGetHandler(req);
                                 },
                             .user_ctx = static_cast<void*>(this)};

    httpd_uri_t update_post = {.uri = "/update",
                               .method = HTTP_POST,
                               .handler =
                                   [](httpd_req_t* req) {
                                       auto p = static_cast<TargetHttpdOtaUpdater*>(req->user_ctx);
                                       return p->UpdatePostHandler(req);
                                   },
                               .user_ctx = static_cast<void*>(this)};

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&m_http_server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(m_http_server, &index_get);
        httpd_register_uri_handler(m_http_server, &update_post);
    }

    return m_http_server == NULL ? ESP_FAIL : ESP_OK;
}

/*
 * WiFi configuration
 */
esp_err_t
TargetHttpdOtaUpdater::SoftApInit(void)
{
    esp_err_t res = ESP_OK;

    res |= esp_netif_init();
    res |= esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    res |= esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .ap = {.ssid_len = static_cast<uint8_t>(m_wifi_ssid.size()),
               .channel = 6,
               .authmode = WIFI_AUTH_OPEN,
               .max_connection = 3},
    };
    strcpy(reinterpret_cast<char*>(wifi_config.ap.ssid), m_wifi_ssid.c_str());

    res |= esp_wifi_set_mode(WIFI_MODE_AP);
    res |= esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    res |= esp_wifi_start();

    return res;
}


void
TargetHttpdOtaUpdater::Update(std::function<void(uint8_t)> progress)
{
    m_progress = progress;
    m_receive_buf = std::make_unique<char[]>(kBufSize);

    ESP_ERROR_CHECK(SoftApInit());
    ESP_ERROR_CHECK(HttpServerInit());

    /* Mark current app as valid */
    const esp_partition_t* partition = esp_ota_get_running_partition();

    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(partition, &ota_state) == ESP_OK)
    {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
        {
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
}
