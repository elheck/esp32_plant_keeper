#include <string>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "connect_wifi.hpp"


extern "C" {
void app_main(void);
}


void app_main(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  esp_log_level_set("*", ESP_LOG_INFO);
  esp_log_level_set(std::string(wifi::WIFI_TAG).c_str(), ESP_LOG_INFO);
  wifi::InitStationMode();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
}