#pragma once
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace cui {

// Plain 8-bit RGBA color.
struct Color {
    uint8_t r = 255, g = 255, b = 255, a = 255;

    constexpr Color() = default;
    constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}

    static Color FromHex(uint32_t hex, uint8_t alpha = 255) {
        return Color((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, alpha);
    }

    // Linear interpolation between two colors, t in [0,1].
    static Color Lerp(const Color& a_, const Color& b_, float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        auto mix = [t](uint8_t x, uint8_t y) -> uint8_t {
            return static_cast<uint8_t>(x + (static_cast<float>(y) - static_cast<float>(x)) * t);
        };
        return Color(mix(a_.r, b_.r), mix(a_.g, b_.g), mix(a_.b, b_.b), mix(a_.a, b_.a));
    }

    // HSV -> RGB. h in [0,360), s/v in [0,1]. Used by the color-picker widget.
    static Color FromHSV(float h, float s, float v, uint8_t alpha = 255) {
        float c = v * s;
        float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;
        float rp, gp, bp;
        if (h < 60)       { rp = c; gp = x; bp = 0; }
        else if (h < 120) { rp = x; gp = c; bp = 0; }
        else if (h < 180) { rp = 0; gp = c; bp = x; }
        else if (h < 240) { rp = 0; gp = x; bp = c; }
        else if (h < 300) { rp = x; gp = 0; bp = c; }
        else              { rp = c; gp = 0; bp = x; }
        return Color(
            static_cast<uint8_t>((rp + m) * 255.0f),
            static_cast<uint8_t>((gp + m) * 255.0f),
            static_cast<uint8_t>((bp + m) * 255.0f),
            alpha);
    }

    // RGB -> HSV, used so the picker can show a cursor position for a stored color.
    void ToHSV(float& h, float& s, float& v) const {
        float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
        float maxc = std::max({rf, gf, bf});
        float minc = std::min({rf, gf, bf});
        v = maxc;
        float delta = maxc - minc;
        s = (maxc <= 0.0f) ? 0.0f : (delta / maxc);
        if (delta <= 0.00001f) { h = 0.0f; return; }
        if (maxc == rf)      h = 60.0f * std::fmod(((gf - bf) / delta), 6.0f);
        else if (maxc == gf) h = 60.0f * (((bf - rf) / delta) + 2.0f);
        else                 h = 60.0f * (((rf - gf) / delta) + 4.0f);
        if (h < 0) h += 360.0f;
    }

    uint32_t ToPackedRGBA() const {
        return (static_cast<uint32_t>(r) << 24) | (static_cast<uint32_t>(g) << 16) |
               (static_cast<uint32_t>(b) << 8) | a;
    }
};

} // namespace cui
