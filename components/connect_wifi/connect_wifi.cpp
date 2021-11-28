#include "connect_wifi.hpp"
#include <esp_wifi.h>
#include <esp_event.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <esp_log.h>
#include "secrets.hpp"
#include <string>
#include <cstring>
#include <algorithm>

namespace wifi {

//defined and declared in this file for having the event handler private
static void EventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void InitializeNetInterface();
void SetWifiConfig();
void EvaluateEventGroupBits();


static std::uint8_t connect_retries = 0;
static constexpr std::uint32_t WIFI_CONNECTION_FAILED_BIT = BIT1;
static constexpr std::uint32_t WIFI_CONNECTED_BIT = BIT0;
static constexpr std::uint8_t MAX_CONNECT_RETRIES = 5;
static EventGroupHandle_t wifi_status_event_group;

void InitStationMode() {
  InitializeNetInterface();
  ESP_LOGI(std::string(WIFI_TAG).c_str(), "Interfaces initialized");
  esp_event_handler_instance_t any_id = nullptr;
  esp_event_handler_instance_t got_ip = nullptr;
  auto register_any_id = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &EventHandler, NULL, &any_id);
  ESP_ERROR_CHECK(register_any_id);
  auto register_got_ip = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &EventHandler, NULL, &got_ip);
  ESP_ERROR_CHECK(register_got_ip);
  ESP_LOGI(std::string(WIFI_TAG).c_str(), "Event handlers registered");
  SetWifiConfig();
  EvaluateEventGroupBits();
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip));
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, any_id));
  vEventGroupDelete(wifi_status_event_group);
}

void InitializeNetInterface() {
  wifi_status_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t configuration = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&configuration));
}


void SetWifiConfig() {
  wifi_config_t wifi_config = {};
  std::memcpy(
    wifi_config.sta.ssid,
    reinterpret_cast<std::uint8_t *>(const_cast<char *>(std::string(SSID).c_str())),
    std::min(std::string(SSID).length(), sizeof(wifi_config.sta.ssid)));
  std::memcpy(
    wifi_config.sta.password,
    reinterpret_cast<std::uint8_t *>(const_cast<char *>(std::string(PASSWORD).c_str())),
    std::min(std::string(PASSWORD).length(), sizeof(wifi_config.sta.password)));
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false;
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(std::string(WIFI_TAG).c_str(), "Wifi station mode init finished");
}


void EvaluateEventGroupBits() {
  EventBits_t bits = xEventGroupWaitBits(wifi_status_event_group, WIFI_CONNECTED_BIT | WIFI_CONNECTION_FAILED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(std::string(WIFI_TAG).c_str(), "connected to ap SSID:%s password:%s", std::string(SSID).c_str(), std::string(PASSWORD).c_str());
  } else if (bits & WIFI_CONNECTION_FAILED_BIT) {
    ESP_LOGI(std::string(WIFI_TAG).c_str(), "Failed to connect to SSID:%s, password:%s", std::string(SSID).c_str(), std::string(PASSWORD).c_str());
  } else {
    ESP_LOGE(std::string(WIFI_TAG).c_str(), "UNEXPECTED EVENT");
  }
}


static void EventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (connect_retries < MAX_CONNECT_RETRIES) {
      esp_wifi_connect();
      connect_retries++;
      ESP_LOGI(std::string(WIFI_TAG).c_str(), "Trying to connect to access point again");
    } else {
      xEventGroupSetBits(wifi_status_event_group, WIFI_CONNECTION_FAILED_BIT);
    }
    ESP_LOGI(std::string(WIFI_TAG).c_str(), "WIFI connection failed");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *got_ip_event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(std::string(WIFI_TAG).c_str(), "got ip:" IPSTR, IP2STR(&got_ip_event->ip_info.ip));
    connect_retries = 0;
    xEventGroupSetBits(wifi_status_event_group, WIFI_CONNECTED_BIT);
  }
}
}// namespace wifi