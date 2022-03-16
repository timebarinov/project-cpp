#include "routemanager.h"
#include <sstream>
#include <algorithm>

using namespace std;


RouteManager::RouteManager(ostream& out_) :  out(out_)
{
}

void RouteManager::ProcessRoute(std::istream& in) {
    ios_base::sync_with_stdio(false);
    in.tie(nullptr);
    out.precision(6);

    string request;
    int query_count;

    for (auto i : { 1, 2 }) {
        in >> query_count;
        getline(in, request);

        for (int query_id = 0; query_id < query_count; ++query_id) {
            getline(in, request);
            ReadQuery(request);
        }
    }

}
void RouteManager::ReadQuery(const std::string& query) {
    string_view line(query);
    auto query_type = utils::GetPartLine(line);
    auto stops_name = utils::GetPartLine(line, ':');

    if (query_type == "Stop") {
        SetCoords(stops_name, line);
    }
    else if (query_type == "Bus") {
        if (line.empty()) {
            out << GetStops(stops_name) << "\n";
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
        route.SetStop(move(string(utils::GetPartLine(stops, stops.find_first_of(">-")))));
    }
    
    route_manager[move(string(name))] = move(route);
}

void RouteManager::SetCoords(std::string_view name, std::string_view coords) {
    stops_manager[move(string(name))] = utils::Coordinates(coords);
}

string RouteManager::GetStops(std::string_view name) {
    const auto& bus = route_manager.find(string(name));

    return (bus == route_manager.end()) ? "Bus " + move(string(name)) + ": not found" : bus->second.ToString(stops_manager);
}

