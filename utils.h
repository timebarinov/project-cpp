#pragma once

#include <string_view>
#include <unordered_map>
#include <sstream>
#include <math.h>
#include <algorithm>
#include <utility>
#define PI  3.1415926535

namespace utils {

    inline void RemoveSpaceLine(std::string_view& line) {

        auto pos = line.find_last_of(' ');

        if (pos != std::string_view::npos && pos == line.size() - 1) {
            line.remove_suffix(line.size() - pos);
        }
    };
    
    inline std::string_view GetPartLine(std::string_view& line, char space = ' ') {
        auto pos = line.find(space);
        std::string_view result = line;

        if (pos != std::string_view::npos) {
            result.remove_suffix(result.size() - pos);
            line.remove_prefix(pos + 1);
        }
        else {
            line.remove_prefix(line.size());
        }
        RemoveSpaceLine(result);
        return result;
    };

    inline std::string_view GetPartLine(std::string_view& line, size_t pos) {
        std::string_view result = line;

        if (pos != std::string_view::npos) {
            result.remove_suffix(result.size() - pos);
            line.remove_prefix(pos + 1);
        }
        else {
            line.remove_prefix(line.size());
        }
        RemoveSpaceLine(result);
        return result;
    };   

    inline long double GetRadian(long double degrees) {
        return degrees * PI / 180.0;
    }

    struct Coordinates {
        Coordinates() = default;

        Coordinates(std::string_view& coords) {
            utils::GetPartLine(coords);
            auto lat = utils::GetPartLine(coords, ',');
            std::stringstream(move(std::string(lat))) >> latitude;

            utils::GetPartLine(coords);
            auto lon = utils::GetPartLine(coords, ',');
            std::stringstream(move(std::string(lon))) >> longitude;

            latitude = utils::GetRadian(latitude);
            longitude = utils::GetRadian(longitude);
        };

        long double latitude = 0.0;
        long double longitude = 0.0;
    };

    inline long double GetLength(const Coordinates& lhs, const Coordinates& rhs) {
        // авторская формула для вычисления расстояния между двумя точками на земной поверхности
        return acos(sin(lhs.latitude) * sin(rhs.latitude) + cos(lhs.latitude) * cos(rhs.latitude) * cos(abs(lhs.longitude - rhs.longitude))) * 6371000.0;
    };

};