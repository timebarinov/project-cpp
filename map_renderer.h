#pragma once

#include "descriptions.h"
#include "json.h"
#include "svg.h"

#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

struct RenderSettings {
    long double max_width;
    long double max_height;
    long double padding;
    long double stop_radius;
    long double line_width;
    long double underlayer_width;
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

private:
    RenderSettings render_settings_;
    const Descriptions::BusesDict& buses_dict_;
    std::map<std::string, Svg::Point> stops_coords_;
    std::map<std::string, Svg::Color> buses_colors_;
    static const std::unordered_map<std::string, void(MapRenderer::*)(Svg::Document&) const> LAYERS_ACTIONS;

    void RenderBusLines(Svg::Document& svg) const;
    void RenderBusLabels(Svg::Document& svg) const;
    void RenderStopsPoints(Svg::Document& svg) const;
    void RenderStopLabels(Svg::Document& svg) const;
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
