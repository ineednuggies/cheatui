#include "Canvas.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <cstring>

namespace cui {

namespace {
// How much a 1-pixel-wide strip [p, p+1) overlaps a [rangeStart, rangeEnd)
// span. Used to fade a shape's straight edges in over their last fraction
// of a pixel instead of snapping on/off - this alone removes most of the
// "jagged pixel art" look from moving/animated bars and panels.
float AxisCoverage(float pixelStart, float rangeStart, float rangeEnd) {
    float overlap = std::min(pixelStart + 1.0f, rangeEnd) - std::max(pixelStart, rangeStart);
    return std::clamp(overlap, 0.0f, 1.0f);
}
} // namespace

Canvas::Canvas(int width, int height) : width_(width), height_(height) {
    pixels_.assign(static_cast<size_t>(width_) * height_ * 3, 0);
    clipStack_.push_back({0, 0, width_, height_});
}

void Canvas::Clear(Color c) {
    for (int y = 0; y < height_; ++y)
        for (int x = 0; x < width_; ++x) {
            size_t i = (static_cast<size_t>(y) * width_ + x) * 3;
            pixels_[i + 0] = c.r;
            pixels_[i + 1] = c.g;
            pixels_[i + 2] = c.b;
        }
}

Canvas::ClipRect Canvas::CurrentClip() const { return clipStack_.back(); }

void Canvas::PushClip(float x, float y, float w, float h) {
    ClipRect parent = CurrentClip();
    ClipRect c{
        std::max(parent.x0, static_cast<int>(x)),
        std::max(parent.y0, static_cast<int>(y)),
        std::min(parent.x1, static_cast<int>(x + w)),
        std::min(parent.y1, static_cast<int>(y + h))
    };
    clipStack_.push_back(c);
}

void Canvas::PopClip() {
    if (clipStack_.size() > 1) clipStack_.pop_back();
}

void Canvas::PlotBlend(int x, int y, Color c) {
    ClipRect clip = CurrentClip();
    if (x < clip.x0 || x >= clip.x1 || y < clip.y0 || y >= clip.y1) return;
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    size_t i = (static_cast<size_t>(y) * width_ + x) * 3;
    if (c.a >= 255) {
        pixels_[i + 0] = c.r;
        pixels_[i + 1] = c.g;
        pixels_[i + 2] = c.b;
        return;
    }
    if (c.a == 0) return;
    float a = c.a / 255.0f;
    pixels_[i + 0] = static_cast<uint8_t>(c.r * a + pixels_[i + 0] * (1.0f - a));
    pixels_[i + 1] = static_cast<uint8_t>(c.g * a + pixels_[i + 1] * (1.0f - a));
    pixels_[i + 2] = static_cast<uint8_t>(c.b * a + pixels_[i + 2] * (1.0f - a));
}

void Canvas::PlotCoverage(int x, int y, Color c, float coverage) {
    if (coverage <= 0.0f) return;
    if (coverage >= 1.0f) { PlotBlend(x, y, c); return; }
    Color soft = c;
    soft.a = static_cast<uint8_t>(std::clamp(c.a * coverage, 0.0f, 255.0f));
    PlotBlend(x, y, soft);
}

float Canvas::RoundedRectCoverage(float lx, float ly, float w, float h, Corners r) {
    float radius = 0.0f, dx = 0.0f, dy = 0.0f;
    bool inCorner = true;
    if (lx < r.tl && ly < r.tl)                    { radius = r.tl; dx = r.tl - lx;       dy = r.tl - ly; }
    else if (lx > w - r.tr && ly < r.tr)            { radius = r.tr; dx = lx - (w - r.tr); dy = r.tr - ly; }
    else if (lx > w - r.br && ly > h - r.br)        { radius = r.br; dx = lx - (w - r.br); dy = ly - (h - r.br); }
    else if (lx < r.bl && ly > h - r.bl)            { radius = r.bl; dx = r.bl - lx;       dy = ly - (h - r.bl); }
    else inCorner = false;

    if (!inCorner || radius <= 0.0f) return inCorner ? 0.0f : 1.0f;
    float dist = std::sqrt(dx * dx + dy * dy);
    // Fully opaque half a pixel inside the arc, fully transparent half a
    // pixel outside it, smooth in between - a soft-edged corner instead of
    // a staircase of square pixels.
    return std::clamp(radius - dist + 0.5f, 0.0f, 1.0f);
}

void Canvas::FillRoundedRectExImpl(float x, float y, float w, float h, Corners r,
                                    Color top, Color bottom, bool gradient) {
    float maxR = std::min(w, h) * 0.5f;
    r.tl = std::min(r.tl, maxR); r.tr = std::min(r.tr, maxR);
    r.br = std::min(r.br, maxR); r.bl = std::min(r.bl, maxR);

    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = static_cast<int>(std::ceil(x + w));
    int y1 = static_cast<int>(std::ceil(y + h));
    for (int yy = y0; yy < y1; ++yy) {
        float py = yy + 0.5f;
        float covY = AxisCoverage(static_cast<float>(yy), y, y + h);
        if (covY <= 0.0f) continue;
        Color rowColor = top;
        if (gradient) {
            float t = h > 0.0f ? std::clamp((py - y) / h, 0.0f, 1.0f) : 0.0f;
            rowColor = Color::Lerp(top, bottom, t);
        }
        for (int xx = x0; xx < x1; ++xx) {
            float px = xx + 0.5f;
            float covX = AxisCoverage(static_cast<float>(xx), x, x + w);
            float corner = RoundedRectCoverage(px - x, py - y, w, h, r);
            PlotCoverage(xx, yy, rowColor, covX * covY * corner);
        }
    }
}

void Canvas::FillRect(float x, float y, float w, float h, Color c) {
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = static_cast<int>(std::ceil(x + w));
    int y1 = static_cast<int>(std::ceil(y + h));
    for (int yy = y0; yy < y1; ++yy) {
        float covY = AxisCoverage(static_cast<float>(yy), y, y + h);
        if (covY <= 0.0f) continue;
        for (int xx = x0; xx < x1; ++xx) {
            float covX = AxisCoverage(static_cast<float>(xx), x, x + w);
            PlotCoverage(xx, yy, c, covX * covY);
        }
    }
}

void Canvas::FillRoundedRect(float x, float y, float w, float h, float radius, Color c) {
    FillRoundedRectExImpl(x, y, w, h, Corners(radius), c, c, false);
}

void Canvas::FillRoundedRectGradient(float x, float y, float w, float h, float radius,
                                      Color top, Color bottom) {
    FillRoundedRectExImpl(x, y, w, h, Corners(radius), top, bottom, true);
}

void Canvas::FillRoundedRectEx(float x, float y, float w, float h, Corners r, Color c) {
    FillRoundedRectExImpl(x, y, w, h, r, c, c, false);
}

void Canvas::FillRoundedRectExGradient(float x, float y, float w, float h, Corners r,
                                        Color top, Color bottom) {
    FillRoundedRectExImpl(x, y, w, h, r, top, bottom, true);
}

void Canvas::StrokeRect(float x, float y, float w, float h, Color c, float thickness) {
    FillRect(x, y, w, thickness, c);
    FillRect(x, y + h - thickness, w, thickness, c);
    FillRect(x, y, thickness, h, c);
    FillRect(x + w - thickness, y, thickness, h, c);
}

void Canvas::FillCircle(float cx, float cy, float radius, Color c) {
    int x0 = static_cast<int>(std::floor(cx - radius));
    int y0 = static_cast<int>(std::floor(cy - radius));
    int x1 = static_cast<int>(std::ceil(cx + radius));
    int y1 = static_cast<int>(std::ceil(cy + radius));
    for (int yy = y0; yy <= y1; ++yy) {
        float py = yy + 0.5f;
        for (int xx = x0; xx <= x1; ++xx) {
            float px = xx + 0.5f;
            float dist = std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
            float coverage = std::clamp(radius - dist + 0.5f, 0.0f, 1.0f);
            PlotCoverage(xx, yy, c, coverage);
        }
    }
}

void Canvas::Line(float x0, float y0, float x1, float y1, Color c, float thickness) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.0001f) { FillCircle(x0, y0, thickness * 0.5f, c); return; }
    int steps = static_cast<int>(len) + 1;
    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / steps;
        FillCircle(x0 + dx * t, y0 + dy * t, thickness * 0.5f, c);
    }
}

void Canvas::Text(float x, float y, const std::string& text, Color c, float scale) {
    const auto& table = strokefont::Table();
    float penX = x;
    float thickness = std::max(1.0f, scale * 0.85f);
    for (char ch : text) {
        char key = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        auto it = table.find(key);
        if (it == table.end()) it = table.find(' ');
        const auto& glyph = it->second;
        for (const auto& stroke : glyph) {
            for (size_t i = 0; i + 1 < stroke.size(); ++i) {
                float x0 = penX + stroke[i].x * scale;
                float y0 = y + stroke[i].y * scale;
                float x1 = penX + stroke[i + 1].x * scale;
                float y1 = y + stroke[i + 1].y * scale;
                Line(x0, y0, x1, y1, c, thickness);
            }
        }
        penX += 6 * scale; // 5-unit glyph + 1-unit spacing
    }
}

float Canvas::TextWidth(const std::string& text, float scale) const {
    return static_cast<float>(text.size()) * 6.0f * scale;
}

bool Canvas::SaveRaw(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write("CUIRAW1", 7);
    int32_t w = width_, h = height_;
    f.write(reinterpret_cast<const char*>(&w), sizeof(w));
    f.write(reinterpret_cast<const char*>(&h), sizeof(h));
    f.write(reinterpret_cast<const char*>(pixels_.data()), static_cast<std::streamsize>(pixels_.size()));
    return true;
}

} // namespace cui
