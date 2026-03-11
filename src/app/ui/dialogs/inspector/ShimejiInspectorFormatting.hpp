//
// Shijima-Qt - Cross-platform shimeji simulation app for desktop
// Copyright (C) 2025 pixelomer
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#pragma once

#include <QPoint>
#include <string>
#include <shijima/mascot/environment.hpp>
#include <shijima/math.hpp>

namespace ShimejiInspectorFormatting {

inline std::string doubleToString(double value) {
    auto str = std::to_string(value);
    auto dot = str.rfind('.');
    if (dot != std::string::npos) {
        str = str.substr(0, dot + 3);
    }
    return str;
}

inline std::string vecToString(shijima::math::vec2 const& vec) {
    return "x: " + doubleToString(vec.x) +
        ", y: " + doubleToString(vec.y);
}

inline std::string vecToString(QPoint const& vec) {
    return "x: " + doubleToString(vec.x()) +
        ", y: " + doubleToString(vec.y());
}

inline std::string areaToString(shijima::mascot::environment::area const& area) {
    return "x: " + doubleToString(area.left) +
        ", y: " + doubleToString(area.top) +
        ", width: " + doubleToString(area.width()) +
        ", height: " + doubleToString(area.height());
}

inline std::string areaToString(shijima::mascot::environment::darea const& area) {
    return "x: " + doubleToString(area.left) +
        ", y: " + doubleToString(area.top) +
        ", width: " + doubleToString(area.width()) +
        ", height: " + doubleToString(area.height()) +
        ", dx: " + doubleToString(area.dx) +
        ", dy: " + doubleToString(area.dy);
}

}
