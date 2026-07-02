# Tutorial

This walks through everything from "I just cloned the repo" to "I have a
themed, animated menu wired into my own app." It pairs with
`examples/getting_started.cpp`, which is the code below in runnable form.

## 1. Getting the code into your project

Two ways to do this, pick whichever fits:

**Copy the headers in.** cheatui has no dependencies, so the simplest
option is: copy `include/` and `src/` into your project, add `src/*.cpp`
to your build, and add `include/` to your include path. Done.

**Use it as a CMake subdirectory.**
```sh
git clone https://github.com/yourname/cheatui.git third_party/cheatui
```
```cmake
add_subdirectory(third_party/cheatui)
target_link_libraries(your_app PRIVATE cheatui)
```
`CHEATUI_BUILD_EXAMPLES` defaults to `ON`; set it to `OFF` in your top-level
`CMakeLists.txt` before the `add_subdirectory` call if you don't want the
example executables built alongside your own project.

Either way, one include gets you the whole public API:

```cpp
#include "cheatui.hpp"
```

## 2. Your first menu

```cpp
cui::Menu menu("My App", cui::Rect{60, 60, 520, 360});
```

That's a window: a title (shown in the title bar, which is draggable out
of the box) and a starting position/size. Nothing shows up until you add
at least one tab.

```cpp
cui::Tab& main = menu.AddTab("Main");
```

`AddTab` returns a reference you use to add widgets to that page. Add as
many tabs as you want; they show up as a vertical list on the left with
an animated selection bar that slides to whichever one is active.

## 3. Adding widgets

Every widget's constructor starts with an id string. That id is what ties
the widget to a saved config value later, so give each one something
unique within the menu, like `"combat.auto_block"` rather than just
`"enabled"`.

```cpp
main.Add<cui::Label>("OPTIONS");                                   // section header, not clickable
main.Add<cui::Checkbox>("main.enabled", "Enable feature", true);   // on/off switch
main.Add<cui::Slider>("main.speed", "Speed", 0.0f, 100.0f, 50.0f, 1.0f, "%");
main.Add<cui::Stepper>("main.retries", "Retries", 0, 5, 2);        // -/+ integer control
main.Add<cui::RadioGroup>("main.mode", "Mode", {"A", "B", "C"}, 0);
main.Add<cui::ComboBox>("main.preset", "Preset", {"Default", "Custom"}, 0);
main.Add<cui::ColorPicker>("main.color", "Highlight color", cui::Color::FromHex(0x7C5CFF));
main.Add<cui::Keybind>("main.hotkey", "Hotkey", 0);
main.Add<cui::Button>("main.reset", "Reset to defaults", []{ /* ... */ });
```

Reading a value back is just `.Value()`:

```cpp
if (enabled.Value()) { /* ... */ }
float pct = speedSlider.Value();
```

If you'd rather react the moment something changes instead of polling,
most widgets take a callback:

```cpp
enabled.SetOnChange([](bool v) {
    std::cout << "feature toggled: " << (v ? "on" : "off") << "\n";
});
```

### Which widget for which job

| Widget | Use it for |
|---|---|
| `Checkbox` | any on/off setting |
| `Slider` | a value picked from a continuous or stepped range |
| `Stepper` | a small integer count, adjusted with -/+ |
| `RadioGroup` | 2-4 mutually exclusive choices, all visible at once |
| `ComboBox` | a longer list of choices, tucked into a dropdown |
| `ColorPicker` | picking an RGB/RGBA color |
| `Keybind` | click, then press a key to bind it |
| `Button` | fire-and-forget action |
| `Label` | section header, purely visual |

## 4. Hooking it up to a real window

The library never touches a window system directly - you drive it with a
plain per-frame loop:

```cpp
while (running) {
    cui::InputState input;
    input.mouseX = /* your mouse x */;
    input.mouseY = /* your mouse y */;
    input.mouseDown = /* left button currently held */;
    input.mousePressed = /* left button went down this frame */;
    input.mouseReleased = /* left button went up this frame */;
    input.keyPressed = /* key code pressed this frame, or 0 */;

    menu.Update(input, deltaTimeSeconds);
    menu.Render(yourRenderer);
}
```

`yourRenderer` is anything implementing `cui::IRenderer` - a handful of
draw calls (`FillRect`, `FillRoundedRect`, `FillCircle`, `Line`, `Text`,
`TextWidth`, and clip push/pop), plus one optional one:
`FillRoundedRectGradient` (used for the bevel look). It has a safe default
if you don't override it, so it's not a required method to get a working
backend, just a nicer-looking one. Implement it once on top of whatever
you already draw with:

```cpp
class MyRenderer : public cui::IRenderer {
public:
    void FillRect(float x, float y, float w, float h, cui::Color c) override {
        // your engine's rect-fill call
    }
    // ...the rest of IRenderer's methods
};
```

`src/Canvas.cpp` is a complete worked example of every method if you want
something to copy from. `Canvas` itself is a CPU software renderer that
ships with the library so the examples run without a GPU or window
system - swap it for your real backend in an actual application.

## 5. Config profiles

Every widget you add is automatically save/load-able by its id:

```cpp
menu.SaveProfile("configs", "default");   // writes configs/default.cfg
menu.LoadProfile("configs", "default");   // reads it back into the widgets
menu.ListProfiles("configs");             // -> {"default", "pvp", "afk", ...}
```

The `.cfg` file is plain text (`key=type:value` per line) so it's easy to
inspect, diff, or hand-edit. Wire this up to buttons the same way as
anything else:

```cpp
tab.Add<cui::Button>("cfg.save", "Save", [&]{ menu.SaveProfile("configs", "default"); });
tab.Add<cui::Button>("cfg.load", "Load", [&]{ menu.LoadProfile("configs", "default"); });
```

## 6. Theming

Everything visual lives in one struct:

```cpp
cui::Theme& theme = menu.GetTheme();
theme.accent = cui::Color::FromHex(0x00FF88);
theme.cornerRadius = 10.0f;
theme.animDuration = 0.25f; // slower fades everywhere
```

Two more knobs control the "modern" look - the soft glow around active/hovered
elements and the light-top/dark-bottom bevel gradient on panels and buttons:

```cpp
theme.glowStrength = 0.8f;   // 0 = no glow anywhere, higher = brighter halos
theme.bevelStrength = 0.15f; // 0 = flat fills, higher = more pronounced bevel
```

There's also a built-in hue-cycling accent mode:

```cpp
theme.rainbowAccent = true;
```

flip that from a checkbox's `onChange` and the accent color continuously
cycles instead of staying fixed - see `examples/demo.cpp`'s "Visuals" tab
for the wiring. The sidebar's active-tab indicator also has a slow ambient
shimmer built in by default, independent of `rainbowAccent`.

If you're drawing your own widget, `include/Style.hpp` has two helpers
built purely on top of `IRenderer` so they work with any backend:

```cpp
cui::style::Glow(renderer, bounds, cornerRadius, theme.accent, theme.glowStrength);
cui::style::BeveledPanel(renderer, bounds, cornerRadius, baseColor, theme.bevelStrength);
```

## 7. Window controls (minimize, close, custom buttons)

The title bar can hold a minimize button, a close button, and any custom
buttons you want - all three are opt-in and independent of each other.

**Minimize** is on by default. Clicking it smoothly collapses the window
down to just its title bar (content fades/clips out rather than just
vanishing). Turn it off with:

```cpp
menu.SetCollapsible(false);
```

**Close** only appears once you give it something to do - closing usually
means something app-specific, so cheatui doesn't assume:

```cpp
menu.SetOnClose([&]() {
    menu.SetVisible(false); // fades the whole menu out
    // or: appRunning = false; save_settings(); etc.
});
```

**Custom buttons** (a pin, a settings gear, whatever) go in left of
minimize/close, in the order you add them:

```cpp
menu.AddTitleBarButton("o", [&]() { alwaysOnTop = !alwaysOnTop; });
```

All three get the same hover glow and beveled look as everything else, for
free.

## 8. Running the examples

```sh
cmake -S . -B build
cmake --build build
./build/cheatui_getting_started   # the walkthrough above, as a program
./build/cheatui_demo              # a bigger 5-tab showcase
```

Both write `.raw` frame dumps to `out/` (there's no window system in these
console examples, so they render to an in-memory canvas and save it).
Turn one into a real image with:

```sh
python3 tools/raw_to_png.py out/getting_started.raw out/getting_started.png
```

## 9. Adding your own widget type

Every widget is just a class with the same shape: implement `Update`
(handle input, advance animations), `Render` (draw with `IRenderer`), and
`SaveTo`/`LoadFrom` (config persistence). Look at `Stepper` in
`src/Widgets.cpp` for a short one to copy from - it's about 40 lines
start to finish. Nothing about `Tab` or `Menu` needs to change; once your
class exists you use it exactly like any built-in widget:

```cpp
tab.Add<MyWidget>("my.id", "My Label", someStartValue);
```
