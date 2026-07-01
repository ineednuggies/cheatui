#pragma once
#include <cmath>

namespace cui::easing {

// Takes t in [0,1], returns an eased t in [0,1]. Standard Penner-style
// curves, used for hover/click/tab transitions throughout the library.

inline float Linear(float t) { return t; }

inline float QuadIn(float t)  { return t * t; }
inline float QuadOut(float t) { return t * (2.0f - t); }
inline float QuadInOut(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

inline float CubicOut(float t) {
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}

inline float CubicInOut(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

// Slight overshoot - nice for toggle "pop" animations.
inline float BackOut(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float f = t - 1.0f;
    return 1.0f + c3 * f * f * f + c1 * f * f;
}

inline float ExpoOut(float t) {
    return t >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}

} // namespace cui::easing
