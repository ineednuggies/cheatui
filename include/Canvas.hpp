#pragma once
#include "Renderer.hpp"
#include "StrokeFont.hpp"
#include <vector>
#include <cstdint>
#include <string>

namespace cui {

// A small CPU rasterizer. It's here so the library builds and runs without
// pulling in a real graphics API - swap it for a Win32/D3D/OpenGL
// IRenderer in an actual application; nothing else changes.
class Canvas : public IRenderer {
public:
    Canvas(int width, int height);

    void Clear(Color c);

    void FillRect(float x, float y, float w, float h, Color c) override;
    void FillRoundedRect(float x, float y, float w, float h, float radius, Color c) override;
    void FillRoundedRectGradient(float x, float y, float w, float h, float radius,
                                  Color top, Color bottom) override;
    void FillRoundedRectEx(float x, float y, float w, float h, Corners r, Color c) override;
    void FillRoundedRectExGradient(float x, float y, float w, float h, Corners r,
                                    Color top, Color bottom) override;
    void StrokeRect(float x, float y, float w, float h, Color c, float thickness) override;
    void FillCircle(float cx, float cy, float radius, Color c) override;
    void Line(float x0, float y0, float x1, float y1, Color c, float thickness) override;
    void Text(float x, float y, const std::string& text, Color c, float scale) override;
    float TextWidth(const std::string& text, float scale) const override;

    void PushClip(float x, float y, float w, float h) override;
    void PopClip() override;

    int Width() const { return width_; }
    int Height() const { return height_; }

    // Writes a tiny custom raw format: "CUIRAW" magic, width, height, then
    // tightly packed RGB8 bytes. A companion Python script (tools/raw_to_png.py)
    // converts this to a real PNG using only the Python standard library
    // (zlib), so no third-party C++ image library is required to inspect output.
    bool SaveRaw(const std::string& path) const;

private:
    void PlotBlend(int x, int y, Color c); // alpha-blends a single pixel, clip-aware
    void PlotCoverage(int x, int y, Color c, float coverage); // same, with a soft edge multiplier

    // Coverage (0..1) of a point against a rounded-rect's corner curve, one
    // independent radius per corner; 1.0 everywhere except the last ~1px
    // band around each corner, where it fades out smoothly instead of
    // stair-stepping. This one function is what takes the whole UI from
    // "pixel art" to "soft edges everywhere".
    static float RoundedRectCoverage(float lx, float ly, float w, float h, Corners r);

    // Shared inner loop for both the uniform and per-corner rounded-rect
    // fills (flat or gradient) so there's exactly one place that does the
    // pixel-coverage math.
    void FillRoundedRectExImpl(float x, float y, float w, float h, Corners r,
                                Color top, Color bottom, bool gradient);

    int width_, height_;
    std::vector<uint8_t> pixels_; // RGB8, row-major

    struct ClipRect { int x0, y0, x1, y1; };
    std::vector<ClipRect> clipStack_;
    ClipRect CurrentClip() const;
};

} // namespace cui
