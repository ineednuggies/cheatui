// Showcase / smoke-test for the cheatui library.
//
// This sandbox has no window system, so instead of opening a real OS window
// this drives the library exactly how a real backend would each frame
// (build an InputState, call Menu::Update, call Menu::Render) but renders
// into the software Canvas and dumps a handful of frames to disk so the
// animations (tab switch, toggle fade, hover, dragging) are visible.
//
// For a much shorter, line-by-line walkthrough aimed at first-time users,
// see examples/getting_started.cpp and TUTORIAL.md instead.
//
// To use this in a real app: replace Canvas with an IRenderer backed by
// Win32/GDI, Direct3D, or OpenGL, and feed real OS input into InputState
// each frame. Nothing else changes.

#include "Menu.hpp"
#include "Canvas.hpp"
#include <iostream>
#include <filesystem>

using namespace cui;

static const std::string kOutDir = "out";

int main() {
    std::filesystem::create_directories(kOutDir);
    std::filesystem::create_directories(kOutDir + "/profiles");

    const int W = 760, H = 460;
    Canvas canvas(W, H);

    Menu menu("CUI - Demo Menu", Rect{40, 30, 680, 400});

    // Window chrome: minimize is on by default (click it to collapse the
    // menu down to just its title bar). Close only appears once you give
    // it something to do - here it just hides the menu, but it could just
    // as easily quit the app, save state first, whatever you need.
    bool appRunning = true;
    menu.SetOnClose([&]() { menu.SetVisible(false); appRunning = false; });
    menu.AddTitleBarButton("o", [&]() { std::cout << "[demo] pin button clicked\n"; });
    (void)appRunning;

    Tab& general = menu.AddTab("General");
    general.Add<Label>("GENERAL");
    general.Add<Checkbox>("general.auto_save", "Auto-save config on exit", true);
    general.Add<Checkbox>("general.auto_reconnect", "Auto-reconnect on disconnect", false);
    general.Add<Checkbox>("general.notifications", "Show notifications", true);
    general.Add<Label>("PERFORMANCE");
    general.Add<Slider>("general.fps_cap", "FPS cap", 30.0f, 240.0f, 144.0f, 1.0f, "");
    general.Add<Slider>("general.opacity", "Menu opacity", 0.2f, 1.0f, 0.92f, 0.01f, "");
    general.Add<RadioGroup>("general.quality", "Render quality",
                             std::vector<std::string>{"Low", "Med", "High", "Ultra"}, 2);

    Tab& timing = menu.AddTab("Timing");
    timing.Add<Label>("DELAYS");
    timing.Add<Slider>("timing.action_delay", "Action delay", 0.0f, 500.0f, 120.0f, 5.0f, "ms");
    timing.Add<Slider>("timing.retry_delay", "Retry delay", 0.0f, 2000.0f, 250.0f, 10.0f, "ms");
    timing.Add<Stepper>("timing.retry_count", "Retry attempts", 0, 10, 3, 1);
    timing.Add<Checkbox>("timing.randomize", "Randomize delay (+/- 10%)", true);

    Tab& binds = menu.AddTab("Binds");
    binds.Add<Label>("HOTKEYS");
    binds.Add<Keybind>("binds.menu_toggle", "Toggle menu", 45);   // 45 = VK_INSERT on Windows
    binds.Add<Keybind>("binds.panic", "Panic key", 0x70);          // 0x70 = VK_F1 on Windows
    binds.Add<ComboBox>("binds.profile", "Active bind profile",
                         std::vector<std::string>{"Default", "Travel", "Home"}, 0);

    Tab& visuals = menu.AddTab("Visuals");
    visuals.Add<Label>("THEME");
    visuals.Add<ColorPicker>("visuals.accent_color", "Accent color", Color::FromHex(0x7C5CFF));
    auto& rainbow = visuals.Add<Checkbox>("visuals.rainbow", "Rainbow accent (hue-cycling)", false);
    visuals.Add<Label>("OVERLAY");
    visuals.Add<Slider>("visuals.box_thickness", "Outline thickness", 1.0f, 5.0f, 2.0f, 1.0f, "px");
    visuals.Add<ComboBox>("visuals.font", "Font", std::vector<std::string>{"Default", "Bold", "Mono"}, 0);

    Tab& configTab = menu.AddTab("Config");
    configTab.Add<Label>("PROFILES");
    configTab.Add<Button>("config.save_btn", "Save profile \"default\"", [&menu]() {
        menu.SaveProfile(kOutDir + "/profiles", "default");
        std::cout << "[demo] profile saved\n";
    });
    configTab.Add<Button>("config.load_btn", "Load profile \"default\"", [&menu]() {
        menu.LoadProfile(kOutDir + "/profiles", "default");
        std::cout << "[demo] profile loaded\n";
    });

    // Wire the rainbow-accent checkbox straight into the theme flag.
    rainbow.SetOnChange([&menu](bool v) { menu.GetTheme().rainbowAccent = v; });

    auto renderFrame = [&](const std::string& name) {
        canvas.Clear(Color::FromHex(0x05060A));
        menu.Render(canvas);
        canvas.SaveRaw(kOutDir + "/" + name + ".raw");
        std::cout << "[demo] wrote frame " << name << "\n";
    };

    InputState in;
    float dt = 1.0f / 60.0f;

    // Frame 0: initial state, nothing hovered.
    in = InputState{};
    menu.Update(in, dt);
    renderFrame("frame_00_initial");

    // Frames simulating cursor moving onto the "Auto-save" toggle and clicking it
    // (smooth pill-fade + back-out pop), then a few settle frames.
    in.mouseX = 40 + 30; in.mouseY = 30 + 36 + 10 + 30; // approx over first checkbox row
    for (int i = 0; i < 6; ++i) { menu.Update(in, dt); }
    in.mousePressed = true;
    menu.Update(in, dt);
    in.mousePressed = false;
    for (int i = 0; i < 10; ++i) menu.Update(in, dt); // let the pop animation play out
    renderFrame("frame_01_toggle_on");

    // Switch to the "Visuals" tab - exercises the sliding sidebar indicator.
    in.mouseX = 40 + 65; in.mouseY = 30 + 36 + 34 * 3 + 10; // 4th tab row ("Visuals")
    in.mousePressed = true;
    menu.Update(in, dt);
    in.mousePressed = false;
    for (int i = 0; i < 6; ++i) menu.Update(in, dt); // indicator still sliding
    renderFrame("frame_02_tab_switching");
    for (int i = 0; i < 14; ++i) menu.Update(in, dt); // indicator settles
    renderFrame("frame_03_tab_settled");

    // Open the color picker and drag the hue strip + SV square a bit.
    Rect content{40 + 130 + 10, 30 + 36 + 10, 680 - 130 - 20, 400 - 36 - 20};
    float pickerHeaderY = content.y + 22.0f + 6.0f; // after the "THEME" label row
    in.mouseX = content.x + 60; in.mouseY = pickerHeaderY + 14.0f;
    in.mousePressed = true;
    menu.Update(in, dt); // opens picker
    in.mousePressed = false;
    for (int i = 0; i < 8; ++i) menu.Update(in, dt); // open animation plays
    renderFrame("frame_04_colorpicker_open");

    // drag inside the SV square
    in.mouseX = content.x + 8 + 60; in.mouseY = pickerHeaderY + 28.0f + 20.0f;
    in.mousePressed = true;
    menu.Update(in, dt);
    in.mouseX += 10; in.mouseY -= 15;
    menu.Update(in, dt);
    in.mousePressed = false;
    in.mouseReleased = true;
    menu.Update(in, dt);
    in.mouseReleased = false;
    renderFrame("frame_05_colorpicker_dragged");

    // Enable rainbow accent and step the hue cycler a few frames.
    in.mouseX = 0; in.mouseY = 0; // move mouse away
    menu.Update(in, dt);
    // simulate clicking the rainbow checkbox directly via its own Update path
    // (re-derive its row rect the same way the layout does it).
    {
        Rect rainbowRect{content.x, pickerHeaderY + 28.0f + 96.0f, content.w, 28.0f};
        in.mouseX = rainbowRect.x + 10; in.mouseY = rainbowRect.y + rainbowRect.h * 0.5f;
        in.mousePressed = true;
        menu.Update(in, dt);
        in.mousePressed = false;
    }
    for (int i = 0; i < 30; ++i) menu.Update(in, dt); // step hue cycler
    renderFrame("frame_06_rainbow_accent");

    // Hover the minimize button, then click it - shows the chrome button
    // glow and the window collapsing down to just its title bar.
    // Chrome buttons lay out right-to-left from the title bar's right edge:
    // close (20px) is first, then a 6px gap, then minimize - this lands on
    // minimize's center.
    in.mouseX = 40 + 680 - 10 - 20 - 6 - 10; in.mouseY = 30 + 18;
    for (int i = 0; i < 8; ++i) menu.Update(in, dt);
    renderFrame("frame_07_minimize_hover");

    in.mousePressed = true;
    menu.Update(in, dt);
    in.mousePressed = false;
    for (int i = 0; i < 14; ++i) menu.Update(in, dt); // collapse animation plays
    renderFrame("frame_08_collapsed");

    // Expand it back out for anyone poking at the remaining frames.
    in.mousePressed = true;
    menu.Update(in, dt);
    in.mousePressed = false;
    for (int i = 0; i < 14; ++i) menu.Update(in, dt);

    // Persist and reload a config profile to prove the config system round-trips.
    menu.SaveProfile(kOutDir + "/profiles", "default");
    std::cout << "[demo] saved profile to " << kOutDir << "/profiles/default.cfg\n";
    bool ok = menu.LoadProfile(kOutDir + "/profiles", "default");
    std::cout << "[demo] reload ok=" << ok << "\n";

    std::cout << "[demo] done.\n";
    return 0;
}
