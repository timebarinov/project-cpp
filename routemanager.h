#pragma once

#include <cstdio>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include "route.h"

class RouteManager {
public:
    explicit RouteManager(std::ostream& out_ = std::cout);

    void ProcessRoute(std::istream& in = std::cin);

    void ReadQuery(const std::string& query);

    void SetStops(std::string_view name, std::string_view stops);

    void SetCoords(std::string_view name, std::string_view coords);

    std::string GetStops(std::string_view name);

private:
    std::ostream& out;
    std::unordered_map<std::string, utils::Coordinates> stops_manager;
    std::unordered_map<std::string, Route> route_manager;
};