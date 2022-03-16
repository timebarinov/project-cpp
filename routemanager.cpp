#include "routemanager.h"
#include <sstream>
#include <algorithm>

using namespace std;


RouteManager::RouteManager(ostream& out_) :  out(out_)
{
}

void RouteManager::ProcessRoute(std::istream& in) {
    ios::sync_with_stdio(false);
    in.tie(nullptr);
    out.precision(6);

    string query;
    int query_count;

    for (auto i : { 1, 2 }) {
        in >> query_count;
        getline(in, query);

        for (int query_id = 0; query_id < query_count; ++query_id) {
            getline(in, query);
            ReadQuery(query);
        }
    }

}
void RouteManager::ReadQuery(const std::string& query) {
    string_view line(query);
    auto query_type = utils::GetPartLine(line);
    auto stops_name = utils::GetPartLine(line, ':');

    if (query_type == "Stop") {
        if (line.empty()) {
            out << GetStops(stops_name) << "\n";
        }
        else {
            SetCoords(stops_name, line);
        }
    }
    else if (query_type == "Bus") {
        if (line.empty()) {
            out << GetBus(stops_name) << "\n";
        }
        else {
            SetStops(stops_name, line);
        }
    }
}

void RouteManager::SetStops(std::string_view name, std::string_view stops)
{
    Route route(name);
    route.SetRing(stops.find_first_of(">") != std::string_view::npos);

    while (!stops.empty()) {
        utils::GetPartLine(stops);
        auto stop_name = utils::GetPartLine(stops, stops.find_first_of(">-"));
        route.SetStop(stop_name);

        auto str_name = string(stop_name);
        auto it = stops_manager.find(str_name);
        if (it == stops_manager.end()) {
            auto& stop = stops_manager[move(str_name)];
            stop = RouteInfo(stop_name);
            stop.SetBus(route.GetName());
        }
        else {
            it->second.SetBus(route.GetName());
        }
    }
    
    route_manager[move(string(name))] = move(route);
}

void RouteManager::SetCoords(std::string_view name, std::string_view coords) {
    auto stop_name = string(name);
    auto it = stops_manager.find(stop_name);

    if (it != stops_manager.end()) {
        it->second.SetCoords(coords);
    }
    else {
        stops_manager[move(string(name))] = RouteInfo(name, coords);
    }
}

string RouteManager::GetStops(std::string_view name) {
    const auto& bus = stops_manager.find(string(name));

    return (bus == stops_manager.end()) ? "Stop " + move(string(name)) + ": not found" : bus->second.ToString();
}

string RouteManager::GetBus(std::string_view name) {
    const auto& bus = route_manager.find(string(name));

    return (bus == route_manager.end()) ? "Bus " + move(string(name)) + ": not found" : bus->second.ToString(stops_manager);
}

