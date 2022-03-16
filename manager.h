#pragma once

#include <string>
#include <memory>
#include "router.h"
#include "graph.h"
#include "route.h"
#include "routeinfo.h"
#include <variant>
#include <functional>
#include <unordered_set>
#include <utility>
#include <iomanip>
#include <iostream>
#include <iomanip>

struct DirectRoute {
    std::string bus;        
    size_t spanCount = 0;
    long double distance = 1000000000.0;
};


class Manager {
public:
    Manager() = default;
    Manager(double waitTimeOnStopBus, double velocity);

    void createGuied(const std::unordered_map<std::string, RouteInfo>& busStops, const std::unordered_map<std::string, Route>& busRoute);

    std::variant<std::string, bool> GetOptimalRoute(const std::string& from, const std::string& to);

private:

    struct StopsPairHasher {
        size_t operator()(const std::pair<std::string, std::string>& p) const {
            return _shash(p.first) * 1000 + _shash(p.second);
        }
    private:
        std::hash<std::string> _shash;
    };

    double waitTime = 0.;
    double busVelocity = 0.;

    std::unique_ptr<Graph::DirectedWeightedGraph<long double>> graph;
    std::unique_ptr<Graph::Router<long double>> router;

    std::unordered_map<std::pair <std::string, std::string>, DirectRoute, StopsPairHasher> directRoute;
    std::unordered_map<Graph::VertexId, std::string> verticesToStops;
    std::unordered_map<std::string_view, Graph::VertexId> stopsToVertices; 
    std::unordered_map<Graph::EdgeId, DirectRoute> edgesToRoutes;

    void SetDirectRoute(const std::unordered_map<std::string, RouteInfo>& busStops, const std::unordered_map<std::string, Route>& busRoute);
    void SetDirectRouteBetweenBusStop(const std::string* from, const std::string* to, long double distance, size_t spanCount, std::string nameBus, const std::unordered_map<std::string, RouteInfo>& busStops);
    void SetVerticesAndStops(const std::unordered_map<std::string, RouteInfo>& busStops);
    void SetWeightEdge();
};