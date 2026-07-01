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
    int keyPressed = 0;
};

} // namespace cui
