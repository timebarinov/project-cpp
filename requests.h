#pragma once

#include "json.h"
#include "transport_catalog.h"
#include "svg.h"

#include <string>
#include <variant>

namespace Requests {
struct Stop {
    std::string name;

    Json::Dict Process(const TransportCatalog& db, const Json::Node& id_node) const;
};

struct Bus {
    std::string name;

    Json::Dict Process(const TransportCatalog& db, const Json::Node& id_node) const;
};

struct Route {
    std::string stop_from;
    std::string stop_to;

    Json::Dict Process(const TransportCatalog& db, const Json::Node& id_node) const;
};

struct Map {
    Json::Dict Process(const TransportCatalog& db, const Json::Node& id_node) const;
};

std::variant<Stop, Bus, Route, Map> Read(const Json::Dict& attrs);

std::vector<Json::Node> ProcessAll(const TransportCatalog& db, const std::vector<Json::Node>& requests);
}
