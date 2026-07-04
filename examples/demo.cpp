// Showcase / smoke-test for the cheatui library.
//
// This sandbox has no window system, so instead of opening a real OS window
// this drives the library exactly how a real backend would each frame
// (build an InputState, call Menu::Update, call Menu::Render) but renders
// into the software Canvas and dumps a handful of frames to disk so the
// animations (tab switch, toggle fade, hover, dragging, theme cross-fade)
// are visible.
//
// For a much shorter, line-by-line walkthrough aimed at first-time users,
// see examples/getting_started.cpp and TUTORIAL.md instead.
//
// To use this in a real app: replace Canvas with an IRenderer backed by
// Win32/GDI, Direct3D, or OpenGL, and feed real OS input into InputState
// each frame. Nothing else changes.

#include "Menu.hpp"
#include "Canvas.hpp"
#include "Themes.hpp"
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
    auto& fpsSlider = general.Add<Slider>("general.fps_cap", "FPS cap", 30.0f, 240.0f, 144.0f, 1.0f, "");
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

    // Layout constants mirrored from Menu's private ones, so this file can
    // work out where things land without needing Menu to expose them.
    const float kMargin = 8.0f, kGap = 10.0f, kTitleH = 36.0f, kTabRowH = 34.0f, kPad = 10.0f;
    const Rect winBounds{40, 30, 680, 400};
    const Rect titleBar{winBounds.x + kMargin, winBounds.y + kMargin, winBounds.w - kMargin * 2.0f, kTitleH};
    const Rect sidebar{winBounds.x + kMargin, titleBar.y + kTitleH + kGap, 130.0f,
                        winBounds.h - kMargin * 2.0f - kTitleH - kGap};
    const Rect contentCard{sidebar.x + sidebar.w + kGap, sidebar.y,
                            winBounds.x + winBounds.w - kMargin - (sidebar.x + sidebar.w + kGap), sidebar.h};
    const Rect innerContent{contentCard.x + kPad, contentCard.y + kPad,
                             contentCard.w - kPad * 2.0f, contentCard.h - kPad * 2.0f};

    InputState in;
    float dt = 1.0f / 60.0f;

    // Frame 0: initial state, nothing hovered.
    menu.Update(in, dt);
    renderFrame("frame_00_initial");

    // Hover then click the first checkbox ("Auto-save config on exit") -
    // Label "GENERAL" (22px) sits above it inside the content card.
    in.mouseX = innerContent.x + 30; in.mouseY = innerContent.y + 22.0f + 6.0f + 14.0f;
    for (int i = 0; i < 6; ++i) menu.Update(in, dt);
    in.mousePressed = true;
    menu.Update(in, dt);
    in.mousePressed = false;
    for (int i = 0; i < 10; ++i) menu.Update(in, dt); // let the pop animation play out
    renderFrame("frame_01_toggle_on");

    // Switch to the "Visuals" tab (4th row) - captured mid-slide to show
    // the sidebar indicator and content crossfade/slide both in flight.
    in.mouseX = sidebar.x + 65; in.mouseY = sidebar.y + kTabRowH * 3.0f + 10.0f;
    in.mousePressed = true;
    menu.Update(in, dt);
    in.mousePressed = false;
    for (int i = 0; i < 4; ++i) menu.Update(in, dt); // indicator + tab content still transitioning
    renderFrame("frame_02_tab_switching");
    for (int i = 0; i < 16; ++i) menu.Update(in, dt); // both settle
    renderFrame("frame_03_tab_settled");

    // Open the color picker (first widget after the "THEME" label) and
    // drag around inside the saturation/value square.
    float pickerHeaderY = innerContent.y + 22.0f + 6.0f;
    in.mouseX = innerContent.x + 60; in.mouseY = pickerHeaderY + 14.0f;
    in.mousePressed = true;
    menu.Update(in, dt); // opens the picker
    in.mousePressed = false;
    for (int i = 0; i < 8; ++i) menu.Update(in, dt); // open animation plays
    renderFrame("frame_04_colorpicker_open");

    float svX = innerContent.x + 8.0f, svY = pickerHeaderY + 28.0f + 6.0f;
    in.mouseX = svX + 60; in.mouseY = svY + 20.0f;
    in.mousePressed = true;
    menu.Update(in, dt);
    in.mouseX += 10; in.mouseY -= 15;
    menu.Update(in, dt);
    in.mousePressed = false;
    in.mouseReleased = true;
    menu.Update(in, dt);
    in.mouseReleased = false;
    renderFrame("frame_05_colorpicker_dragged");

    // Enable rainbow accent - it sits right below the now-fully-open color
    // picker (header 28px + open panel 120px + 6px gap below it).
    in.mouseX = 0; in.mouseY = 0;
    menu.Update(in, dt);
    {
        float rainbowY = pickerHeaderY + 28.0f + 120.0f + 6.0f;
        in.mouseX = innerContent.x + 10.0f; in.mouseY = rainbowY + 14.0f;
        in.mousePressed = true;
        menu.Update(in, dt);
        in.mousePressed = false;
    }
    for (int i = 0; i < 30; ++i) menu.Update(in, dt); // step the hue cycler
    renderFrame("frame_06_rainbow_accent");
    menu.GetTheme().rainbowAccent = false;
    rainbow.SetValue(false, true);

    // Hover the minimize button, then click it - shows the chrome button
    // glow and the window collapsing down to just its title bar. Chrome
    // buttons lay out right-to-left from the title bar's right edge:
    // close (20px) first, then a 6px gap, then minimize.
    float chromeY = titleBar.y + (kTitleH - 20.0f) * 0.5f + 10.0f;
    float minimizeX = titleBar.x + titleBar.w - kPad - 20.0f - 6.0f - 10.0f;
    in.mouseX = minimizeX; in.mouseY = chromeY;
    for (int i = 0; i < 8; ++i) menu.Update(in, dt);
    renderFrame("frame_07_minimize_hover");

    in.mousePressed = true;
    menu.Update(in, dt);
    in.mousePressed = false;
    for (int i = 0; i < 14; ++i) menu.Update(in, dt); // collapse animation plays
    renderFrame("frame_08_collapsed");

    // Expand it back out.
    in.mousePressed = true;
    menu.Update(in, dt);
    in.mousePressed = false;
    for (int i = 0; i < 14; ++i) menu.Update(in, dt);
    in.mouseX = 0; in.mouseY = 0;
    menu.Update(in, dt);

    // Switch a whole color theme with a real cross-fade - captured
    // mid-transition, then settled.
    menu.SetTheme(themes::Crimson());
    for (int i = 0; i < 6; ++i) menu.Update(in, dt);
    renderFrame("frame_09_theme_transition");
    for (int i = 0; i < 20; ++i) menu.Update(in, dt);
    renderFrame("frame_10_theme_settled");
    menu.SetTheme(themes::Default(), false); // back to default, no transition needed for the remaining frames

    // Back to the General tab to reach the FPS slider, then click its
    // value box and type a new number.
    in.mouseX = sidebar.x + 65; in.mouseY = sidebar.y + kTabRowH * 0.0f + 10.0f;
    in.mousePressed = true;
    menu.Update(in, dt);
    in.mousePressed = false;
    for (int i = 0; i < 16; ++i) menu.Update(in, dt);

    float fpsY = innerContent.y + 22.0f + 6.0f    // GENERAL label
                 + 28.0f + 6.0f                    // auto_save checkbox
                 + 28.0f + 6.0f                    // auto_reconnect checkbox
                 + 28.0f + 6.0f                    // notifications checkbox
                 + 22.0f + 6.0f;                   // PERFORMANCE label
    Rect fpsValueBox{innerContent.x + innerContent.w - 64.0f, fpsY, 60.0f, 16.0f};
    in.mouseX = fpsValueBox.x + fpsValueBox.w * 0.5f; in.mouseY = fpsValueBox.y + fpsValueBox.h * 0.5f;
    in.mousePressed = true;
    menu.Update(in, dt); // click the value box - starts editing, buffer pre-filled with "144"
    in.mousePressed = false;
    menu.Update(in, dt);
    renderFrame("frame_11_slider_edit_start");

    in.backspace = true; menu.Update(in, dt); in.backspace = false; // clear the pre-filled "144"
    menu.Update(in, dt);
    in.backspace = true; menu.Update(in, dt); in.backspace = false;
    menu.Update(in, dt);
    in.backspace = true; menu.Update(in, dt); in.backspace = false;
    for (char c : std::string("77")) {
        in.textChar = c;
        menu.Update(in, dt);
        in.textChar = 0;
    }
    for (int i = 0; i < 6; ++i) menu.Update(in, dt); // cursor blink a couple times
    renderFrame("frame_12_slider_edit_typing");

    in.enterPressed = true;
    menu.Update(in, dt);
    in.enterPressed = false;
    for (int i = 0; i < 10; ++i) menu.Update(in, dt); // thumb glides to the new value
    renderFrame("frame_13_slider_edit_committed");
    std::cout << "[demo] fps slider value after typed edit: " << fpsSlider.Value() << "\n";

    // Persist and reload a config profile to prove the config system round-trips.
    menu.SaveProfile(kOutDir + "/profiles", "default");
    std::cout << "[demo] saved profile to " << kOutDir << "/profiles/default.cfg\n";
    bool ok = menu.LoadProfile(kOutDir + "/profiles", "default");
    std::cout << "[demo] reload ok=" << ok << "\n";

    std::cout << "[demo] done.\n";
    return 0;
}
