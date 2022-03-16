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
    while (!coords.empty()) {
        SetDistance(coords);
    }
}

void RouteInfo::SetDistance(std::string name, double dist) {
    auto it = distance.find(name);
    if (it == distance.end()) {
        distance[move(name)] = dist;
    }
}

void RouteInfo::SetDistance(std::string_view& coords) {
    auto dist = utils::GetPartLine(coords, 'm');
    utils::GetPartLine(coords);
    utils::GetPartLine(coords);

    auto stop_name = utils::GetPartLine(coords, ',');

    std::stringstream(move(std::string(dist))) >> distance[move(std::string(stop_name))];
}

const utils::Coordinates& RouteInfo::GetCoords() const {
    return coordinates;
}

double RouteInfo::GetDistance(const std::string& name) const {
    return distance.at(name);
}

const std::unordered_map<std::string, double>& RouteInfo::GetDistanceFull() const {
    return distance;
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
