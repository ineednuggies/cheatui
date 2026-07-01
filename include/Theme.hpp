#pragma once
#include "Color.hpp"

namespace cui {

// Tweak these and the whole menu re-themes. Defaults are a typical dark
// overlay-menu look: near-black panels, one accent color, soft borders.
struct Theme {
    Color background   = Color::FromHex(0x0B0D12, 235); // main window panel
    Color sidebar       = Color::FromHex(0x0E1117, 245); // tab rail
    Color panelAlt       = Color::FromHex(0x141823, 255); // widget rows
    Color border         = Color::FromHex(0x23283A, 255);
    Color text           = Color::FromHex(0xE7E9EE, 255);
    Color textDim        = Color::FromHex(0x8A90A2, 255);
    Color accent         = Color::FromHex(0x7C5CFF, 255); // primary accent
    Color accentDim      = Color::FromHex(0x7C5CFF, 90);
    Color toggleOff      = Color::FromHex(0x2A2F3F, 255);
    Color danger         = Color::FromHex(0xFF5C5C, 255);
    Color success        = Color::FromHex(0x4CD27A, 255);

    float cornerRadius   = 6.0f;
    float padding        = 10.0f;
    float rowHeight      = 28.0f;
    float tabWidth       = 130.0f;
    float animDuration   = 0.16f; // seconds, used by most hover/click fades

    bool rainbowAccent   = false; // accent color hue-cycles instead of staying fixed
};

inline Theme& DefaultTheme() {
    static Theme theme;
    return theme;
}

} // namespace cui
