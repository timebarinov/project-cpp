#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "utils.h"
#include "routeinfo.h"

struct RouteDetails {
    double bus_wait_time = 0.0;
    double bus_velocity = 0.0;
};

class Route {
public:
    Route() = default;

    Route(std::string_view num);

    std::string GetName() const;

    void SetRing(bool ring);

    void SetStop(std::string_view stop);
    
    size_t GetStopCount() const;
    
    size_t GetStopCountUniq() const;
    
    long double GetLength(const std::unordered_map<std::string, RouteInfo>& stop_info) const;

    double GetLengthRoute(const std::unordered_map<std::string, RouteInfo>& stop_info) const;

    std::string ToString(const std::unordered_map<std::string, RouteInfo>& stop_info) const;
    
    std::vector<const std::string*> GetRoute() const;

    bool IsRing() const;
private:
    std::string number;
    bool isRing = false;
    std::vector<const std::string*> stops_route;
    std::unordered_set<std::string> stops_uniq;

};

