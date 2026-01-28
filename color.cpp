#include <algorithm>

#include "color.h"


Color Color::operator*(float t) const {
    return Color{
        static_cast<unsigned char>(std::min(255.0f, t * r)),
        static_cast<unsigned char>(std::min(255.0f, t * g)),
        static_cast<unsigned char>(std::min(255.0f, t * b))
    };
}

Color Color::operator+(const Color& other) const {
    return Color{
        static_cast<unsigned char>(std::min(255, r + other.r)),
        static_cast<unsigned char>(std::min(255, g + other.g)),
        static_cast<unsigned char>(std::min(255, b + other.b))
    };
}

Color operator*(float t, const Color& c) {
    return c * t;
}