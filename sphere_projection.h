#pragma once

#include "sphere.h"
#include "svg.h"
#include "utils.h"
#include <algorithm>
#include <cmath>
#include <optional>

namespace Sphere {

class Projector {
public:
    template<typename PointInputIt>
    Projector(PointInputIt points_begin, PointInputIt points_end, const double& max_width, const double& max_height, const double& padding);

    Svg::Point operator()(Point point) const;

private:
    double min_lon_;
    double max_lat_;
    const double padding_;
    double zoom_coef_;
};

template<typename PointInputIt>
Projector::Projector(PointInputIt points_begin, PointInputIt points_end, const double& max_width, const double& max_height, const double& padding) : padding_(padding) {
    auto min_lat = std::min_element(points_begin, points_end, 
        [](const Point& p1, const Point& p2) { return p1.latitude < p2.latitude; });
    auto min_lon = std::min_element(points_begin, points_end,
        [](const Point& p1, const Point& p2) { return p1.longitude < p2.longitude; });
    auto max_lat = std::max_element(points_begin, points_end,
        [](const Point& p1, const Point& p2) { return p1.latitude < p2.latitude; });
    auto max_lon = std::max_element(points_begin, points_end,
        [](const Point& p1, const Point& p2) { return p1.longitude < p2.longitude; });
    min_lon_ = (*min_lon).longitude;
    max_lat_ = (*max_lat).latitude;

    const auto coef_lon = (*max_lon).longitude - (*min_lon).longitude;
    const auto coef_lat = (*max_lat).latitude - (*min_lat).latitude;

    if (coef_lon == 0 && coef_lat == 0) zoom_coef_ = 0;
    else if (coef_lon == 0) zoom_coef_ = (max_height - 2 * padding) / coef_lat;
    else if (coef_lat == 0) zoom_coef_ = (max_width - 2 * padding) / coef_lon;
    else {zoom_coef_ = std::min((max_width - 2 * padding) / coef_lon, (max_height - 2 * padding) / coef_lat);}
}
}