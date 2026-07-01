// Getting-started example.
//
// This is the short version - one tab, one of each widget type, and a
// loop shaped like what you'd write against a real window. Read this
// alongside TUTORIAL.md. For a bigger showcase with multiple tabs and a
// rainbow-accent toggle, see examples/demo.cpp instead.

#include "cheatui.hpp"
#include "Canvas.hpp"   // only needed because this example has no real window to draw to
#include <filesystem>
#include <iostream>

using namespace cui;

int main() {
    // 1. Make a menu. Position and size are just a starting rectangle -
    //    the title bar is draggable so the window can move from there.
    Menu menu("My App", Rect{60, 60, 520, 360});

    // 2. Add a tab and put some widgets in it. Every widget's first
    //    argument is its id - that's the key it gets saved/loaded under,
    //    so keep it unique within your menu.
    Tab& main = menu.AddTab("Main");

    main.Add<Label>("OPTIONS");
    auto& enabled = main.Add<Checkbox>("main.enabled", "Enable feature", true);
    main.Add<Slider>("main.speed", "Speed", 0.0f, 100.0f, 50.0f, 1.0f, "%");
    main.Add<Stepper>("main.retries", "Retries", 0, 5, 2);
    main.Add<RadioGroup>("main.mode", "Mode", std::vector<std::string>{"A", "B", "C"}, 0);
    main.Add<ComboBox>("main.preset", "Preset",
                        std::vector<std::string>{"Default", "Custom"}, 0);
    main.Add<ColorPicker>("main.color", "Highlight color", Color::FromHex(0x7C5CFF));
    main.Add<Keybind>("main.hotkey", "Hotkey", 0);

    // 3. React to changes immediately with a callback, if you want one -
    //    or just read enabled.Value() / slider.Value() / etc. whenever
    //    you need it, e.g. once per frame in your own app logic.
    enabled.SetOnChange([](bool v) {
        std::cout << "feature toggled: " << (v ? "on" : "off") << "\n";
    });

    // --- the part below is the render loop shape; swap Canvas for your ---
    // --- own IRenderer and feed it real input instead of fake frames.  ---

    Canvas canvas(640, 480);
    InputState input;     // a real backend fills this from OS events each frame
    float dt = 1.0f / 60.0f;

    menu.Update(input, dt);
    canvas.Clear(Color::FromHex(0x05060A));
    menu.Render(canvas);

    std::filesystem::create_directories("out");
    canvas.SaveRaw("out/getting_started.raw");
    std::cout << "wrote out/getting_started.raw "
                 "(convert with tools/raw_to_png.py to view it)\n";

    return 0;
}
