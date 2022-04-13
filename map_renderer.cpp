#include "map_renderer.h"
#include "sphere.h"
#include "utils.h"
#include <iterator>

using namespace std;

using RouteBusItem = TransportRouter::RouteInfo::BusItem;
using RouteWaitItem = TransportRouter::RouteInfo::WaitItem;

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
    Svg::Rgb rgb { 
        static_cast<uint8_t>(array[0].AsInt()),
        static_cast<uint8_t>(array[1].AsInt()),
        static_cast<uint8_t>(array[2].AsInt()) 
    };
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
    unordered_map<double, vector<double>> neighbour_lats;
    unordered_map<double, vector<double>> neighbour_lons;
    for (const auto& [bus_name, bus_ptr] : buses_dict) {
        const auto& stops = bus_ptr->stops;
        if (stops.empty()) continue;
        Sphere::Point point_prev = stops_dict.at(stops[0]);
        for (size_t stop_idx = 1; stop_idx < stops.size(); stop_idx++) {
            const auto point_cur = stops_dict.at(stops[stop_idx]);
            if (stops[stop_idx] != stops[stop_idx - 1]) {
                const auto [min_lat, max_lat] = minmax(point_prev.latitude, point_cur.latitude);
                const auto [min_lon, max_lon] = minmax(point_prev.longitude, point_cur.longitude);
                neighbour_lats[max_lat].push_back(min_lat);
                neighbour_lons[max_lon].push_back(min_lon);
            }
            point_prev = point_cur;
        }
    }
    for (auto* neighbours_dict : {&neighbour_lats, &neighbour_lons}) {
        for (auto& [_, values] : *neighbours_dict) {
            sort(begin(values), end(values));
            values.erase(unique(begin(values), end(values)), end(values));
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
    result.outer_margin = json.at("outer_margin").AsDouble();
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

static map<string, Descriptions::Bus> CopyBusesDict(const Descriptions::BusesDict& source) {
    map<string, Descriptions::Bus> target;
    for (const auto& [name, data_ptr] : source) {
        target.emplace(name, *data_ptr);
    }
    return target;
}

void MapRenderer::RenderBusLines(Svg::Document& svg) const {
    for (const auto& [bus_name, bus] : buses_dict_) {
        const auto& stops = bus.stops;
        if (stops.empty()) continue;
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

void MapRenderer::RenderRouteBusLines(Svg::Document& svg, const TransportRouter::RouteInfo& route) const {
    for (const auto& item : route.items) {
        if (!holds_alternative<RouteBusItem>(item)) continue;
        const auto& bus_item = get<RouteBusItem>(item);
        const string& bus_name = bus_item.bus_name;
        const auto& stops = buses_dict_.at(bus_name).stops;
        if (stops.empty()) continue;
        Svg::Polyline line;
        line.SetStrokeColor(buses_colors_.at(bus_name))
            .SetStrokeWidth(render_settings_.line_width)
            .SetStrokeLineCap("round").SetStrokeLineJoin("round");
        for (size_t stop_idx = bus_item.start_stop_idx; stop_idx <= bus_item.finish_stop_idx; stop_idx++) {
            const string& stop_name = stops[stop_idx];
            line.AddPoint(stops_coords_.at(stop_name));
        }
        svg.Add(line);
    }
}

void MapRenderer::RenderBusLabel(Svg::Document& svg, const string& bus_name, const string& stop_name) const {
    const auto& color = buses_colors_.at(bus_name);
    const auto point = stops_coords_.at(stop_name);
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
        .SetStrokeLineCap("round").SetStrokeLineJoin("round")
    );
    svg.Add(Svg::Text(base_text)
        .SetFillColor(color)
    );
}

void MapRenderer::RenderBusLabels(Svg::Document& svg) const {
    for (const auto& [bus_name, bus] : buses_dict_) {
        const auto& stops = bus.stops;
        if (stops.empty()) continue;
        for (const string& endpoint : bus.endpoints) {
            RenderBusLabel(svg, bus_name, endpoint);
        }
    }

}

void MapRenderer::RenderRouteBusLabels(Svg::Document& svg, const TransportRouter::RouteInfo& route) const {
    for (const auto& item : route.items) {
        if (!holds_alternative<RouteBusItem>(item)) continue;
        const auto& bus_item = get<RouteBusItem>(item);
        const string& bus_name = bus_item.bus_name;
        const auto& bus = buses_dict_.at(bus_name);
        const auto& stops = bus.stops;
        if (stops.empty()) continue;
        for (const size_t stop_idx : {bus_item.start_stop_idx, bus_item.finish_stop_idx}) {
            const auto stop_name = stops[stop_idx];
            if (stop_idx == 0 || stop_idx == stops.size() - 1 || 
                find(begin(bus.endpoints), end(bus.endpoints), stop_name) != end(bus.endpoints)) {
                RenderBusLabel(svg, bus_name, stop_name);
            }
        }
    }
}

void MapRenderer::RenderStopPoint(Svg::Document& svg, Svg::Point point) const {
    svg.Add(Svg::Circle{}
            .SetCenter(point)
            .SetRadius(render_settings_.stop_radius)
            .SetFillColor("white"));
}

void MapRenderer::RenderStopPoints(Svg::Document& svg) const {
    for (const auto& [_, stop_point] : stops_coords_) {
        RenderStopPoint(svg, stop_point);
    }
}

void MapRenderer::RenderRouteStopPoints(Svg::Document& svg, const TransportRouter::RouteInfo& route) const {
    for (const auto& item : route.items) {
        if (!holds_alternative<RouteBusItem>(item)) continue;
        const auto& bus_item = get<RouteBusItem>(item);
        const string& bus_name = bus_item.bus_name;
        const auto& stops = buses_dict_.at(bus_name).stops;
        if (stops.empty()) continue;
        for (size_t stop_idx = bus_item.start_stop_idx; stop_idx <= bus_item.finish_stop_idx; stop_idx++) {
            const string& stop_name = stops[stop_idx];
            RenderStopPoint(svg, stops_coords_.at(stop_name));
        }
    }
}

void MapRenderer::RenderStopLabel(Svg::Document& svg, Svg::Point point, const string& name) const {
    auto base_text = Svg::Text{}
        .SetPoint(point)
        .SetOffset(render_settings_.stop_label_offset)
        .SetFontSize(render_settings_.stop_label_font_size)
        .SetFontFamily("Verdana")
        .SetData(name);
    svg.Add(Svg::Text(base_text)
        .SetFillColor(render_settings_.underlayer_color)
        .SetStrokeColor(render_settings_.underlayer_color)
        .SetStrokeWidth(render_settings_.underlayer_width)
        .SetStrokeLineCap("round").SetStrokeLineJoin("round")
    );
    svg.Add(base_text
        .SetFillColor("black")
    );
}

void MapRenderer::RenderStopLabels(Svg::Document& svg) const {
    for (const auto& [stop_name, stop_point] : stops_coords_) {
        RenderStopLabel(svg, stop_point, stop_name);
    }
}

void MapRenderer::RenderRouteStopLabels(Svg::Document& svg, const TransportRouter::RouteInfo& route) const {
    if (route.items.empty()) return;
    for (const auto& item : route.items) {
        if (!holds_alternative<RouteWaitItem>(item)) continue;
        const auto& wait_item = get<RouteWaitItem>(item);
        const string& stop_name = wait_item.stop_name;
        RenderStopLabel(svg, stops_coords_.at(stop_name), stop_name);
    }
    const auto& last_bus_item = get<RouteBusItem>(route.items.back());
    const string& last_stop_name = buses_dict_.at(last_bus_item.bus_name).stops[last_bus_item.finish_stop_idx];
    RenderStopLabel(svg, stops_coords_.at(last_stop_name), last_stop_name);
}

const unordered_map<string, void (MapRenderer::*)(Svg::Document&) const> 
    MapRenderer::MAP_LAYER_ACTIONS = {
    {"bus_lines",   &MapRenderer::RenderBusLines},
    {"bus_labels",  &MapRenderer::RenderBusLabels},
    {"stop_points", &MapRenderer::RenderStopPoints},
    {"stop_labels", &MapRenderer::RenderStopLabels},
};

const unordered_map<string, void (MapRenderer::*)(Svg::Document&, const TransportRouter::RouteInfo&) const> 
    MapRenderer::ROUTE_LAYER_ACTIONS = {
    {"bus_lines",   &MapRenderer::RenderRouteBusLines},
    {"bus_labels",  &MapRenderer::RenderRouteBusLabels},
    {"stop_points", &MapRenderer::RenderRouteStopPoints},
    {"stop_labels", &MapRenderer::RenderRouteStopLabels},
};

MapRenderer::MapRenderer(const Descriptions::StopsDict& stops_, const Descriptions::BusesDict& buses_, const Json::Dict& render_settings_json_) :
        render_settings_(ParseRenderSettings(render_settings_json_)), buses_dict_(CopyBusesDict(buses_)), 
        stops_coords_(ComputeStopsCoords(stops_, buses_, render_settings_)),
        buses_colors_(ChooseBusColors(buses_, render_settings_)) {
}

Svg::Document MapRenderer::Render() const {
    Svg::Document svg;
    for (const auto& layer : render_settings_.layers) {
        (this->*MAP_LAYER_ACTIONS.at(layer))(svg);
    }
    return svg;
}

Svg::Document MapRenderer::RenderRoute(Svg::Document svg, const TransportRouter::RouteInfo& route) const {
    const double outer_margin = render_settings_.outer_margin;
    svg.Add(Svg::Rectangle{}
        .SetFillColor(render_settings_.underlayer_color)
        .SetTopLeftPoint({-outer_margin, -outer_margin})
        .SetBottomRightPoint({
            render_settings_.max_width + outer_margin,
            render_settings_.max_height + outer_margin
        })
    );

    for (const auto& layer : render_settings_.layers) {
        (this->*ROUTE_LAYER_ACTIONS.at(layer))(svg, route);
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

    const size_t max_lat_idx = FindMaxIdx(lats_);
    const double y_step = max_lat_idx ? (max_height - 2 * padding) / max_lat_idx : 0;

    const size_t max_lon_idx = FindMaxIdx(lons_);
    const double x_step = max_lon_idx ? (max_width - 2 * padding) / max_lon_idx : 0;

    for (auto& [_, idx, value] : lats_) {
        value = max_height - padding - idx * y_step;
    }
    for (auto& [_, idx, value] : lons_) {
        value = padding + idx * x_step;
    }
}

void CoordsCompressor::FillCoordIndices(std::vector<CoordInfo>& coords, const std::unordered_map<double, std::vector<double>>& neighbour_values) {
    for (auto coord_it = begin(coords); coord_it != end(coords); coord_it++) {
        const auto neighbours_it = neighbour_values.find(coord_it->source);
        if (neighbours_it == neighbour_values.end()) {
            coord_it->idx = 0;
            continue;
        }
        const auto& neighbours = neighbours_it->second;
        optional<size_t> max_neighbour_idx;
        for (const double value : neighbours) {
            const size_t idx = Find(coords, value, coord_it).idx;
            if (idx > max_neighbour_idx) max_neighbour_idx = idx;
        }
        coord_it->idx = *max_neighbour_idx + 1;
    }
}
