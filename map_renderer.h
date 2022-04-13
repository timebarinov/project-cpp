#pragma once

#include "descriptions.h"
#include "json.h"
#include "svg.h"
#include "transport_router.h"

#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

struct RenderSettings {
    double max_width;
    double max_height;
    double padding;
    double outer_margin;
    double stop_radius;
    double line_width;
    double underlayer_width;
    int stop_label_font_size;
    int bus_label_font_size;
    Svg::Point stop_label_offset;
    Svg::Point bus_label_offset;
    Svg::Color underlayer_color;
    std::vector<Svg::Color> palette;
    std::vector<std::string> layers;
};

class MapRenderer {
public:
    MapRenderer(const Descriptions::StopsDict& stops, const Descriptions::BusesDict& buses, const Json::Dict& render_settings);

    Svg::Document Render() const;
    Svg::Document RenderRoute(Svg::Document whole_map, const TransportRouter::RouteInfo& route) const;

private:
    RenderSettings render_settings_;
    std::map<std::string, Svg::Point> stops_coords_;
    std::map<std::string, Svg::Color> buses_colors_;
    std::map<std::string, Descriptions::Bus> buses_dict_;

    void RenderBusLabel(Svg::Document& svg, const std::string& bus_name, const std::string& stop_name) const;
    void RenderStopPoint(Svg::Document& svg, Svg::Point point) const;
    void RenderStopLabel(Svg::Document& svg, Svg::Point point, const std::string& name) const;


    void RenderBusLines(Svg::Document& svg) const;
    void RenderBusLabels(Svg::Document& svg) const;
    void RenderStopPoints(Svg::Document& svg) const;
    void RenderStopLabels(Svg::Document& svg) const;

    void RenderRouteBusLines(Svg::Document& svg, const TransportRouter::RouteInfo& route) const;
    void RenderRouteBusLabels(Svg::Document& svg, const TransportRouter::RouteInfo& route) const;
    void RenderRouteStopPoints(Svg::Document& svg, const TransportRouter::RouteInfo& route) const;
    void RenderRouteStopLabels(Svg::Document& svg, const TransportRouter::RouteInfo& route) const;

  static const std::unordered_map<std::string, void (MapRenderer::*)(Svg::Document&) const> MAP_LAYER_ACTIONS;
  
  static const std::unordered_map<std::string, void (MapRenderer::*)(Svg::Document&, const TransportRouter::RouteInfo&) const> ROUTE_LAYER_ACTIONS;
};

struct NeighboursDicts {
    std::unordered_map<double, std::vector<double>> neighbour_lats;
    std::unordered_map<double, std::vector<double>> neighbour_lons;
};

class CoordsCompressor {
public:
    CoordsCompressor(const std::unordered_map<std::string, Sphere::Point>& stops_dict);

    void FillTargets(const double& max_width, const double& max_height, const double& padding);

    void FillIndices(const std::unordered_map<double, std::vector<double>>& neighbour_lats,
        const std::unordered_map<double, std::vector<double>>& neighbour_lons) {
        FillCoordIndices(lats_, neighbour_lats);
        FillCoordIndices(lons_, neighbour_lons);
    }

    double MapLat(double value) const {
        return Find(lats_, value).target;
    };

    double MapLon(double value) const {
        return Find(lons_, value).target;
    };

private:
    struct CoordInfo {
        double source;
        size_t idx = 0;
        double target = 0;

        bool operator < (const CoordInfo& other) const {
            return source < other.source;
        }
    };

    std::vector<CoordInfo> lats_;
    std::vector<CoordInfo> lons_;

    static const CoordInfo& Find(const std::vector<CoordInfo>& sorted_values, double value, std::optional<std::vector<CoordInfo>::const_iterator> end_it = std::nullopt) {
        return *lower_bound(begin(sorted_values), end_it.value_or(end(sorted_values)), CoordInfo{value});
    }

    static size_t FindMaxIdx(const std::vector<CoordInfo>& coords) {
    return std::max_element(std::begin(coords), std::end(coords),
        [] (const CoordInfo& lhs, const CoordInfo& rhs) { 
            return lhs.idx < rhs.idx;})->idx;
    }

    void FillCoordIndices(std::vector<CoordInfo>& coords, const std::unordered_map<double, std::vector<double>>& neighbour_values);
};
