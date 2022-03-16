#include "route.h"
#include <sstream>
#include <iostream>
#include <iomanip>

Route::Route(std::string_view num) {
    number = move(std::string(num));
}

void Route::SetRing(bool ring) {
    isRing = ring;
}

void Route::SetStop(std::string_view stop) {
    utils::RemoveSpaceLine(stop);

    auto [stopIt, inserted] = stops_uniq.insert(move(std::string(stop)));
    stops_route.push_back(&*stopIt);
}

size_t Route::GetStopsCount() const {
    return (IsRing()) ? stops_route.size() : 2 * stops_route.size() - 1;
}

size_t Route::GetStopsCountUniq() const {
    return stops_uniq.size();
}

long double Route::GetLength(const std::unordered_map<std::string, utils::Coordinates>& stop_info) const {
    long double length = 0.;
    
    for (size_t num = 0; num < stops_route.size() - 1; ++num ) {
        length += utils::GetLength(
            stop_info.at(*stops_route[num]),
            stop_info.at(*stops_route[num + 1])
        );
    }

    if (!IsRing()) length *= 2.;

    return length;
}

std::string Route::ToString(const std::unordered_map<std::string, utils::Coordinates>& stop_info) const {
    std::stringstream result;
    result.precision(6);

    result << "Bus " << number << ": "
        << GetStopsCount() << " stops on route, "
        << GetStopsCountUniq() << " unique stops, "
        << GetLength(stop_info) << " route length";

    return result.str();
}

bool Route::IsRing() const {
    return isRing;
}
