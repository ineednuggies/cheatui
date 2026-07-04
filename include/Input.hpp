#pragma once

namespace cui {

// Fill one of these in per frame (mouse position, button state, the last
// key pressed) and hand it to Menu::Update(). Works the same regardless of
// whether your input comes from Win32 messages, GLFW callbacks, SDL events,
// or anything else.
struct InputState {
    float mouseX = 0.0f, mouseY = 0.0f;
    bool mouseDown = false;      // currently held
    bool mousePressed = false;   // went down this frame
    bool mouseReleased = false;  // went up this frame
    float scrollDelta = 0.0f;

    // For keybind widgets: last key code pressed this frame, or 0 if none.
    // This is the raw platform key code (whatever VK_/SDL_SCANCODE_/etc.
    // your backend uses) - Keybind just stores and compares it, it never
    // needs to know what the number means.
    int keyPressed = 0;

    // For text-entry widgets (like clicking a slider's value to type a
    // number): the actual typed character this frame, already translated
    // to ASCII (e.g. from a Win32 WM_CHAR message or an SDL text-input
    // event), or 0 if nothing was typed. Kept separate from keyPressed
    // because that one carries raw, non-portable key codes meant for
    // Keybind, not characters meant for typing.
    char textChar = 0;
    bool backspace = false;  // backspace pressed this frame
    bool enterPressed = false;
    bool escapePressed = false;
};

} // namespace cui
