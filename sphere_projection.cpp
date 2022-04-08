#include "sphere_projection.h"

namespace Sphere {
    Svg::Point Projector::operator() (Point point) const {
        return { (point.longitude - min_lon_)*zoom_coef_ + padding_, (max_lat_ - point.latitude)*zoom_coef_ + padding_};
    }
}
