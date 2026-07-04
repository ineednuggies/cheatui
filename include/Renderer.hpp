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

// Radii for each of a rect's four corners, in top-left, top-right,
// bottom-right, bottom-left order (clockwise from top-left). Used to round
// only the corners that actually sit on a window's outer edge - a panel
// flush against another panel on one side should have square corners
// there, or you get a visible notch where the two meet.
struct Corners {
    float tl, tr, br, bl;
    explicit Corners(float all) : tl(all), tr(all), br(all), bl(all) {}
    Corners(float tl_, float tr_, float br_, float bl_) : tl(tl_), tr(tr_), br(br_), bl(bl_) {}

    static Corners Top(float r)    { return Corners(r, r, 0, 0); }
    static Corners Bottom(float r) { return Corners(0, 0, r, r); }
    static Corners Left(float r)   { return Corners(r, 0, 0, r); }
    static Corners Right(float r)  { return Corners(0, r, r, 0); }
    static Corners TopLeft(float r)     { return Corners(r, 0, 0, 0); }
    static Corners TopRight(float r)    { return Corners(0, r, 0, 0); }
    static Corners BottomRight(float r) { return Corners(0, 0, r, 0); }
    static Corners BottomLeft(float r)  { return Corners(0, 0, 0, r); }
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

    // Vertical gradient fill (top color -> bottom color), used for the
    // glassy bevel look on panels, buttons, and toggles. Backends that
    // can't easily do a true gradient can leave this as-is; the default
    // just averages the two colors and falls back to a flat fill, so
    // existing IRenderer implementations don't break when this was added.
    virtual void FillRoundedRectGradient(float x, float y, float w, float h, float radius,
                                          Color top, Color bottom) {
        FillRoundedRect(x, y, w, h, radius, Color::Lerp(top, bottom, 0.5f));
    }

    // Same as FillRoundedRect/FillRoundedRectGradient but each corner gets
    // its own radius (0 = square). The default falls back to rounding all
    // four corners the same amount, so this is safe to leave unoverridden.
    virtual void FillRoundedRectEx(float x, float y, float w, float h, Corners r, Color c) {
        float avg = (r.tl + r.tr + r.br + r.bl) * 0.25f;
        FillRoundedRect(x, y, w, h, avg, c);
    }
    virtual void FillRoundedRectExGradient(float x, float y, float w, float h, Corners r,
                                            Color top, Color bottom) {
        float avg = (r.tl + r.tr + r.br + r.bl) * 0.25f;
        FillRoundedRectGradient(x, y, w, h, avg, top, bottom);
    }

    // Optional scissor/clip support - default no-op for simple backends.
    virtual void PushClip(float x, float y, float w, float h) { (void)x; (void)y; (void)w; (void)h; }
    virtual void PopClip() {}
};

} // namespace cui
