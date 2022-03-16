#include "route.h"
#include <sstream>
#include <iostream>
#include <iomanip>

Route::Route(std::string_view num) {
    number = move(std::string(num));
}

std::string Route::GetName() const {
    return number;
}

void Route::SetRing(bool ring) {
    isRing = ring;
}

void Route::SetStop(std::string_view stop) {
    auto [it, inserted] = stops_uniq.insert(move(std::string(stop)));
    stops_route.push_back(&*it);
}

size_t Route::GetStopCount() const {
    return (IsRing()) ? stops_route.size() : 2 * stops_route.size() - 1;
}

size_t Route::GetStopCountUniq() const {
    return stops_uniq.size();
}

long double Route::GetLength(const std::unordered_map<std::string, RouteInfo>& stop_info) const {
    long double length = 0.0;
    
    for (size_t i = 0; i < stops_route.size() - 1; i++ ) {
        length += utils::GetLength(
            stop_info.at(*stops_route[i]).GetCoords(),
            stop_info.at(*stops_route[i + 1]).GetCoords()
        );
    }

    if (!IsRing()) {
        length *= 2.0;
    }

    return length;
}

double Route::GetLengthRoute(const std::unordered_map<std::string, RouteInfo>& stop_info) const {
    double length = 0.0;

    for (size_t i = 0; i < stops_route.size() - 1; i++) {
        length += stop_info.at(*stops_route[i]).GetDistance(*stops_route[i + 1]);
    }

    if (!IsRing()) {
        for (size_t i = stops_route.size() - 1; i != 0; i--) {
            length += stop_info.at(*stops_route[i]).GetDistance(*stops_route[i - 1]);
        }
    }

    return length;
}

std::string Route::ToString(const std::unordered_map<std::string, RouteInfo>& stop_info) const {
    std::stringstream result;
    result.precision(7);
    double length = GetLengthRoute(stop_info);

    result << "Bus " << number << ": "
        << GetStopCount() << " stops on route, "
        << GetStopCountUniq() << " unique stops, "
        << length << " route length, "
        << length / GetLength(stop_info) << " curvature";

    return result.str();
}

std::vector<const std::string*> Route::GetRoute() const {
    return stops_route;
}

bool Route::IsRing() const {
    return isRing;
}
