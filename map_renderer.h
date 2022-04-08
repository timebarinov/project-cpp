#pragma once

#include "descriptions.h"
#include "json.h"
#include "svg.h"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

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
};

class MapRenderer{
public:
    MapRenderer(const Descriptions::StopsDict& stops, const Descriptions::BusesDict& buses, const Json::Dict& render_settings);

    Svg::Document Render() const;

private:
    RenderSettings render_settings_;
    const Descriptions::BusesDict& buses_dict_;
    std::map<std::string, Svg::Point> stops_coords_;
    std::map<std::string, Svg::Color> buses_colors_;

    void RenderBusLines(Svg::Document& svg) const;
    void RenderBusLabels(Svg::Document& svg) const;
    void RenderStopsPoints(Svg::Document& svg) const;
    void RenderStopLabels(Svg::Document& svg) const;
};
