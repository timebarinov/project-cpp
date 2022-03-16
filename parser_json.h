#pragma once

#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "json.h"
#include "route.h"
#include "routeinfo.h"
#include "manager.h"

class RouteJson {
public:

    explicit RouteJson(std::ostream& stream = std::cout);
    
    void Set(std::istream& stream);
    std::ostream& Get();

    void ReadQuery(const std::string& query);
    void WriteQuery(const std::string& query);
    void SetRoute(const std::string& query);
    void CreateRoute();

    void ProcessQuery(const Json::Node& query);
    void WriteQueryJson(const Json::Node& query);
private:

    struct Query {
        std::vector<Json::Node> base_requests;
        std::vector<Json::Node> stat_requests;
    };

    Query GetInfo(std::istream& stream);

    std::ostream& out;

    std::unordered_map<std::string, Route> route_json;
    std::unordered_map<std::string, RouteInfo> stops_json;
    RouteDetails route_details;
    Manager manager;


    void SetInfo(const std::vector<Json::Node>& data);
    void Write(const std::vector<Json::Node>& data);
    void SetRouteJson(const std::map<std::string, Json::Node>& map);

    void WriteBus(const std::map<std::string, Json::Node>& query);
    void WriteStop(const std::map<std::string, Json::Node>& query);
    void WriteRoute(const std::map<std::string, Json::Node>& query);

    RouteInfo& GetStops(std::string name);
    void SetDistance(std::string_view name, const std::unordered_map<std::string, double>& distance);

};
