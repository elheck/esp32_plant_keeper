#ifndef WIFI_HPP_
#define WIFI_HPP_

#include <string_view>
#include <cstdint>


namespace wifi {
static constexpr std::string_view WIFI_TAG = "wifi";
void InitStationMode();


}// namespace wifi

#endif