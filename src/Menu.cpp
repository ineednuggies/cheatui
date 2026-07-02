#include "Menu.hpp"
#include "Style.hpp"
#include <algorithm>

namespace cui {

namespace {
constexpr float kTabRowHeight = 34.0f;
constexpr float kChromeButtonSize = 20.0f;
constexpr float kChromeButtonGap = 6.0f;
}

Menu::Menu(std::string title, Rect bounds)
    : title_(std::move(title)), bounds_(bounds), indicatorY_(0.0f, 0.2f, easing::CubicInOut) {
    liveAccent_ = theme_.accent;
}

Tab& Menu::AddTab(const std::string& name) {
    tabs_.push_back(std::make_unique<Tab>(name));
    if (tabs_.size() == 1) indicatorY_.SetTarget(0.0f, true);
    return *tabs_.back();
}

void Menu::AddTitleBarButton(const std::string& glyph, std::function<void()> onClick) {
    TitleBarButton b;
    b.glyph = glyph;
    b.onClick = std::move(onClick);
    extraButtons_.push_back(std::move(b));
}

void Menu::SetVisible(bool v) {
    wantVisible_ = v;
    openAnim_.SetTarget(v ? 1.0f : 0.0f);
}

std::vector<Menu::ButtonSlot> Menu::LayoutTitleButtons() const {
    std::vector<ButtonSlot> slots;
    Rect tb = TitleBarRect();
    float cursorX = tb.x + tb.w - theme_.padding - kChromeButtonSize;
    float y = tb.y + (tb.h - kChromeButtonSize) * 0.5f;

    if (onClose_) {
        slots.push_back({Rect{cursorX, y, kChromeButtonSize, kChromeButtonSize}, "X", closeHover_.Value(), 0, -1});
        cursorX -= kChromeButtonSize + kChromeButtonGap;
    }
    if (showMinimize_) {
        std::string glyph = collapsed_ ? "+" : "-";
        slots.push_back({Rect{cursorX, y, kChromeButtonSize, kChromeButtonSize}, glyph, minimizeHover_.Value(), 1, -1});
        cursorX -= kChromeButtonSize + kChromeButtonGap;
    }
    for (int i = static_cast<int>(extraButtons_.size()) - 1; i >= 0; --i) {
        slots.push_back({Rect{cursorX, y, kChromeButtonSize, kChromeButtonSize}, extraButtons_[i].glyph,
                          extraButtons_[i].hover.Value(), 2, i});
        cursorX -= kChromeButtonSize + kChromeButtonGap;
    }
    return slots;
}

void Menu::Update(const InputState& in, float dt) {
    openAnim_.Update(dt);
    if (!wantVisible_ && openAnim_.Value() <= 0.001f) return; // fully closed, nothing to do

    if (theme_.rainbowAccent) {
        liveAccent_ = hueCycler_.Update(dt);
        theme_.accent = liveAccent_;
        theme_.accentDim = Color(liveAccent_.r, liveAccent_.g, liveAccent_.b, 90);
        theme_.accentSoft = Color::Lerp(liveAccent_, Color(255, 255, 255, 255), 0.35f);
    }
    ambientPulse_.Update(dt);

    if (!wantVisible_) return; // fading out - let it animate, but stop taking input

    Rect titleBar = TitleBarRect();
    bool overChrome = false;
    for (auto& slot : LayoutTitleButtons()) {
        if (slot.rect.Contains(in.mouseX, in.mouseY)) { overChrome = true; break; }
    }

    if (in.mousePressed && !overChrome && titleBar.Contains(in.mouseX, in.mouseY)) {
        draggingWindow_ = true;
        dragOffsetX_ = in.mouseX - bounds_.x;
        dragOffsetY_ = in.mouseY - bounds_.y;
    }
    if (in.mouseReleased) draggingWindow_ = false;
    if (draggingWindow_) {
        bounds_.x = in.mouseX - dragOffsetX_;
        bounds_.y = in.mouseY - dragOffsetY_;
    }

    // Chrome buttons: update each one's own hover animation and fire on click.
    for (auto& slot : LayoutTitleButtons()) {
        bool hovered = slot.rect.Contains(in.mouseX, in.mouseY);
        AnimatedFloat* hoverAnim = nullptr;
        if (slot.kind == 0) hoverAnim = &closeHover_;
        else if (slot.kind == 1) hoverAnim = &minimizeHover_;
        else hoverAnim = &extraButtons_[slot.customIndex].hover;

        hoverAnim->SetTarget(hovered ? 1.0f : 0.0f);
        hoverAnim->Update(dt);

        if (hovered && in.mousePressed) {
            if (slot.kind == 0 && onClose_) onClose_();
            else if (slot.kind == 1) { collapsed_ = !collapsed_; collapseAnim_.SetTarget(collapsed_ ? 1.0f : 0.0f); }
            else if (slot.kind == 2 && extraButtons_[slot.customIndex].onClick) extraButtons_[slot.customIndex].onClick();
        }
    }
    collapseAnim_.Update(dt);

    if (!collapsed_) {
        Rect sidebar = SidebarRect();
        for (size_t i = 0; i < tabs_.size(); ++i) {
            Rect tabRect{sidebar.x, sidebar.y + kTabRowHeight * static_cast<float>(i), sidebar.w, kTabRowHeight};
            if (tabRect.Contains(in.mouseX, in.mouseY) && in.mousePressed && !overChrome) {
                active_ = static_cast<int>(i);
                indicatorY_.SetTarget(sidebar.y + kTabRowHeight * static_cast<float>(i));
            }
        }
        indicatorY_.Update(dt);
        if (!tabs_.empty()) tabs_[active_]->Update(in, ContentRect(), theme_, dt);
    }
}

void Menu::Render(IRenderer& r) const {
    float windowAlpha = openAnim_.Value();
    if (windowAlpha <= 0.001f) return;

    // Fade every themed color together so open/close is a real cross-fade,
    // not just a hard cut once the animation crosses some threshold.
    Theme faded = theme_;
    auto fade = [windowAlpha](Color c) { c.a = static_cast<uint8_t>(c.a * windowAlpha); return c; };
    faded.background = fade(theme_.background);
    faded.sidebar = fade(theme_.sidebar);
    faded.panelAlt = fade(theme_.panelAlt);
    faded.border = fade(theme_.border);
    faded.text = fade(theme_.text);
    faded.textDim = fade(theme_.textDim);
    faded.accent = fade(theme_.accent);
    faded.accentDim = fade(theme_.accentDim);
    faded.accentSoft = fade(theme_.accentSoft);
    faded.toggleOff = fade(theme_.toggleOff);

    float slideOffset = (1.0f - windowAlpha) * 8.0f; // gentle upward slide while fading in/out
    float bodyHeight = (bounds_.h - 36.0f) * (1.0f - collapseAnim_.Value());
    float windowHeight = 36.0f + bodyHeight;

    Rect windowRect{bounds_.x, bounds_.y + slideOffset, bounds_.w, windowHeight};

    style::Glow(r, windowRect, theme_.cornerRadius, theme_.accent,
                (0.15f + ambientPulse_.Value() * 0.1f) * theme_.glowStrength * windowAlpha);
    r.FillRoundedRect(windowRect.x, windowRect.y, windowRect.w, windowRect.h, theme_.cornerRadius, faded.background);
    r.StrokeRect(windowRect.x, windowRect.y, windowRect.w, windowRect.h, faded.border, 1.0f);

    Rect titleBar{windowRect.x, windowRect.y, windowRect.w, 36.0f};
    style::BeveledPanel(r, titleBar, theme_.cornerRadius, faded.sidebar, theme_.bevelStrength * 0.6f);
    r.Text(titleBar.x + theme_.padding, titleBar.y + titleBar.h * 0.5f - 3.5f, title_, faded.text, 1.0f);

    // Chrome buttons (close / minimize / custom), right-aligned in the title bar.
    for (auto& slot : LayoutTitleButtons()) {
        Rect btn = slot.rect;
        btn.y += slideOffset;
        Color base = slot.kind == 0
            ? Color::Lerp(theme_.panelAlt, theme_.danger, slot.hoverValue * 0.7f)
            : Color::Lerp(theme_.panelAlt, theme_.accent, slot.hoverValue * 0.5f);
        if (slot.hoverValue > 0.05f) {
            style::Glow(r, btn, 5.0f, slot.kind == 0 ? theme_.danger : theme_.accent,
                        slot.hoverValue * theme_.glowStrength * windowAlpha);
        }
        style::BeveledPanel(r, btn, 5.0f, fade(base), theme_.bevelStrength);
        float glyphW = r.TextWidth(slot.glyph, 1.0f);
        r.Text(btn.x + (btn.w - glyphW) * 0.5f, btn.y + btn.h * 0.5f - 3.5f, slot.glyph, faded.text, 1.0f);
    }

    if (bodyHeight < 1.0f) return; // fully collapsed - just the title bar, nothing below it

    r.PushClip(windowRect.x, titleBar.y + titleBar.h, windowRect.w, bodyHeight);

    Rect sidebar{windowRect.x, titleBar.y + titleBar.h, theme_.tabWidth, bodyHeight};
    r.FillRect(sidebar.x, sidebar.y, sidebar.w, sidebar.h, faded.sidebar);

    // The active-tab indicator: a soft glowing bar that slides to whichever
    // tab is selected, with a slow shimmer running along its gradient.
    float shimmer = ambientPulse_.Value();
    Color indicatorTop = Color::Lerp(theme_.accent, theme_.accentSoft, shimmer);
    Rect indicatorRect{sidebar.x, indicatorY_.Value() + slideOffset, 3.0f, kTabRowHeight};
    style::Glow(r, indicatorRect, 1.5f, theme_.accent, (0.5f + shimmer * 0.3f) * theme_.glowStrength * windowAlpha);
    r.FillRoundedRectGradient(indicatorRect.x, indicatorRect.y, indicatorRect.w, indicatorRect.h, 1.5f,
                               fade(indicatorTop), fade(theme_.accent));

    for (size_t i = 0; i < tabs_.size(); ++i) {
        Rect tabRect{sidebar.x, sidebar.y + kTabRowHeight * static_cast<float>(i), sidebar.w, kTabRowHeight};
        bool isActive = static_cast<int>(i) == active_;
        if (isActive) r.FillRect(tabRect.x, tabRect.y, tabRect.w, tabRect.h, faded.accentDim);
        r.Text(tabRect.x + theme_.padding + 6.0f, tabRect.y + kTabRowHeight * 0.5f - 3.5f, tabs_[i]->Name(),
               isActive ? faded.text : faded.textDim, 1.0f);
    }

    r.FillRect(sidebar.x + sidebar.w, sidebar.y, 1.0f, sidebar.h, faded.border);

    if (!tabs_.empty()) {
        Rect content{sidebar.x + sidebar.w + theme_.padding, sidebar.y + theme_.padding,
                     windowRect.w - sidebar.w - theme_.padding * 2.0f, bodyHeight - theme_.padding * 2.0f};
        r.PushClip(content.x - theme_.padding, content.y - theme_.padding,
                   content.w + theme_.padding * 2.0f, content.h + theme_.padding * 2.0f);
        tabs_[active_]->Render(r, faded, content);
        r.PopClip();
    }

    r.PopClip();
}

bool Menu::SaveProfile(const std::string& dir, const std::string& name) const {
    ConfigStore store;
    for (auto& tab : tabs_) tab->SaveAll(store);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return store.SaveToFile(dir + "/" + name + ".cfg");
}

bool Menu::LoadProfile(const std::string& dir, const std::string& name) {
    ConfigStore store;
    if (!store.LoadFromFile(dir + "/" + name + ".cfg")) return false;
    for (auto& tab : tabs_) tab->LoadAll(store);
    return true;
}

} // namespace cui
