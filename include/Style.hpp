#pragma once
#include "Renderer.hpp"
#include "Theme.hpp"
#include <algorithm>

namespace cui::style {

// Soft glow halo behind a shape, built out of a handful of expanding,
// increasingly transparent rounded rects rather than a real blur - cheap,
// backend-agnostic (only needs FillRoundedRect), and reads fine at UI
// scale. strength scales the whole effect; pass theme.glowStrength for
// the normal "how glowy is this theme" knob, or something animated
// (a hover fade, a pulse) for a glow that breathes with the widget.
inline void Glow(IRenderer& r, Rect rect, float cornerRadius, Color color, float strength) {
    if (strength <= 0.001f) return;
    const int layers = 4;
    const float spread = 7.0f; // how far the glow reaches past the shape, in pixels
    for (int i = layers; i >= 1; --i) {
        float t = static_cast<float>(i) / layers;
        float grow = spread * t;
        uint8_t alpha = static_cast<uint8_t>(std::clamp(color.a * strength * (1.0f - t) * 0.35f, 0.0f, 255.0f));
        Color layerColor(color.r, color.g, color.b, alpha);
        r.FillRoundedRect(rect.x - grow, rect.y - grow, rect.w + grow * 2.0f, rect.h + grow * 2.0f,
                           cornerRadius + grow, layerColor);
    }
}

// Same idea, centered on a point - for glows behind round elements like a
// slider thumb or a toggle knob rather than a rectangular panel.
inline void GlowCircle(IRenderer& r, float cx, float cy, float radius, Color color, float strength) {
    if (strength <= 0.001f) return;
    const int layers = 4;
    const float spread = 6.0f;
    for (int i = layers; i >= 1; --i) {
        float t = static_cast<float>(i) / layers;
        float grow = spread * t;
        uint8_t alpha = static_cast<uint8_t>(std::clamp(color.a * strength * (1.0f - t) * 0.35f, 0.0f, 255.0f));
        r.FillCircle(cx, cy, radius + grow, Color(color.r, color.g, color.b, alpha));
    }
}

// A panel with a subtle light-top / dark-bottom gradient plus a hairline
// highlight along the top edge - the "soft bevel" look, built from a
// gradient fill and two thin lines rather than needing real pixel-level
// lighting. strength is theme.bevelStrength by default; 0 gives a flat fill.
inline void BeveledPanel(IRenderer& r, Rect rect, float cornerRadius, Color base, float strength) {
    if (strength <= 0.001f) {
        r.FillRoundedRect(rect.x, rect.y, rect.w, rect.h, cornerRadius, base);
        return;
    }
    Color top = Color::Lerp(base, Color(255, 255, 255, base.a), strength);
    Color bottom = Color::Lerp(base, Color(0, 0, 0, base.a), strength * 0.8f);
    r.FillRoundedRectGradient(rect.x, rect.y, rect.w, rect.h, cornerRadius, top, bottom);

    // a faint 1px highlight just inside the top edge sells the "glassy" look
    Color highlight(255, 255, 255, static_cast<uint8_t>(40 * strength * 3.0f));
    r.Line(rect.x + cornerRadius * 0.5f, rect.y + 1.0f, rect.x + rect.w - cornerRadius * 0.5f, rect.y + 1.0f,
           highlight, 1.0f);
}

} // namespace cui::style
