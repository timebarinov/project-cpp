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

static map<string, Svg::Point> ComputeStopsCoords(const Descriptions::StopsDict& stops_dict, const RenderSettings& render_settings) {
    vector<Sphere::Point> points;
    points.reserve(stops_dict.size());
    for(const auto& [_,stop_ptr] : stops_dict) {
        points.push_back(stop_ptr->position);
    }

    const double max_width = render_settings.max_width;
    const double max_height = render_settings.max_height;
    const double padding = render_settings.padding;

    const Sphere::Projector projector(begin(points),end(points), max_width,max_height,padding);

    map<string, Svg::Point> stops_coords;
    for (const auto& [stop_name, stop_ptr] : stops_dict) {
        stops_coords[stop_name] = projector(stop_ptr->position);
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
        stops_coords_(ComputeStopsCoords(stops_, ParseRenderSettings(render_settings_json_))),
        buses_colors_(ChooseBusColors(buses_, ParseRenderSettings(render_settings_json_))) {
}

Svg::Document MapRenderer::Render() const {
    Svg::Document svg;

    RenderBusLines(svg);
    RenderBusLabels(svg);
    RenderStopsPoints(svg);
    RenderStopLabels(svg);

    return svg;
}
