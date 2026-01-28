#pragma once

struct Color {
    unsigned char r, g, b;

    Color operator*(float t) const; 
    Color operator+(const Color& other) const;
};