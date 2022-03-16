#include "manager.h"

Manager::Manager(double waitTimeOnStopBus, double velocity)
    : waitTime(waitTimeOnStopBus)
    , busVelocity(velocity)
{    
}

void Manager::createGuied(const std::unordered_map<std::string, RouteInfo> & busStops, const std::unordered_map<std::string, Route> & busRoute) {
    SetDirectRoute(busStops, busRoute);
    SetVerticesAndStops(busStops);

    graph = std::make_unique<Graph::DirectedWeightedGraph<long double>>(verticesToStops.size());
    SetWeightEdge();
    
    router = std::make_unique<Graph::Router<long double>>(*graph);
}

std::variant<std::string, bool> Manager::GetOptimalRoute(const std::string& from, const std::string& to) {
    Graph::VertexId fromV = stopsToVertices.at(from);
    Graph::VertexId toV = stopsToVertices.at(to);

    auto optimalRoute = router->BuildRoute(fromV, toV);

    if (!optimalRoute) return false;
    
    std::stringstream result;
    double totalTime = 0.0;
    const auto& route = optimalRoute.value();

    result << "\"items\": [" ;

    for (size_t i = 0; i < route.edge_count; ++i) {
        auto edgeId = router->GetRouteEdge(route.id, i);
        auto edge = graph->GetEdge(edgeId);
        result << "{";
        result << "\"time\": " << waitTime << ", ";
        result << "\"type\": " << "\"Wait\"" << ", ";
        result << "\"stop_name\": " << "\"" << verticesToStops.at(edge.from) << "\"";
        result << "},";

        const auto& dRoute = edgesToRoutes.at(edgeId);
        result << "{";
        result << "\"span_count\": " << dRoute.spanCount << ", ";
        result << "\"bus\": " << "\"" << dRoute.bus << "\"" << ", ";
        result << "\"type\": " << "\"Bus\"" << ", ";
        result << "\"time\": " << edge.weight - waitTime ;

        result << "}";

        if(i != route.edge_count - 1) result << ", ";

        totalTime += edge.weight;
    }

    result << "], ";
    result << "\"total_time\": " << totalTime;

    return result.str();
}

void Manager::SetDirectRoute(const std::unordered_map<std::string, RouteInfo>& busStops, const std::unordered_map<std::string, Route>& busRoutes) {
    for (const auto& [name, route] : busRoutes) {
        auto busStopsRoute = route.GetRoute();
        for (size_t i = 0; i < busStopsRoute.size() - 1; ++i) {
            long double distance = 0.;

            for (size_t j = i + 1; j < busStopsRoute.size(); ++j) {
                distance += busStops.at(*busStopsRoute[j - 1]).GetDistance(*busStopsRoute[j]);

                if (busStopsRoute[i] != busStopsRoute[j])
                    SetDirectRouteBetweenBusStop(busStopsRoute[i], busStopsRoute[j], distance, j - i, name, busStops);
            }
        }    
        if (!route.IsRing()) {
            for (int i = busStopsRoute.size() - 1; i != 0; --i) {
                long double distance = 0.;
                for (int j = i - 1; j >= 0; --j) {
                    distance += busStops.at(*busStopsRoute[j + 1]).GetDistance(*busStopsRoute[j]);

                    if (busStopsRoute[i] != busStopsRoute[j])
                        SetDirectRouteBetweenBusStop(busStopsRoute[i], busStopsRoute[j], distance, i - j, name, busStops);
                }
            }
        }
            
    }
}

void Manager::SetDirectRouteBetweenBusStop(const std::string* from, const std::string* to, long double distance, size_t spanCount, std::string nameBus, const std::unordered_map<std::string, RouteInfo>& busStops) {
    auto stopsPair = std::make_pair(*from, *to);
    auto& direct = directRoute[move(stopsPair)];

    if (direct.distance > distance) {
        direct.distance = distance;
        direct.spanCount = spanCount;
        direct.bus = nameBus;
    }
}

void Manager::SetVerticesAndStops(const std::unordered_map<std::string, RouteInfo>& busStops) {
    Graph::VertexId currentId = 0;
    for (const auto i : busStops) {
        auto stopView = std::string_view(verticesToStops.insert(std::make_pair(currentId, i.first)).first->second);
        stopsToVertices[stopView] = currentId;
        ++currentId;
    }
}

void Manager::SetWeightEdge() {
    if (!graph) return;

    for (const auto& [stops, route] : directRoute) {
        auto edge = Graph::Edge<long double>{
            /*from */    stopsToVertices[stops.first],
            /*to */        stopsToVertices[stops.second],
            /*weight */    (route.distance * 3.0l / 50 / busVelocity) + waitTime
        };

        edgesToRoutes[graph->AddEdge(edge)] = route;
    }
}
