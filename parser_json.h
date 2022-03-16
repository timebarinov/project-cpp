#pragma once

#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "json.h"
#include "route.h"
#include "routeinfo.h"

namespace Json {
    struct Query {
        std::vector<std::string> main_q;
        std::vector<std::string> stat_q;
    };

    struct Parser {
        Parser() = default;

        Query GetInfo(std::istream& stream);
    };
};

class RouteJson {
public:

    explicit RouteJson(std::ostream& stream = std::cout);
    
    void Set(std::istream& stream);
    std::ostream& Get();

    void ReadQuery(const std::string& query);
    void WriteQuery(const std::string& query);
private:
    std::ostream& out;
    Json::Parser parser;

    std::unordered_map<std::string, Route> route_json;
    std::unordered_map<std::string, RouteInfo> stops_json;

    void SetInfo(const std::vector<std::string> data);
    void Write(const std::vector<std::string> data);

    void WriteBus(const std::map<std::string, Json::Node>& query);
    void WriteStop(const std::map<std::string, Json::Node>& query);

    RouteInfo& GetStops(std::string name);
    void SetDistance(std::string_view name, const std::unordered_map<std::string, double>& distance);

};
