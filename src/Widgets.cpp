#include "Widgets.hpp"
#include "Style.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace cui {

static std::string FormatNumber(float v, bool whole) {
    std::ostringstream ss;
    if (whole) ss << static_cast<int>(std::lround(v));
    else ss << std::fixed << std::setprecision(2) << v;
    return ss.str();
}

// ---------------------------------------------------------------- Checkbox

Checkbox::Checkbox(std::string id, std::string label, bool startValue)
    : Widget(std::move(id)), label_(std::move(label)), on_(startValue),
      pillFade_(startValue ? 1.0f : 0.0f, 0.15f, easing::BackOut) {}

void Checkbox::SetValue(bool v, bool snap) {
    on_ = v;
    pillFade_.SetTarget(v ? 1.0f : 0.0f, snap);
}

void Checkbox::Update(const InputState& in, Rect bounds, float dt) {
    bool hovered = bounds.Contains(in.mouseX, in.mouseY);
    glow_.SetTarget(hovered ? 1.0f : 0.0f);
    glow_.Update(dt);
    pillFade_.Update(dt);
    if (hovered && in.mousePressed) {
        SetValue(!on_);
        if (onChange_) onChange_(on_);
    }
}

void Checkbox::Render(IRenderer& r, const Theme& theme, Rect bounds) const {
    Color rowBg = Color::Lerp(theme.background, theme.panelAlt, glow_.Value() * 0.6f);
    style::BeveledPanel(r, bounds, theme.cornerRadius * 0.6f, rowBg, theme.bevelStrength * 0.5f);

    r.Text(bounds.x + theme.padding, bounds.y + bounds.h * 0.5f - 3.5f, label_, theme.text, 1.0f);

    float pillW = 34.0f, pillH = 16.0f;
    float pillX = bounds.x + bounds.w - pillW - theme.padding;
    float pillY = bounds.y + (bounds.h - pillH) * 0.5f;
    float t = pillFade_.Value();

    if (t > 0.02f) {
        style::Glow(r, Rect{pillX, pillY, pillW, pillH}, pillH * 0.5f, theme.accent, t * theme.glowStrength);
    }

    Color trackBase = Color::Lerp(theme.toggleOff, theme.accent, t);
    Color trackTop = Color::Lerp(trackBase, theme.accentSoft, t * 0.6f);
    r.FillRoundedRectGradient(pillX, pillY, pillW, pillH, pillH * 0.5f, trackTop, trackBase);

    float knobR = pillH * 0.5f - 2.0f;
    float knobX = pillX + 2.0f + knobR + (pillW - 4.0f - knobR * 2.0f) * t;
    float knobY = pillY + pillH * 0.5f;
    r.FillCircle(knobX, knobY, knobR, Color(255, 255, 255, 255));
    r.FillCircle(knobX, knobY - knobR * 0.25f, knobR * 0.6f, Color(255, 255, 255, 90)); // soft top-lit highlight
}

void Checkbox::SaveTo(ConfigStore& cfg) const { cfg.SetBool(id_, on_); }
void Checkbox::LoadFrom(const ConfigStore& cfg) { SetValue(cfg.GetBool(id_, on_), true); }

// ------------------------------------------------------------------ Slider

Slider::Slider(std::string id, std::string label, float lo, float hi, float startValue, float step,
               std::string suffix)
    : Widget(std::move(id)), label_(std::move(label)), suffix_(std::move(suffix)),
      lo_(lo), hi_(hi), step_(step), value_(std::clamp(startValue, lo, hi)) {
    float t = (hi_ > lo_) ? (value_ - lo_) / (hi_ - lo_) : 0.0f;
    thumbPos_.SetTarget(t, true);
}

float Slider::SnapToStep(float v) const {
    v = std::clamp(v, lo_, hi_);
    if (step_ > 0.0f) v = lo_ + std::round((v - lo_) / step_) * step_;
    return v;
}

void Slider::SetValue(float v, bool snap) {
    value_ = SnapToStep(v);
    float t = (hi_ > lo_) ? (value_ - lo_) / (hi_ - lo_) : 0.0f;
    thumbPos_.SetTarget(t, snap);
}

void Slider::Update(const InputState& in, Rect bounds, float dt) {
    float trackY = bounds.y + bounds.h - 12.0f;
    Rect track{bounds.x + 2.0f, trackY, bounds.w - 4.0f, 8.0f};
    Rect grabZone{track.x - 4, track.y - 8, track.w + 8, track.h + 16};

    if (grabZone.Contains(in.mouseX, in.mouseY) && in.mousePressed) dragging_ = true;
    if (in.mouseReleased) dragging_ = false;

    if (dragging_) {
        float t = std::clamp((in.mouseX - track.x) / track.w, 0.0f, 1.0f);
        SetValue(lo_ + t * (hi_ - lo_), true); // snap while dragging so the thumb tracks the cursor exactly
        if (onChange_) onChange_(value_);
    }
    thumbPos_.Update(dt);
    bool nearThumb = grabZone.Contains(in.mouseX, in.mouseY);
    thumbGlow_.SetTarget((nearThumb || dragging_) ? 1.0f : 0.0f);
    thumbGlow_.Update(dt);
}

void Slider::Render(IRenderer& r, const Theme& theme, Rect bounds) const {
    r.Text(bounds.x + theme.padding, bounds.y + 2.0f, label_, theme.text, 1.0f);
    std::string valStr = FormatNumber(value_, step_ >= 1.0f && step_ == std::floor(step_)) + suffix_;
    float valW = r.TextWidth(valStr, 1.0f);
    r.Text(bounds.x + bounds.w - valW - theme.padding, bounds.y + 2.0f, valStr, theme.accent, 1.0f);

    float trackY = bounds.y + bounds.h - 12.0f;
    float trackX = bounds.x + 2.0f, trackW = bounds.w - 4.0f, trackH = 8.0f;
    style::BeveledPanel(r, Rect{trackX, trackY, trackW, trackH}, trackH * 0.5f, theme.toggleOff, theme.bevelStrength);

    float t = thumbPos_.Value();
    float fillW = trackW * t;
    if (fillW > 1.0f) {
        r.FillRoundedRectGradient(trackX, trackY, fillW, trackH, trackH * 0.5f, theme.accentSoft, theme.accent);
    }

    float thumbX = trackX + fillW;
    float thumbY = trackY + trackH * 0.5f;
    float glowAmount = thumbGlow_.Value();
    if (glowAmount > 0.02f) {
        style::GlowCircle(r, thumbX, thumbY, 6.5f, theme.accent, glowAmount * theme.glowStrength);
    }
    r.FillCircle(thumbX, thumbY, 6.5f, Color(255, 255, 255, 255));
    r.FillCircle(thumbX, thumbY - 1.5f, 3.5f, Color(255, 255, 255, 90)); // top-lit highlight, sells the bevel
}

void Slider::SaveTo(ConfigStore& cfg) const { cfg.SetFloat(id_, value_); }
void Slider::LoadFrom(const ConfigStore& cfg) { SetValue(cfg.GetFloat(id_, value_), true); }

// ----------------------------------------------------------------- Stepper

Stepper::Stepper(std::string id, std::string label, int lo, int hi, int startValue, int step)
    : Widget(std::move(id)), label_(std::move(label)), lo_(lo), hi_(hi), step_(step),
      value_(std::clamp(startValue, lo, hi)) {}

void Stepper::SetValue(int v) { value_ = std::clamp(v, lo_, hi_); }

void Stepper::Update(const InputState& in, Rect bounds, float dt) {
    float btnSize = bounds.h;
    Rect minusBtn{bounds.x + bounds.w - btnSize * 2 - 4, bounds.y, btnSize, btnSize};
    Rect plusBtn{bounds.x + bounds.w - btnSize, bounds.y, btnSize, btnSize};

    bool overMinus = minusBtn.Contains(in.mouseX, in.mouseY);
    bool overPlus = plusBtn.Contains(in.mouseX, in.mouseY);
    minusGlow_.SetTarget(overMinus ? 1.0f : 0.0f);
    plusGlow_.SetTarget(overPlus ? 1.0f : 0.0f);
    minusGlow_.Update(dt);
    plusGlow_.Update(dt);

    if (in.mousePressed) {
        int old = value_;
        if (overMinus) SetValue(value_ - step_);
        if (overPlus) SetValue(value_ + step_);
        if (value_ != old && onChange_) onChange_(value_);
    }
}

void Stepper::Render(IRenderer& r, const Theme& theme, Rect bounds) const {
    r.Text(bounds.x + theme.padding, bounds.y + bounds.h * 0.5f - 3.5f, label_, theme.text, 1.0f);

    float btnSize = bounds.h;
    Rect minusBtn{bounds.x + bounds.w - btnSize * 2 - 4, bounds.y, btnSize, btnSize};
    Rect plusBtn{bounds.x + bounds.w - btnSize, bounds.y, btnSize, btnSize};

    std::string valStr = std::to_string(value_);
    float valW = r.TextWidth(valStr, 1.0f);
    r.Text(minusBtn.x - valW - 10.0f, bounds.y + bounds.h * 0.5f - 3.5f, valStr, theme.accent, 1.0f);

    Color minusBg = Color::Lerp(theme.panelAlt, theme.accent, minusGlow_.Value() * 0.35f);
    Color plusBg = Color::Lerp(theme.panelAlt, theme.accent, plusGlow_.Value() * 0.35f);
    if (minusGlow_.Value() > 0.05f) style::Glow(r, minusBtn, theme.cornerRadius * 0.5f, theme.accent, minusGlow_.Value() * theme.glowStrength * 0.7f);
    if (plusGlow_.Value() > 0.05f) style::Glow(r, plusBtn, theme.cornerRadius * 0.5f, theme.accent, plusGlow_.Value() * theme.glowStrength * 0.7f);
    style::BeveledPanel(r, minusBtn, theme.cornerRadius * 0.5f, minusBg, theme.bevelStrength);
    style::BeveledPanel(r, plusBtn, theme.cornerRadius * 0.5f, plusBg, theme.bevelStrength);

    float mcx = minusBtn.x + minusBtn.w * 0.5f, mcy = minusBtn.y + minusBtn.h * 0.5f;
    r.Line(mcx - 4, mcy, mcx + 4, mcy, theme.text, 1.5f);
    float pcx = plusBtn.x + plusBtn.w * 0.5f, pcy = plusBtn.y + plusBtn.h * 0.5f;
    r.Line(pcx - 4, pcy, pcx + 4, pcy, theme.text, 1.5f);
    r.Line(pcx, pcy - 4, pcx, pcy + 4, theme.text, 1.5f);
}

void Stepper::SaveTo(ConfigStore& cfg) const { cfg.SetInt(id_, value_); }
void Stepper::LoadFrom(const ConfigStore& cfg) { SetValue(cfg.GetInt(id_, value_)); }

// ---------------------------------------------------------------- ComboBox

ComboBox::ComboBox(std::string id, std::string label, std::vector<std::string> options, int startIndex)
    : Widget(std::move(id)), label_(std::move(label)), options_(std::move(options)),
      selected_(std::clamp(startIndex, 0, static_cast<int>(options_.size()) - 1)) {}

float ComboBox::Height(const Theme& theme) const {
    float base = theme.rowHeight;
    float extra = static_cast<float>(options_.size()) * theme.rowHeight * openFade_.Value();
    return base + extra;
}

void ComboBox::Update(const InputState& in, Rect bounds, float dt) {
    Rect header{bounds.x, bounds.y, bounds.w, DefaultTheme().rowHeight};
    bool headerHovered = header.Contains(in.mouseX, in.mouseY);
    if (headerHovered && in.mousePressed) open_ = !open_;

    if (open_) {
        float rowH = DefaultTheme().rowHeight;
        bool clickedAnOption = false;
        for (size_t i = 0; i < options_.size(); ++i) {
            Rect optRect{bounds.x, header.y + rowH * (1 + i), bounds.w, rowH};
            if (optRect.Contains(in.mouseX, in.mouseY) && in.mousePressed) {
                selected_ = static_cast<int>(i);
                open_ = false;
                clickedAnOption = true;
            }
        }
        bool insideAny = headerHovered || clickedAnOption;
        if (!insideAny) {
            for (size_t i = 0; i < options_.size() && !insideAny; ++i) {
                Rect optRect{bounds.x, header.y + rowH * (1 + i), bounds.w, rowH};
                insideAny = optRect.Contains(in.mouseX, in.mouseY);
            }
        }
        if (in.mousePressed && !insideAny) open_ = false;
    }
    openFade_.SetTarget(open_ ? 1.0f : 0.0f);
    openFade_.Update(dt);
}

void ComboBox::Render(IRenderer& r, const Theme& theme, Rect bounds) const {
    Rect header{bounds.x, bounds.y, bounds.w, theme.rowHeight};
    style::BeveledPanel(r, header, theme.cornerRadius * 0.6f, theme.panelAlt, theme.bevelStrength);
    r.Text(header.x + theme.padding, header.y + header.h * 0.5f - 3.5f, label_, theme.textDim, 1.0f);
    std::string val = options_.empty() ? "" : options_[selected_];
    float valW = r.TextWidth(val, 1.0f);
    r.Text(header.x + header.w - valW - 22.0f, header.y + header.h * 0.5f - 3.5f, val, theme.text, 1.0f);

    // chevron rotates a little as the dropdown opens, instead of just appearing
    float openT = openFade_.Value();
    float cx = header.x + header.w - 12.0f, cy = header.y + header.h * 0.5f;
    float flip = 1.0f - openT * 2.0f; // 1 -> -1 as it opens, flips the chevron upside down
    r.Line(cx - 4, cy - 2 * flip, cx, cy + 2 * flip, theme.textDim, 1.5f);
    r.Line(cx, cy + 2 * flip, cx + 4, cy - 2 * flip, theme.textDim, 1.5f);

    if (openT > 0.001f) {
        r.PushClip(bounds.x, header.y + header.h, bounds.w, bounds.h - header.h);
        for (size_t i = 0; i < options_.size(); ++i) {
            Rect optRect{bounds.x, header.y + header.h * (1 + static_cast<float>(i)), bounds.w, header.h};
            bool isSel = static_cast<int>(i) == selected_;
            r.FillRect(optRect.x, optRect.y, optRect.w, optRect.h, isSel ? theme.accentDim : theme.background);
            if (isSel) r.FillRect(optRect.x, optRect.y, 3.0f, optRect.h, theme.accent); // active-row accent strip
            r.Text(optRect.x + theme.padding, optRect.y + optRect.h * 0.5f - 3.5f, options_[i],
                   isSel ? theme.accent : theme.textDim, 1.0f);
        }
        r.PopClip();
    }
}

void ComboBox::SaveTo(ConfigStore& cfg) const { cfg.SetInt(id_, selected_); }
void ComboBox::LoadFrom(const ConfigStore& cfg) {
    selected_ = std::clamp(cfg.GetInt(id_, selected_), 0, static_cast<int>(options_.size()) - 1);
}

// -------------------------------------------------------------- RadioGroup

RadioGroup::RadioGroup(std::string id, std::string label, std::vector<std::string> options, int startIndex)
    : Widget(std::move(id)), label_(std::move(label)), options_(std::move(options)),
      selected_(std::clamp(startIndex, 0, static_cast<int>(options_.size()) - 1)) {}

float RadioGroup::Height(const Theme& theme) const { return theme.rowHeight + 24.0f; }

void RadioGroup::Update(const InputState& in, Rect bounds, float dt) {
    float btnY = bounds.y + 22.0f;
    float btnH = bounds.h - 22.0f;
    float gap = 6.0f;
    float btnW = (bounds.w - gap * (options_.size() - 1)) / static_cast<float>(options_.size());

    for (size_t i = 0; i < options_.size(); ++i) {
        Rect btn{bounds.x + (btnW + gap) * i, btnY, btnW, btnH};
        if (btn.Contains(in.mouseX, in.mouseY) && in.mousePressed) selected_ = static_cast<int>(i);
    }

    float selX = bounds.x + (btnW + gap) * selected_;
    bool firstFrame = !initialized_;
    selectorX_.SetTarget(selX, firstFrame);
    selectorW_.SetTarget(btnW, firstFrame);
    initialized_ = true;
    selectorX_.Update(dt);
    selectorW_.Update(dt);
}

void RadioGroup::Render(IRenderer& r, const Theme& theme, Rect bounds) const {
    r.Text(bounds.x + 2.0f, bounds.y, label_, theme.textDim, 1.0f);

    float btnY = bounds.y + 22.0f;
    float btnH = bounds.h - 22.0f;
    float gap = 6.0f;
    float btnW = (bounds.w - gap * (options_.size() - 1)) / static_cast<float>(options_.size());

    style::BeveledPanel(r, Rect{bounds.x, btnY, bounds.w, btnH}, theme.cornerRadius * 0.6f, theme.panelAlt, theme.bevelStrength);
    style::Glow(r, Rect{selectorX_.Value(), btnY, selectorW_.Value(), btnH}, theme.cornerRadius * 0.6f,
                theme.accent, theme.glowStrength * 0.6f);
    r.FillRoundedRectGradient(selectorX_.Value(), btnY, selectorW_.Value(), btnH, theme.cornerRadius * 0.6f,
                               theme.accentSoft, theme.accent);

    for (size_t i = 0; i < options_.size(); ++i) {
        float bx = bounds.x + (btnW + gap) * i;
        bool isSel = static_cast<int>(i) == selected_;
        float tw = r.TextWidth(options_[i], 1.0f);
        r.Text(bx + (btnW - tw) * 0.5f, btnY + btnH * 0.5f - 3.5f, options_[i],
               isSel ? Color(255, 255, 255, 255) : theme.textDim, 1.0f);
    }
}

void RadioGroup::SaveTo(ConfigStore& cfg) const { cfg.SetInt(id_, selected_); }
void RadioGroup::LoadFrom(const ConfigStore& cfg) {
    selected_ = std::clamp(cfg.GetInt(id_, selected_), 0, static_cast<int>(options_.size()) - 1);
}

// -------------------------------------------------------------- ColorPicker

ColorPicker::ColorPicker(std::string id, std::string label, Color startValue)
    : Widget(std::move(id)), label_(std::move(label)), value_(startValue) {
    value_.ToHSV(hue_, sat_, val_);
}

float ColorPicker::Height(const Theme& theme) const {
    return theme.rowHeight + 120.0f * openFade_.Value();
}

void ColorPicker::Update(const InputState& in, Rect bounds, float dt) {
    Rect header{bounds.x, bounds.y, bounds.w, DefaultTheme().rowHeight};
    if (header.Contains(in.mouseX, in.mouseY) && in.mousePressed) open_ = !open_;

    if (open_) {
        float panelY = header.y + header.h + 6.0f;
        float svSize = 90.0f;
        Rect svRect{bounds.x + 8, panelY, svSize, svSize};
        Rect hueRect{svRect.x + svSize + 8, panelY, 16.0f, svSize};

        if (svRect.Contains(in.mouseX, in.mouseY) && in.mousePressed) draggingSV_ = true;
        if (hueRect.Contains(in.mouseX, in.mouseY) && in.mousePressed) draggingHue_ = true;
        if (in.mouseReleased) draggingSV_ = draggingHue_ = false;

        if (draggingSV_) {
            sat_ = std::clamp((in.mouseX - svRect.x) / svRect.w, 0.0f, 1.0f);
            val_ = 1.0f - std::clamp((in.mouseY - svRect.y) / svRect.h, 0.0f, 1.0f);
        }
        if (draggingHue_) {
            hue_ = std::clamp((in.mouseY - hueRect.y) / hueRect.h, 0.0f, 1.0f) * 360.0f;
        }
        if (draggingSV_ || draggingHue_) value_ = Color::FromHSV(hue_, sat_, val_);
    }
    openFade_.SetTarget(open_ ? 1.0f : 0.0f);
    openFade_.Update(dt);
}

void ColorPicker::Render(IRenderer& r, const Theme& theme, Rect bounds) const {
    Rect header{bounds.x, bounds.y, bounds.w, theme.rowHeight};
    style::BeveledPanel(r, header, theme.cornerRadius * 0.6f, theme.panelAlt, theme.bevelStrength);
    r.Text(header.x + theme.padding, header.y + header.h * 0.5f - 3.5f, label_, theme.text, 1.0f);
    float swSize = 18.0f;
    float swX = header.x + header.w - swSize - theme.padding;
    float swY = header.y + (header.h - swSize) * 0.5f;
    style::Glow(r, Rect{swX, swY, swSize, swSize}, 4.0f, value_, theme.glowStrength * 0.5f);
    r.FillRoundedRectGradient(swX, swY, swSize, swSize, 4.0f,
                               Color::Lerp(value_, Color(255, 255, 255, 255), 0.25f), value_);
    r.StrokeRect(swX, swY, swSize, swSize, theme.border, 1.0f);

    float openT = openFade_.Value();
    if (openT > 0.01f) {
        r.PushClip(bounds.x, header.y + header.h, bounds.w, 120.0f * openT);
        float panelY = header.y + header.h + 6.0f;
        float svSize = 90.0f;
        const int grid = 18;
        float cell = svSize / grid;
        for (int gy = 0; gy < grid; ++gy) {
            for (int gx = 0; gx < grid; ++gx) {
                float s = (gx + 0.5f) / grid;
                float v = 1.0f - (gy + 0.5f) / grid;
                r.FillRect(bounds.x + 8 + gx * cell, panelY + gy * cell, cell + 0.5f, cell + 0.5f,
                           Color::FromHSV(hue_, s, v));
            }
        }
        float cursorX = bounds.x + 8 + sat_ * svSize;
        float cursorY = panelY + (1.0f - val_) * svSize;
        r.StrokeRect(cursorX - 4, cursorY - 4, 8, 8, Color(255, 255, 255, 255), 1.5f);

        float hueX = bounds.x + 8 + svSize + 8;
        for (int i = 0; i < 36; ++i) {
            r.FillRect(hueX, panelY + (svSize / 36.0f) * i, 16.0f, svSize / 36.0f + 0.5f,
                       Color::FromHSV(i * 10.0f, 1.0f, 1.0f));
        }
        float hueCursorY = panelY + (hue_ / 360.0f) * svSize;
        r.StrokeRect(hueX - 1, hueCursorY - 2, 18, 4, Color(255, 255, 255, 255), 1.5f);
        r.PopClip();
    }
}

void ColorPicker::SaveTo(ConfigStore& cfg) const { cfg.SetColor(id_, value_); }
void ColorPicker::LoadFrom(const ConfigStore& cfg) {
    value_ = cfg.GetColor(id_, value_);
    value_.ToHSV(hue_, sat_, val_);
}

// ------------------------------------------------------------------ Button

Button::Button(std::string id, std::string label, std::function<void()> onClick)
    : Widget(std::move(id)), label_(std::move(label)), onClick_(std::move(onClick)) {}

void Button::Update(const InputState& in, Rect bounds, float dt) {
    bool hovered = bounds.Contains(in.mouseX, in.mouseY);
    glow_.SetTarget(hovered ? 1.0f : 0.0f);
    glow_.Update(dt);
    if (hovered && in.mousePressed && !wasDown_ && onClick_) onClick_();
    wasDown_ = in.mouseDown;
}

void Button::Render(IRenderer& r, const Theme& theme, Rect bounds) const {
    Color bg = Color::Lerp(theme.panelAlt, theme.accent, glow_.Value() * 0.35f);
    if (glow_.Value() > 0.05f) {
        style::Glow(r, bounds, theme.cornerRadius * 0.6f, theme.accent, glow_.Value() * theme.glowStrength);
    }
    style::BeveledPanel(r, bounds, theme.cornerRadius * 0.6f, bg, theme.bevelStrength);
    float tw = r.TextWidth(label_, 1.0f);
    r.Text(bounds.x + (bounds.w - tw) * 0.5f, bounds.y + bounds.h * 0.5f - 3.5f, label_, theme.text, 1.0f);
}

// ----------------------------------------------------------------- Keybind

Keybind::Keybind(std::string id, std::string label, int startKey)
    : Widget(std::move(id)), label_(std::move(label)), key_(startKey) {}

void Keybind::Update(const InputState& in, Rect bounds, float dt) {
    bool hovered = bounds.Contains(in.mouseX, in.mouseY);
    glow_.SetTarget((hovered || listening_) ? 1.0f : 0.0f);
    glow_.Update(dt);

    if (hovered && in.mousePressed && !listening_) {
        listening_ = true;
        return; // swallow the click that opened capture mode
    }

    if (listening_ && in.keyPressed != 0) {
        key_ = in.keyPressed;
        listening_ = false;
        if (onChange_) onChange_(key_);
    }
}

void Keybind::Render(IRenderer& r, const Theme& theme, Rect bounds) const {
    Color rowBg = Color::Lerp(theme.background, theme.panelAlt, glow_.Value() * 0.6f);
    style::BeveledPanel(r, bounds, theme.cornerRadius * 0.6f, rowBg, theme.bevelStrength * 0.5f);
    r.Text(bounds.x + theme.padding, bounds.y + bounds.h * 0.5f - 3.5f, label_, theme.text, 1.0f);

    std::string display;
    if (listening_) display = "...";
    else if (keyName_) display = keyName_(key_);
    else if (key_ == 0) display = "NONE";
    else display = "KEY " + std::to_string(key_);

    float pillW = 70.0f, pillH = 20.0f;
    float pillX = bounds.x + bounds.w - pillW - theme.padding;
    float pillY = bounds.y + (bounds.h - pillH) * 0.5f;
    Color pillBg = listening_ ? theme.accentDim : theme.toggleOff;
    if (listening_) style::Glow(r, Rect{pillX, pillY, pillW, pillH}, pillH * 0.4f, theme.accent, theme.glowStrength);
    style::BeveledPanel(r, Rect{pillX, pillY, pillW, pillH}, pillH * 0.4f, pillBg, theme.bevelStrength);
    float tw = r.TextWidth(display, 1.0f);
    r.Text(pillX + (pillW - tw) * 0.5f, pillY + pillH * 0.5f - 3.5f, display,
           listening_ ? theme.accent : theme.text, 1.0f);
}

void Keybind::SaveTo(ConfigStore& cfg) const { cfg.SetInt(id_, key_); }
void Keybind::LoadFrom(const ConfigStore& cfg) { key_ = cfg.GetInt(id_, key_); }

// ------------------------------------------------------------------- Label

void Label::Render(IRenderer& r, const Theme& theme, Rect bounds) const {
    r.Text(bounds.x + 2.0f, bounds.y + bounds.h - 14.0f, text_, theme.textDim, 1.0f);
    r.FillRect(bounds.x, bounds.y + bounds.h - 2.0f, bounds.w, 1.0f, theme.border);
}

} // namespace cui
