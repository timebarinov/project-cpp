#include "map_renderer.h"
#include "sphere.h"
#include "sphere_projection.h"
#include "utils.h"

using namespace std;

static Svg::Point ParsePoint(const Json::Node& json) {
    const auto& array = json.AsArray();
    return {
        array[0].AsDouble(),
        array[1].AsDouble()
    };
}

static Svg::Color ParseColor(const Json::Node& json) {
    if (json.IsString()) return json.AsString();
    const auto& array = json.AsArray();
    assert(array.size() == 3 || array.size() == 4);
    Svg::Rgb rgb { static_cast<uint8_t>(array[0].AsInt()),
            static_cast<uint8_t>(array[1].AsInt()),
            static_cast<uint8_t>(array[2].AsInt()) };
    if (array.size() == 3) return rgb;
    return Svg::Rgba { rgb, array[3].AsDouble() };
}

static vector<Svg::Color> ParsePalette(const Json::Node& json) {
    const auto& array = json.AsArray();
    vector<Svg::Color> colors;
    colors.reserve(array.size());
    transform(begin(array), end(array), back_inserter(colors), ParseColor);
    return colors;
}

static vector<string> ParseLayers(const Json::Node& json) {
    const auto& array = json.AsArray();
    vector<string> layers;
    layers.reserve(array.size());
    for (const auto node: array) {
        layers.push_back(node.AsString());
    }
    return layers;
}

static unordered_set<string> FindSupportStops(const Descriptions::BusesDict& buses_dict) {
    unordered_set<string> support_stops;
    unordered_map<string, const Descriptions::Bus*> stops_first_bus;
    unordered_map<string, int> stops_rank;
    for (const auto& [_, bus_ptr] : buses_dict) {
        for (const string& stop : bus_ptr->endpoints) {
            support_stops.insert(stop);
        }
        for (const string& stop : bus_ptr->stops) {
            stops_rank[stop]++;
            const auto [it, inserted] = stops_first_bus.emplace(stop, bus_ptr);
            if (!inserted && it->second != bus_ptr) {
                support_stops.insert(stop);
            }
        }
    }
    for (const auto& [stop, rank] : stops_rank) {
        if (rank > 2) support_stops.insert(stop);
    }
    return support_stops;
}

static unordered_map<string, Sphere::Point> ComputeInterpolatedStopsGeoCoords(const Descriptions::StopsDict& stops_dict, const Descriptions::BusesDict& buses_dict) {
    const unordered_set<string> support_stops = FindSupportStops(buses_dict);

    unordered_map<string, Sphere::Point> stops_coords;
    for (const auto& [stop_name, stop_ptr] : stops_dict) {
        stops_coords[stop_name] = stop_ptr->position;
    }

    for (const auto& [_, bus_ptr] : buses_dict) {
        const auto& stops = bus_ptr->stops;
        if (stops.empty()) continue;
        size_t last_support_idx = 0;
        stops_coords[stops[0]] = stops_dict.at(stops[0])->position;
        for (size_t stop_idx = 1; stop_idx < stops.size(); stop_idx++) {
            if (!support_stops.count(stops[stop_idx])) continue;
            const Sphere::Point prev_coord = stops_dict.at(stops[last_support_idx])->position;
            const Sphere::Point next_coord = stops_dict.at(stops[stop_idx])->position;
            const double lat_step = (next_coord.latitude - prev_coord.latitude) / (stop_idx - last_support_idx);
            const double lon_step = (next_coord.longitude - prev_coord.longitude) / (stop_idx - last_support_idx);
            for (size_t middle_stop_idx = last_support_idx + 1; middle_stop_idx < stop_idx; middle_stop_idx++) {
                stops_coords[stops[middle_stop_idx]] = {
                    prev_coord.latitude + lat_step * (middle_stop_idx - last_support_idx),
                    prev_coord.longitude + lon_step * (middle_stop_idx - last_support_idx),
                };
            }
            stops_coords[stops[stop_idx]] = stops_dict.at(stops[stop_idx])->position;
            last_support_idx = stop_idx;
        }
    }
    return stops_coords;
}

static NeighboursDicts BuildCoordNeighboursDicts(const std::unordered_map<std::string, Sphere::Point>& stops_dict, const Descriptions::BusesDict& buses_dict) {
    unordered_map<double, unordered_set<double>> neighbour_lats;
    unordered_map<double, unordered_set<double>> neighbour_lons;
    for (const auto& [bus_name, bus_ptr] : buses_dict) {
        const auto& stops = bus_ptr->stops;
        if (stops.empty()) continue;
        Sphere::Point point_prev = stops_dict.at(stops[0]);
        for (size_t stop_idx = 1; stop_idx < stops.size(); stop_idx++) {
            const auto point_cur = stops_dict.at(stops[stop_idx]);
            const auto [min_lat, max_lat] = minmax(point_prev.latitude, point_cur.latitude);
            const auto [min_lon, max_lon] = minmax(point_prev.longitude, point_cur.longitude);
            neighbour_lats[max_lat].insert(min_lat);
            neighbour_lons[max_lon].insert(min_lon);
            point_prev = point_cur;
        }
    }
    return {move(neighbour_lats), move(neighbour_lons)};
}

static map<string, Svg::Point> ComputeStopsCoords(const Descriptions::StopsDict& stops_dict, const Descriptions::BusesDict& buses_dict, const RenderSettings& render_settings) {
    const auto stops_geo_coords = ComputeInterpolatedStopsGeoCoords(stops_dict, buses_dict);
    const auto [neighbour_lats, neighbour_lons] = BuildCoordNeighboursDicts(stops_geo_coords, buses_dict);
    CoordsCompressor compressor(stops_geo_coords);
    compressor.FillIndices(neighbour_lats, neighbour_lons);
    compressor.FillTargets(render_settings.max_width, render_settings.max_height, render_settings.padding);

    map<string, Svg::Point> stops_coords;
    for (const auto& [stop_name, stop_coord] : stops_geo_coords) {
        stops_coords[stop_name] = {compressor.MapLon(stop_coord.longitude), compressor.MapLat(stop_coord.latitude)};
    }
    return stops_coords;
}

static map<std::string, Svg::Color> ChooseBusColors(const Descriptions::BusesDict& buses_, const RenderSettings& render_settings) {
    const auto palette = render_settings.palette;
    map<std::string, Svg::Color> bus_colors_;
    int idx = 0;
    for (const auto& [bus_name, bus_ptr] : buses_) {
        bus_colors_[bus_name] = palette[idx++ % palette.size()];
    }

    return bus_colors_;
}

RenderSettings ParseRenderSettings(const Json::Dict& json) {
    RenderSettings result;
    result.max_width = json.at("width").AsDouble();
    result.max_height = json.at("height").AsDouble();
    result.padding = json.at("padding").AsDouble();
    result.stop_radius = json.at("stop_radius").AsDouble();
    result.line_width = json.at("line_width").AsDouble();
    result.underlayer_width = json.at("underlayer_width").AsDouble();
    result.stop_label_font_size = json.at("stop_label_font_size").AsInt();
    result.bus_label_font_size = json.at("bus_label_font_size").AsInt();
    result.stop_label_offset = ParsePoint(json.at("stop_label_offset"));
    result.bus_label_offset = ParsePoint(json.at("bus_label_offset"));
    result.underlayer_color = ParseColor(json.at("underlayer_color"));
    result.palette = ParsePalette(json.at("color_palette"));
    result.layers  = ParseLayers(json.at("layers"));

    return result;
}

void MapRenderer::RenderBusLines(Svg::Document& svg) const {
    for (const auto& [bus_name, bus_ptr] : buses_dict_) {
        const auto& stops = bus_ptr->stops;
        if (stops.empty()) {
            continue;
        }
        Svg::Polyline line;
        line.SetStrokeColor(buses_colors_.at(bus_name))
            .SetStrokeWidth(render_settings_.line_width)
            .SetStrokeLineCap("round").SetStrokeLineJoin("round");
        for (const auto& stop_name : stops) {
            line.AddPoint(stops_coords_.at(stop_name));
        }
        svg.Add(line);
    }
}

void MapRenderer::RenderBusLabels(Svg::Document& svg) const {
    for (const auto& [bus_name, bus_ptr] : buses_dict_) {
        const auto& stops = bus_ptr->stops;
        if (stops.empty()) return;
        const auto& color = buses_colors_.at(bus_name);
        for (const string& endpoint : bus_ptr->endpoints) {
            const auto point = stops_coords_.at(endpoint);
            const auto base_text = Svg::Text{}
                .SetPoint(point)
                .SetOffset(render_settings_.bus_label_offset)
                .SetFontSize(render_settings_.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus_name);
            svg.Add(Svg::Text(base_text)
                .SetFillColor(render_settings_.underlayer_color)
                .SetStrokeColor(render_settings_.underlayer_color)
                .SetStrokeWidth(render_settings_.underlayer_width)
                .SetStrokeLineCap("round").SetStrokeLineJoin("round"));
            svg.Add(Svg::Text(base_text)
                .SetFillColor(color)
                .SetStrokeColor("none")
                .SetStrokeWidth(1));
        }
    }

}

void MapRenderer::RenderStopsPoints(Svg::Document& svg) const {
    for (const auto& [stop_name, stop_point] : stops_coords_){
        svg.Add(Svg::Circle{}
        .SetCenter(stop_point)
        .SetRadius(render_settings_.stop_radius)
        .SetFillColor("white"));
    }
}

void MapRenderer::RenderStopLabels(Svg::Document& svg) const {
    for (const auto& [stop_name, stop_point] : stops_coords_) {
        const auto base_text = Svg::Text{}
                .SetPoint(stop_point)
                .SetOffset(render_settings_.stop_label_offset)
                .SetFontSize(render_settings_.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(stop_name);

        svg.Add(Svg::Text(base_text)
                .SetFillColor(render_settings_.underlayer_color)
                .SetStrokeColor(render_settings_.underlayer_color)
                .SetStrokeWidth(render_settings_.underlayer_width)
                .SetStrokeLineCap("round").SetStrokeLineJoin("round")
        );
        svg.Add(Svg::Text(base_text)
                .SetFillColor("black")
                .SetStrokeColor("none")
                .SetStrokeWidth(1));
    }
}

MapRenderer::MapRenderer(const Descriptions::StopsDict& stops_, const Descriptions::BusesDict& buses_, const Json::Dict& render_settings_json_) :
        render_settings_(ParseRenderSettings(render_settings_json_)), buses_dict_(buses_), 
        stops_coords_(ComputeStopsCoords(stops_, buses_, ParseRenderSettings(render_settings_json_))),
        buses_colors_(ChooseBusColors(buses_, ParseRenderSettings(render_settings_json_))) {
}

const std::unordered_map<std::string, void(MapRenderer::*)(Svg::Document&) const> MapRenderer::LAYERS_ACTIONS = {
        {"bus_lines", &MapRenderer::RenderBusLines},
        {"bus_labels", &MapRenderer::RenderBusLabels},
        {"stop_points", &MapRenderer::RenderStopsPoints},
        {"stop_labels", &MapRenderer::RenderStopLabels}
};

Svg::Document MapRenderer::Render() const {
    Svg::Document svg;

    for (const auto& layer : render_settings_.layers) {
        (this->*LAYERS_ACTIONS.at(layer))(svg);
    }

    return svg;
}

CoordsCompressor::CoordsCompressor(const std::unordered_map<std::string, Sphere::Point>& stops_dict){
    for (const auto& [_, stop_coord] : stops_dict) {
        lats_.push_back({stop_coord.latitude});
        lons_.push_back({stop_coord.longitude});
    }
    sort(begin(lats_), end(lats_));
    sort(begin(lons_), end(lons_));
}

void CoordsCompressor::FillTargets(const double& max_width, const double& max_height, const double& padding){
    if (lats_.empty() || lons_.empty()) return;

    const size_t max_lat_idx = lats_.back().idx;
    const double y_step = max_lat_idx ? (max_height - 2 * padding) / max_lat_idx : 0;

    const size_t max_lon_idx = lons_.back().idx;
    const double x_step = max_lon_idx ? (max_width - 2 * padding) / max_lon_idx : 0;

    for (auto& [_, idx, value] : lats_) {
        value = max_height - padding - idx * y_step;
    }
    for (auto& [_, idx, value] : lons_) {
        value = padding + idx * x_step;
    }
}

void CoordsCompressor::FillCoordIndices(std::vector<CoordInfo>& coords, const std::unordered_map<double, std::unordered_set<double>>& neighbour_values) {
    size_t global_idx = 0;
    for (size_t coord_idx = 0; coord_idx < coords.size(); coord_idx++) {
        CoordInfo& coord = coords[coord_idx];
        const auto* neighbours_ptr = GetValuePointer(neighbour_values, coord.source);
        size_t prev_idx = coord_idx;
        while (prev_idx > 0 && coords[prev_idx - 1].idx == global_idx) {
            prev_idx--;
            if (neighbours_ptr && neighbours_ptr->count(coords[prev_idx].source)) {
                global_idx++;
                break;
            }
        }
        coord.idx = global_idx;
    }
}
