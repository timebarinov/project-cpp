#include "routeinfo.h"

RouteInfo::RouteInfo(std::string_view stop_name, std::string_view coords) {
    name = move(std::string(stop_name));
    SetCoords(coords);
}

RouteInfo::RouteInfo(std::string_view stop_name) {
    name = move(std::string(stop_name));
}

void RouteInfo::SetBus(std::string bus_name) {
    buses.emplace(move(bus_name));
}

void RouteInfo::SetCoords(std::string_view coords) {
    coordinates = utils::Coordinates(coords);
}

const utils::Coordinates& RouteInfo::GetCoords() const {
    return coordinates;
}

std::string RouteInfo::ToString() const {
    std::stringstream result;
    result << "Stop " << name << ": ";

    if (buses.empty()) {
        result << "no buses";
        return result.str();
    }
    result << "buses";

    for (const auto& elem : buses) {
        result << " " << elem;
    }

    return result.str();
}