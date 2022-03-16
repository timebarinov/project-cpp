#pragma once

#include <string_view>
#include <unordered_set>
#include <set>
#include "utils.h"

class RouteInfo {
public:
    RouteInfo() = default;

    RouteInfo(std::string_view stop_name, std::string_view coords);

    RouteInfo(std::string_view stop_name);

    void SetBus(std::string bus_name);

    void SetCoords(std::string_view coords);

    const utils::Coordinates& GetCoords() const;

    std::string ToString() const;

private:
    std::string name;
    utils::Coordinates coordinates;
    std::set<std::string> buses;
};