#pragma once
#include "Theme.hpp"

// A handful of ready-made palettes. Pass one to Menu::SetTheme() for an
// instant (and, by default, smoothly cross-fading) re-theme:
//
//   menu.SetTheme(cui::themes::Crimson());
//
// Or use one as a starting point and tweak individual fields from there:
//
//   Theme t = cui::themes::Ocean();
//   t.cornerRadius = 4.0f;
//   menu.SetTheme(t);

namespace cui::themes {

// The look Theme{} already defaults to - included here so it has a name
// alongside the others and can be switched back to.
inline Theme Default() { return Theme{}; }

inline Theme Midnight() {
    Theme t;
    t.background = Color::FromHex(0x07080C, 235);
    t.sidebar = Color::FromHex(0x0A0C11, 245);
    t.panelAlt = Color::FromHex(0x10131B, 255);
    t.border = Color::FromHex(0x1D2230, 255);
    t.text = Color::FromHex(0xE7E9EE, 255);
    t.textDim = Color::FromHex(0x767C8C, 255);
    t.accent = Color::FromHex(0x4C8DFF, 255);
    t.accentDim = Color::FromHex(0x4C8DFF, 90);
    t.accentSoft = Color::FromHex(0x82B0FF, 255);
    t.toggleOff = Color::FromHex(0x232838, 255);
    return t;
}

inline Theme Crimson() {
    Theme t;
    t.background = Color::FromHex(0x120A0C, 235);
    t.sidebar = Color::FromHex(0x160C0E, 245);
    t.panelAlt = Color::FromHex(0x1E1215, 255);
    t.border = Color::FromHex(0x33191D, 255);
    t.text = Color::FromHex(0xF1E6E7, 255);
    t.textDim = Color::FromHex(0x9C7F82, 255);
    t.accent = Color::FromHex(0xFF4C5E, 255);
    t.accentDim = Color::FromHex(0xFF4C5E, 90);
    t.accentSoft = Color::FromHex(0xFF8B96, 255);
    t.toggleOff = Color::FromHex(0x33191D, 255);
    return t;
}

inline Theme Emerald() {
    Theme t;
    t.background = Color::FromHex(0x08110E, 235);
    t.sidebar = Color::FromHex(0x0A1512, 245);
    t.panelAlt = Color::FromHex(0x101E19, 255);
    t.border = Color::FromHex(0x1C3229, 255);
    t.text = Color::FromHex(0xE7EEEA, 255);
    t.textDim = Color::FromHex(0x7C9C8C, 255);
    t.accent = Color::FromHex(0x35D68C, 255);
    t.accentDim = Color::FromHex(0x35D68C, 90);
    t.accentSoft = Color::FromHex(0x7CE8B4, 255);
    t.toggleOff = Color::FromHex(0x1C3229, 255);
    return t;
}

inline Theme Ocean() {
    Theme t;
    t.background = Color::FromHex(0x061014, 235);
    t.sidebar = Color::FromHex(0x08151A, 245);
    t.panelAlt = Color::FromHex(0x0D1E24, 255);
    t.border = Color::FromHex(0x1A343C, 255);
    t.text = Color::FromHex(0xE6EEF1, 255);
    t.textDim = Color::FromHex(0x74959E, 255);
    t.accent = Color::FromHex(0x2BD1D6, 255);
    t.accentDim = Color::FromHex(0x2BD1D6, 90);
    t.accentSoft = Color::FromHex(0x7DEBEE, 255);
    t.toggleOff = Color::FromHex(0x1A343C, 255);
    return t;
}

inline Theme Sunset() {
    Theme t;
    t.background = Color::FromHex(0x140D08, 235);
    t.sidebar = Color::FromHex(0x18100A, 245);
    t.panelAlt = Color::FromHex(0x21160D, 255);
    t.border = Color::FromHex(0x3A2717, 255);
    t.text = Color::FromHex(0xF1EAE2, 255);
    t.textDim = Color::FromHex(0xA1876F, 255);
    t.accent = Color::FromHex(0xFF9A4C, 255);
    t.accentDim = Color::FromHex(0xFF9A4C, 90);
    t.accentSoft = Color::FromHex(0xFFC08B, 255);
    t.toggleOff = Color::FromHex(0x3A2717, 255);
    return t;
}

} // namespace cui::themes
