#include "Menu.hpp"
#include "Style.hpp"
#include <algorithm>

namespace cui {

namespace {
constexpr float kChromeButtonSize = 20.0f;
constexpr float kChromeButtonGap = 6.0f;

Color ScaleAlpha(Color c, float k) {
    c.a = static_cast<uint8_t>(std::clamp(c.a * k, 0.0f, 255.0f));
    return c;
}

// Blends every color field between two themes, and eases the few numeric
// knobs (corner radius, bevel/glow strength) along with them, so a
// Menu::SetTheme() call is a real cross-fade rather than a hard cut.
// Layout-affecting fields (padding, tab width, margins...) come straight
// from the target theme - those rarely differ between presets, and
// animating rects is a lot more machinery for very little visible gain.
Theme LerpTheme(const Theme& a, const Theme& b, float t) {
    Theme out = b;
    out.background = Color::Lerp(a.background, b.background, t);
    out.sidebar = Color::Lerp(a.sidebar, b.sidebar, t);
    out.panelAlt = Color::Lerp(a.panelAlt, b.panelAlt, t);
    out.border = Color::Lerp(a.border, b.border, t);
    out.text = Color::Lerp(a.text, b.text, t);
    out.textDim = Color::Lerp(a.textDim, b.textDim, t);
    out.accent = Color::Lerp(a.accent, b.accent, t);
    out.accentDim = Color::Lerp(a.accentDim, b.accentDim, t);
    out.accentSoft = Color::Lerp(a.accentSoft, b.accentSoft, t);
    out.toggleOff = Color::Lerp(a.toggleOff, b.toggleOff, t);
    out.danger = Color::Lerp(a.danger, b.danger, t);
    out.success = Color::Lerp(a.success, b.success, t);
    out.cornerRadius = a.cornerRadius + (b.cornerRadius - a.cornerRadius) * t;
    out.bevelStrength = a.bevelStrength + (b.bevelStrength - a.bevelStrength) * t;
    out.glowStrength = a.glowStrength + (b.glowStrength - a.glowStrength) * t;
    return out;
}
} // namespace

Menu::Menu(std::string title, Rect bounds)
    : title_(std::move(title)), bounds_(bounds), indicatorY_(0.0f, 0.2f, easing::CubicInOut) {
    liveAccent_ = theme_.accent;
    themeFrom_ = theme_;
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

void Menu::SetTheme(const Theme& newTheme, bool animate) {
    if (animate) {
        // Capture wherever the current (possibly still mid-fade) blend is
        // as the new starting point, so re-theming mid-transition doesn't
        // jump.
        themeFrom_ = LerpTheme(themeFrom_, theme_, themeTransition_.Value());
        themeTransition_.SetDuration(newTheme.themeTransitionTime > 0.0f ? newTheme.themeTransitionTime : 0.35f);
        theme_ = newTheme;
        themeTransition_.SetTarget(0.0f, true);
        themeTransition_.SetTarget(1.0f);
    } else {
        theme_ = newTheme;
        themeFrom_ = newTheme;
        themeTransition_.SetTarget(1.0f, true);
    }
}

float Menu::BodyHeight() const {
    float full = bounds_.h - theme_.windowMargin * 2.0f - kTitleBarHeight - theme_.sectionGap;
    if (full < 0.0f) full = 0.0f;
    return full * (1.0f - collapseAnim_.Value());
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
    themeTransition_.Update(dt);
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
                int clicked = static_cast<int>(i);
                if (clicked != active_) {
                    previousActive_ = active_;
                    active_ = clicked;
                    tabSwitchAnim_.SetTarget(0.0f, true);
                    tabSwitchAnim_.SetTarget(1.0f);
                }
                indicatorY_.SetTarget(sidebar.y + kTabRowHeight * static_cast<float>(i));
            }
        }
        indicatorY_.Update(dt);
        tabSwitchAnim_.Update(dt);
        if (!tabs_.empty()) tabs_[active_]->Update(in, InnerContentRect(), theme_, dt);
    }
}

void Menu::Render(IRenderer& r) const {
    float windowAlpha = openAnim_.Value();
    if (windowAlpha <= 0.001f) return;

    Theme live = LerpTheme(themeFrom_, theme_, themeTransition_.Value());

    Theme faded = live;
    auto fade = [windowAlpha](Color c) { return ScaleAlpha(c, windowAlpha); };
    faded.background = fade(live.background);
    faded.sidebar = fade(live.sidebar);
    faded.panelAlt = fade(live.panelAlt);
    faded.border = fade(live.border);
    faded.text = fade(live.text);
    faded.textDim = fade(live.textDim);
    faded.accent = fade(live.accent);
    faded.accentDim = fade(live.accentDim);
    faded.accentSoft = fade(live.accentSoft);
    faded.toggleOff = fade(live.toggleOff);

    float slideOffset = (1.0f - windowAlpha) * 8.0f; // gentle upward slide while fading in/out
    float bodyH = BodyHeight();
    float gapNow = live.sectionGap * (1.0f - collapseAnim_.Value());
    float windowHeight = live.windowMargin * 2.0f + kTitleBarHeight + gapNow + bodyH;
    Rect windowRect{bounds_.x, bounds_.y + slideOffset, bounds_.w, windowHeight};

    // Outer backdrop - a dim rounded frame the real panels float above.
    style::Glow(r, windowRect, live.cornerRadius, live.accent,
                (0.12f + ambientPulse_.Value() * 0.08f) * live.glowStrength * windowAlpha);
    r.FillRoundedRect(windowRect.x, windowRect.y, windowRect.w, windowRect.h, live.cornerRadius,
                       ScaleAlpha(faded.background, 0.6f));
    r.StrokeRect(windowRect.x, windowRect.y, windowRect.w, windowRect.h, faded.border, 1.0f);

    // Title bar - its own fully-rounded floating card.
    Rect titleBar = TitleBarRect();
    titleBar.y += slideOffset;
    style::BeveledPanel(r, titleBar, live.cornerRadius, faded.sidebar, live.bevelStrength * 0.6f);
    r.Text(titleBar.x + live.padding, titleBar.y + titleBar.h * 0.5f - (3.5f * live.textScale), title_, faded.text, live.textScale);

    for (auto& slot : LayoutTitleButtons()) {
        Rect btn = slot.rect;
        btn.y += slideOffset;
        Color base = slot.kind == 0
            ? Color::Lerp(live.panelAlt, live.danger, slot.hoverValue * 0.7f)
            : Color::Lerp(live.panelAlt, live.accent, slot.hoverValue * 0.5f);
        if (slot.hoverValue > 0.05f) {
            style::Glow(r, btn, 5.0f, slot.kind == 0 ? live.danger : live.accent,
                        slot.hoverValue * live.glowStrength * windowAlpha);
        }
        style::BeveledPanel(r, btn, 5.0f, fade(base), live.bevelStrength);
        float glyphW = r.TextWidth(slot.glyph, live.textScale);
        r.Text(btn.x + (btn.w - glyphW) * 0.5f, btn.y + btn.h * 0.5f - (3.5f * live.textScale), slot.glyph, faded.text, live.textScale);
    }

    if (bodyH < 1.0f) return; // fully collapsed - just the title bar

    // Sidebar - its own fully-rounded floating card.
    Rect sidebar = SidebarRect();
    sidebar.y += slideOffset;
    style::BeveledPanel(r, sidebar, live.cornerRadius, faded.sidebar, live.bevelStrength * 0.5f);

    r.PushClip(sidebar.x, sidebar.y, sidebar.w, sidebar.h);
    float shimmer = ambientPulse_.Value();
    Color indicatorTop = Color::Lerp(live.accent, live.accentSoft, shimmer);
    Rect indicatorRect{sidebar.x, indicatorY_.Value() + slideOffset, 3.0f, kTabRowHeight};
    style::Glow(r, indicatorRect, 1.5f, live.accent, (0.5f + shimmer * 0.3f) * live.glowStrength * windowAlpha);
    r.FillRoundedRectGradient(indicatorRect.x, indicatorRect.y, indicatorRect.w, indicatorRect.h, 1.5f,
                               fade(indicatorTop), fade(live.accent));

    for (size_t i = 0; i < tabs_.size(); ++i) {
        Rect tabRect{sidebar.x, sidebar.y + kTabRowHeight * static_cast<float>(i), sidebar.w, kTabRowHeight};
        bool isActive = static_cast<int>(i) == active_;
        if (isActive) {
            bool isFirst = (i == 0), isLast = (i + 1 == tabs_.size());
            if (isFirst || isLast) {
                // The first/last row's highlight sits flush against the
                // sidebar's own rounded corner - round the same corners on
                // the highlight itself, or its square edge pokes past the
                // sidebar's curve and looks like a rounding bug.
                Corners c(0.0f, 0.0f, 0.0f, 0.0f);
                if (isFirst) { c.tl = live.cornerRadius; c.tr = live.cornerRadius; }
                if (isLast) { c.bl = live.cornerRadius; c.br = live.cornerRadius; }
                r.FillRoundedRectEx(tabRect.x, tabRect.y, tabRect.w, tabRect.h, c, faded.accentDim);
            } else {
                r.FillRect(tabRect.x, tabRect.y, tabRect.w, tabRect.h, faded.accentDim);
            }
        }
        r.Text(tabRect.x + live.padding + 6.0f, tabRect.y + kTabRowHeight * 0.5f - (3.5f * live.textScale), tabs_[i]->Name(),
               isActive ? faded.text : faded.textDim, live.textScale);
    }
    r.PopClip();

    if (tabs_.empty()) return;

    // Content - its own fully-rounded floating card, with the active tab's
    // widgets crossfading/sliding in while the previous tab's fade out.
    Rect content = ContentRect();
    content.y += slideOffset;
    style::BeveledPanel(r, content, live.cornerRadius, faded.background, live.bevelStrength * 0.4f);

    Rect innerContent = InnerContentRect();
    innerContent.y += slideOffset;
    r.PushClip(content.x, content.y, content.w, content.h);

    float switchT = tabSwitchAnim_.Value();
    if (switchT < 0.999f && previousActive_ != active_ && previousActive_ < static_cast<int>(tabs_.size())) {
        float outT = 1.0f - switchT;
        float inT = switchT;

        Theme outTheme = faded, inTheme = faded;
        auto applyFade = [](Theme& t, float k) {
            t.text = ScaleAlpha(t.text, k);
            t.textDim = ScaleAlpha(t.textDim, k);
            t.accent = ScaleAlpha(t.accent, k);
            t.accentDim = ScaleAlpha(t.accentDim, k);
            t.accentSoft = ScaleAlpha(t.accentSoft, k);
            t.panelAlt = ScaleAlpha(t.panelAlt, k);
            t.toggleOff = ScaleAlpha(t.toggleOff, k);
            t.background = ScaleAlpha(t.background, k);
            t.border = ScaleAlpha(t.border, k);
        };
        applyFade(outTheme, outT);
        applyFade(inTheme, inT);

        Rect outRect = innerContent; outRect.x -= 14.0f * switchT;
        Rect inRect = innerContent; inRect.x += 14.0f * outT;

        tabs_[previousActive_]->Render(r, outTheme, outRect);
        tabs_[active_]->Render(r, inTheme, inRect);
    } else {
        tabs_[active_]->Render(r, faded, innerContent);
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
