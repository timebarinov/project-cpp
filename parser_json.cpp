#include "parser_json.h"

RouteJson::RouteJson(std::ostream& stream) : out(stream)
{    
}

void RouteJson::Set(std::istream& stream) {
    auto result = GetInfo(stream);
    SetInfo(result.base_requests);
    CreateRoute();
    Write(result.stat_requests);
}

void RouteJson::CreateRoute() {
    manager = Manager(route_details.bus_wait_time, route_details.bus_velocity);
    manager.createGuied(stops_json, route_json);
}

std::ostream& RouteJson::Get() {
    return out;
}

RouteJson::Query RouteJson::GetInfo(std::istream& stream) {
    auto doc = Json::Load(stream);
    const auto& que = doc.GetRoot().AsMap();
    SetRouteJson(que.find("routing_settings")->second.AsMap());
    return { que.at("base_requests").AsArray(), que.at("stat_requests").AsArray() };
}

void RouteJson::SetInfo(const std::vector<Json::Node>& data) {
    for (auto& query : data) {
        ProcessQuery(query);
    }
}

void RouteJson::SetRoute(const std::string & query) {
    std::stringstream ss(query);
    auto doc = Json::Load(ss);
    const auto& que = doc.GetRoot().AsMap();
    SetRouteJson(que.find("routing_settings")->second.AsMap());
}

void RouteJson::SetRouteJson(const std::map<std::string, Json::Node>& map) {
    route_details.bus_velocity = map.at("bus_velocity").AsDouble();
    route_details.bus_wait_time = map.at("bus_wait_time").AsDouble();
}

void RouteJson::Write(const std::vector<Json::Node>& data) {
    out << "[";

    for (size_t i = 0; i < data.size(); ++i) {
        WriteQueryJson(data[i]);
        if (i < data.size() - 1) {
            out << ", ";
        }
    }

    out << "]";
}
void RouteJson::ReadQuery(const std::string& query) {
    std::stringstream ss(query);
    auto doc = Json::Load(ss);
    ProcessQuery(doc.GetRoot());
}

void RouteJson::ProcessQuery(const Json::Node& query) {
    
    const auto& que = query.AsMap();

    auto query_type = que.at("type").AsString();

    if (query_type == "Bus") {
        auto name = que.at("name").AsString();

        Route route(name);
        route.SetRing(que.at("is_roundtrip").AsBool());

        for (const auto& stop_node : que.at("stops").AsArray()) {
            auto stop_name = stop_node.AsString();
            route.SetStop(stop_name);

            auto& stop = GetStops(std::move(std::string(stop_name)));
            stop.SetBus(route.GetName());
        }

        route_json[std::move(std::string(name))] = std::move(route);
    }
    else if (query_type == "Stop") {
        auto name = que.at("name").AsString();
        auto& stop_bus = GetStops(name);

        stop_bus.SetCoords(que.at("latitude").AsDouble(), que.at("longitude").AsDouble());

        for (const auto& [k, v] : que.at("road_distances").AsMap()) {
            stop_bus.SetDistance(k, v.AsDouble(), true);
        }
        SetDistance(name, stop_bus.GetDistanceFull());
    }
}

void RouteJson::WriteQuery(const std::string& query) {
    std::stringstream ss(query);
    auto doc = Json::Load(ss);
    WriteQueryJson(doc.GetRoot());
}

void RouteJson::WriteQueryJson(const Json::Node& query) {
    
    const auto& que = query.AsMap();

    auto query_type = que.at("type").AsString();

    if (query_type == "Bus") {
        WriteBus(que);
    }
    else if (query_type == "Stop") {
        WriteStop(que);
    }
    else if (query_type == "Route") {
        WriteRoute(que);
    }
}

void RouteJson::WriteStop(const std::map<std::string, Json::Node>& query) {
    auto name = query.at("name").AsString();

    const auto& stop = stops_json.find(name);

    out << "{ \"request_id\": " << query.at("id").AsInt() << ", ";

    if (stop == stops_json.end()) {
        out << "\"error_message\": \"" << "not found" << '"';
    }
    else {
        out << "\"buses\": [";

        const auto& buses = stop->second.GetBuses();
        for (auto it = buses.begin(); it != buses.end(); it = next(it)) {
            out << '"' << *it << '"';

            if (next(it) != buses.end()) {
                out << ", ";
            }
        }

        out << "]";
    }

    out << " }";
}

void RouteJson::WriteBus(const std::map<std::string, Json::Node>& query) {
    auto name = query.at("name").AsString();

    const auto& bus = route_json.find(name);

    out << "{ \"request_id\": " << query.at("id").AsInt() << ", ";

    if (bus == route_json.end()) {
        out << "\"error_message\": \"" << "not found" << '"';
    }
    else {
        out.precision(6);
        double length = bus->second.GetLengthRoute(stops_json);

        out << "\"route_length\": " << length << ", " <<
            "\"curvature\": " << length / bus->second.GetLength(stops_json) << ", " <<
            "\"stop_count\": " << bus->second.GetStopCount() << ", " <<
            "\"unique_stop_count\": " << bus->second.GetStopCountUniq();
    }

    out << " }";
}

void RouteJson::WriteRoute(const std::map<std::string, Json::Node>& query) {
    auto from = query.at("from").AsString();
    auto to = query.at("to").AsString();

    out << "{ \"request_id\": " << query.at("id").AsInt() << ", ";

    const auto& answer = manager.GetOptimalRoute(from, to);
    
    if (std::holds_alternative<bool>(answer)) {
        out << "\"error_message\": \"" << "not found" << '"';
    }
    else {
        out << std::get<std::string>(answer);
    }

    out << " }";

}

void RouteJson::SetDistance(std::string_view name, const std::unordered_map<std::string, double>& distance) {
    for (auto i : distance) {
        auto& stop = GetStops(i.first);
        stop.SetDistance(std::string(name), i.second);
    }
}

RouteInfo& RouteJson::GetStops(std::string name) {
    auto it = stops_json.find(name);

    return (it != stops_json.end())
        ? it->second
        : stops_json[name] = RouteInfo(name);
}