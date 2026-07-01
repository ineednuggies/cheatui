#pragma once
#include "Color.hpp"
#include <string>

namespace cui {

struct Rect {
    float x, y, w, h;
    bool Contains(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

// Everything a widget needs to draw itself. Implement this once on top of
// whatever you already render with (Win32+GDI, D3D, OpenGL, an existing
// ImGui draw list...) and the rest of the library doesn't need to change.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void FillRect(float x, float y, float w, float h, Color c) = 0;
    virtual void FillRoundedRect(float x, float y, float w, float h, float radius, Color c) = 0;
    virtual void StrokeRect(float x, float y, float w, float h, Color c, float thickness = 1.0f) = 0;
    virtual void FillCircle(float cx, float cy, float radius, Color c) = 0;
    virtual void Line(float x0, float y0, float x1, float y1, Color c, float thickness = 1.0f) = 0;
    virtual void Text(float x, float y, const std::string& text, Color c, float scale = 1.0f) = 0;
    virtual float TextWidth(const std::string& text, float scale = 1.0f) const = 0;

    // Optional scissor/clip support - default no-op for simple backends.
    virtual void PushClip(float x, float y, float w, float h) { (void)x; (void)y; (void)w; (void)h; }
    virtual void PopClip() {}
};

} // namespace cui
