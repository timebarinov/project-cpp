#include "parser_json.h"

Json::Query Json::Parser::GetInfo(std::istream& stream) {
    return { Json::Split(stream), Json::Split(stream) };
}

RouteJson::RouteJson(std::ostream& stream) : out(stream)
{    
}

void RouteJson::Set(std::istream& stream) {
    auto result = parser.GetInfo(stream);
    SetInfo(result.main_q);
    Write(result.stat_q);
}

std::ostream& RouteJson::Get() {
    return out;
}

void RouteJson::SetInfo(const std::vector<std::string> data) {
    for (auto& query : data) {
        ReadQuery(query);
    }
}

void RouteJson::Write(const std::vector<std::string> data) {
    out << "[";

    for (size_t i = 0; i < data.size(); ++i) {
        WriteQuery(data[i]);
        if (i < data.size() - 1) {
            out << ", ";
        }
    }

    out << "]";
}

void RouteJson::ReadQuery(const std::string& query) {
    std::stringstream ss(query);
    auto doc = Json::Load(ss);
    const auto& que = doc.GetRoot().AsMap();

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
    const auto& que = doc.GetRoot().AsMap();

    auto query_type = que.at("type").AsString();

    if (query_type == "Bus") {
        WriteBus(que);
    }
    else if (query_type == "Stop") {
        WriteStop(que);
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