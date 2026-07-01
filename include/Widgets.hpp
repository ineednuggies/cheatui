#pragma once
#include "Renderer.hpp"
#include "Theme.hpp"
#include "Animation.hpp"
#include "Input.hpp"
#include "Config.hpp"
#include <string>
#include <vector>
#include <functional>

namespace cui {

// Base class every control inherits from. id is what ties a widget to a
// config key, so keep it unique per menu (e.g. "combat.delay", not just
// "delay"). Update() and Render() are split so animations can keep
// progressing even on a frame where you skip drawing.
class Widget {
public:
    explicit Widget(std::string id) : id_(std::move(id)) {}
    virtual ~Widget() = default;

    virtual float Height(const Theme& theme) const { return theme.rowHeight; }
    virtual void Update(const InputState& in, Rect bounds, float dt) = 0;
    virtual void Render(IRenderer& r, const Theme& theme, Rect bounds) const = 0;

    virtual void SaveTo(ConfigStore& cfg) const = 0;
    virtual void LoadFrom(const ConfigStore& cfg) = 0;

    const std::string& Id() const { return id_; }

protected:
    std::string id_;
};

// On/off switch with a sliding pill and a little overshoot pop when toggled.
class Checkbox : public Widget {
public:
    Checkbox(std::string id, std::string label, bool startValue = false);

    void Update(const InputState& in, Rect bounds, float dt) override;
    void Render(IRenderer& r, const Theme& theme, Rect bounds) const override;
    void SaveTo(ConfigStore& cfg) const override;
    void LoadFrom(const ConfigStore& cfg) override;

    bool Value() const { return on_; }
    void SetValue(bool v, bool snap = false);
    void SetOnChange(std::function<void(bool)> cb) { onChange_ = std::move(cb); }

private:
    std::string label_;
    bool on_;
    mutable AnimatedFloat pillFade_; // 0 = off, 1 = on
    mutable AnimatedFloat glow_{0.0f, 0.1f};
    std::function<void(bool)> onChange_;
};

// Drag-to-set numeric slider. Works for both float ranges and integer
// ranges (set step = 1 and it'll snap and print as a whole number).
class Slider : public Widget {
public:
    Slider(std::string id, std::string label, float lo, float hi, float startValue, float step = 0.0f,
           std::string suffix = "");

    void Update(const InputState& in, Rect bounds, float dt) override;
    void Render(IRenderer& r, const Theme& theme, Rect bounds) const override;
    void SaveTo(ConfigStore& cfg) const override;
    void LoadFrom(const ConfigStore& cfg) override;

    float Value() const { return value_; }
    void SetValue(float v, bool snap = false);
    void SetOnChange(std::function<void(float)> cb) { onChange_ = std::move(cb); }

private:
    std::string label_, suffix_;
    float lo_, hi_, step_, value_;
    bool dragging_ = false;
    mutable AnimatedFloat thumbPos_;
    std::function<void(float)> onChange_;

    float SnapToStep(float v) const;
};

// Compact "value with - / + buttons" control. Handy for small integer
// counts where a full drag-slider feels like overkill (stack size, hotkey
// repeat count, that kind of thing).
class Stepper : public Widget {
public:
    Stepper(std::string id, std::string label, int lo, int hi, int startValue, int step = 1);

    void Update(const InputState& in, Rect bounds, float dt) override;
    void Render(IRenderer& r, const Theme& theme, Rect bounds) const override;
    void SaveTo(ConfigStore& cfg) const override;
    void LoadFrom(const ConfigStore& cfg) override;

    int Value() const { return value_; }
    void SetValue(int v);
    void SetOnChange(std::function<void(int)> cb) { onChange_ = std::move(cb); }

private:
    std::string label_;
    int lo_, hi_, step_, value_;
    mutable AnimatedFloat minusGlow_{0.0f, 0.1f};
    mutable AnimatedFloat plusGlow_{0.0f, 0.1f};
    std::function<void(int)> onChange_;
};

// Dropdown list. Click the header to open, click an option to pick it,
// click anywhere else to dismiss.
class ComboBox : public Widget {
public:
    ComboBox(std::string id, std::string label, std::vector<std::string> options, int startIndex = 0);

    float Height(const Theme& theme) const override;
    void Update(const InputState& in, Rect bounds, float dt) override;
    void Render(IRenderer& r, const Theme& theme, Rect bounds) const override;
    void SaveTo(ConfigStore& cfg) const override;
    void LoadFrom(const ConfigStore& cfg) override;

    int SelectedIndex() const { return selected_; }
    const std::string& SelectedValue() const { return options_[selected_]; }

private:
    std::string label_;
    std::vector<std::string> options_;
    int selected_;
    bool open_ = false;
    mutable AnimatedFloat openFade_{0.0f, 0.18f};
};

// A row of buttons where exactly one is active at a time - presets,
// difficulty levels, render quality, that style of choice. Functionally
// similar to a ComboBox but reads better when there are only 2-4 options
// and you want them all visible at once instead of tucked in a dropdown.
class RadioGroup : public Widget {
public:
    RadioGroup(std::string id, std::string label, std::vector<std::string> options, int startIndex = 0);

    float Height(const Theme& theme) const override;
    void Update(const InputState& in, Rect bounds, float dt) override;
    void Render(IRenderer& r, const Theme& theme, Rect bounds) const override;
    void SaveTo(ConfigStore& cfg) const override;
    void LoadFrom(const ConfigStore& cfg) override;

    int SelectedIndex() const { return selected_; }
    const std::string& SelectedValue() const { return options_[selected_]; }

private:
    std::string label_;
    std::vector<std::string> options_;
    int selected_;
    mutable AnimatedFloat selectorX_;
    mutable AnimatedFloat selectorW_;
    bool initialized_ = false;
};

// HSV color swatch: click to expand a saturation/value square plus a hue
// strip. Used for theme colors, outline colors, anything that just needs
// "pick an RGB value" with no further meaning attached.
class ColorPicker : public Widget {
public:
    ColorPicker(std::string id, std::string label, Color startValue);

    float Height(const Theme& theme) const override;
    void Update(const InputState& in, Rect bounds, float dt) override;
    void Render(IRenderer& r, const Theme& theme, Rect bounds) const override;
    void SaveTo(ConfigStore& cfg) const override;
    void LoadFrom(const ConfigStore& cfg) override;

    Color Value() const { return value_; }

private:
    std::string label_;
    Color value_;
    float hue_ = 0, sat_ = 1, val_ = 1;
    bool open_ = false;
    bool draggingSV_ = false, draggingHue_ = false;
    mutable AnimatedFloat openFade_{0.0f, 0.18f};
};

// Plain push button.
class Button : public Widget {
public:
    Button(std::string id, std::string label, std::function<void()> onClick);

    void Update(const InputState& in, Rect bounds, float dt) override;
    void Render(IRenderer& r, const Theme& theme, Rect bounds) const override;
    void SaveTo(ConfigStore&) const override {}
    void LoadFrom(const ConfigStore&) override {}

private:
    std::string label_;
    std::function<void()> onClick_;
    mutable AnimatedFloat glow_{0.0f, 0.12f};
    bool wasDown_ = false;
};

// "Click to set a key" widget. Click it, then press any key (or a mouse
// button via SetMouseButton) and it captures that as the new bind. Reads
// keyPressed off InputState, so your input backend just needs to fill that
// in with whatever key code your platform uses - the widget doesn't care
// what the numbers mean, it just stores and compares them.
class Keybind : public Widget {
public:
    Keybind(std::string id, std::string label, int startKey = 0);

    void Update(const InputState& in, Rect bounds, float dt) override;
    void Render(IRenderer& r, const Theme& theme, Rect bounds) const override;
    void SaveTo(ConfigStore& cfg) const override;
    void LoadFrom(const ConfigStore& cfg) override;

    int Key() const { return key_; }
    void SetKey(int key) { key_ = key; }
    void SetKeyName(std::function<std::string(int)> nameFn) { keyName_ = std::move(nameFn); }
    void SetOnChange(std::function<void(int)> cb) { onChange_ = std::move(cb); }

private:
    std::string label_;
    int key_;
    bool listening_ = false;
    mutable AnimatedFloat glow_{0.0f, 0.1f};
    std::function<std::string(int)> keyName_; // optional: key code -> display name
    std::function<void(int)> onChange_;
};

// Non-interactive section header, used to group a few widgets under a
// title within a tab ("GENERAL", "TIMING", etc).
class Label : public Widget {
public:
    explicit Label(std::string text) : Widget(""), text_(std::move(text)) {}
    float Height(const Theme&) const override { return 22.0f; }
    void Update(const InputState&, Rect, float) override {}
    void Render(IRenderer& r, const Theme& theme, Rect bounds) const override;
    void SaveTo(ConfigStore&) const override {}
    void LoadFrom(const ConfigStore&) override {}

private:
    std::string text_;
};

} // namespace cui
