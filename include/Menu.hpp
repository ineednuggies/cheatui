#pragma once
#include "Tab.hpp"
#include "Theme.hpp"
#include "Animation.hpp"
#include "Config.hpp"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace cui {

// A small round button in the title bar - close, minimize, or anything
// else you want up there (a pin, a settings gear, whatever). Menu owns a
// list of these and draws/hit-tests them itself.
struct TitleBarButton {
    std::string glyph;              // short label drawn centered in the button, e.g. "X" or "-"
    std::function<void()> onClick;
    AnimatedFloat hover{0.0f, 0.12f};
};

// The whole menu window: draggable title bar, sidebar of tabs with a
// sliding selection indicator, content panel for the active tab's widgets.
// Title bar, sidebar, and content are each their own rounded floating
// panel with real gaps between them, so nothing needs corner-matching
// against its neighbor - every panel is just uniformly rounded. This is
// the class an application actually creates and drives.
class Menu {
public:
    Menu(std::string title, Rect bounds);

    Tab& AddTab(const std::string& name);

    void Update(const InputState& in, float dt);
    void Render(IRenderer& r) const;

    // Fades/pops the whole window in or out over a couple of frames rather
    // than snapping - host app still just flips this on a hotkey, the
    // animation is automatic.
    void SetVisible(bool v);
    bool Visible() const { return wantVisible_; }
    void ToggleVisible() { SetVisible(!wantVisible_); }

    // A minimize ("-") button is shown by default; clicking it collapses
    // the window down to just its title bar. No callback needed - this is
    // handled entirely inside Menu.
    void SetCollapsible(bool enabled) { showMinimize_ = enabled; }
    bool IsCollapsed() const { return collapsed_; }

    // No close button is shown until you set a callback for it - closing
    // usually means something app-specific (hide the menu? quit? both?),
    // so that decision is left to whoever owns the Menu.
    void SetOnClose(std::function<void()> onClose) { onClose_ = std::move(onClose); }

    // Add your own title bar button (a settings gear, a pin, anything).
    // Buttons appear left of minimize/close, in the order added.
    void AddTitleBarButton(const std::string& glyph, std::function<void()> onClick);

    // Direct mutation - GetTheme().accent = ...; - takes effect next frame
    // with no transition. For a smooth cross-fade between two whole
    // palettes (including switching to one of the cui::themes:: presets),
    // use SetTheme() instead.
    Theme& GetTheme() { return theme_; }
    const Theme& GetTheme() const { return theme_; }

    // Replaces the whole theme. By default every themed color cross-fades
    // from its current (possibly mid-transition) value to the new theme's
    // over theme.themeTransitionTime seconds, instead of snapping.
    void SetTheme(const Theme& newTheme, bool animate = true);

    const Rect& Bounds() const { return bounds_; }

    // Config profile management - pushes every bound widget's current value
    // into a ConfigStore and writes it to <dir>/<name>.cfg, or the reverse.
    bool SaveProfile(const std::string& dir, const std::string& name) const;
    bool LoadProfile(const std::string& dir, const std::string& name);
    std::vector<std::string> ListProfiles(const std::string& dir) const { return ConfigStore::ListProfiles(dir); }

private:
    static constexpr float kTitleBarHeight = 36.0f;
    static constexpr float kTabRowHeight = 34.0f;

    std::string title_;
    Rect bounds_;
    Theme theme_;
    std::vector<std::unique_ptr<Tab>> tabs_;
    int active_ = 0;
    int previousActive_ = 0;
    AnimatedFloat tabSwitchAnim_{1.0f, 0.2f, easing::CubicInOut}; // 0 = just switched, 1 = settled

    bool wantVisible_ = true;
    AnimatedFloat openAnim_{1.0f, 0.16f, easing::CubicOut}; // 0 = closed, 1 = fully open

    bool collapsed_ = false;
    bool showMinimize_ = true;
    AnimatedFloat collapseAnim_{0.0f, 0.2f, easing::CubicInOut}; // 0 = expanded, 1 = collapsed

    AnimatedFloat indicatorY_;
    HueCycler hueCycler_{45.0f};
    Oscillator ambientPulse_{0.25f};
    Color liveAccent_;

    // Theme cross-fade: themeFrom_ is where the fade started, theme_ is
    // the target, themeTransition_ is how far along [0,1] we are.
    Theme themeFrom_;
    AnimatedFloat themeTransition_{1.0f, 0.35f, easing::CubicInOut};

    std::function<void()> onClose_;
    AnimatedFloat closeHover_{0.0f, 0.12f};
    AnimatedFloat minimizeHover_{0.0f, 0.12f};
    std::vector<TitleBarButton> extraButtons_;

    bool draggingWindow_ = false;
    float dragOffsetX_ = 0, dragOffsetY_ = 0;

    // Body height (sidebar + content row), already scaled by the collapse
    // animation - 0 when fully collapsed, full height when expanded.
    float BodyHeight() const;

    Rect TitleBarRect() const {
        float m = theme_.windowMargin;
        return {bounds_.x + m, bounds_.y + m, bounds_.w - m * 2.0f, kTitleBarHeight};
    }
    Rect SidebarRect() const {
        float m = theme_.windowMargin;
        float top = bounds_.y + m + kTitleBarHeight + theme_.sectionGap * (1.0f - collapseAnim_.Value());
        return {bounds_.x + m, top, theme_.tabWidth, BodyHeight()};
    }
    Rect ContentRect() const {
        Rect sb = SidebarRect();
        float m = theme_.windowMargin;
        float contentX = sb.x + sb.w + theme_.sectionGap;
        float right = bounds_.x + bounds_.w - m;
        return {contentX, sb.y, right - contentX > 0.0f ? right - contentX : 0.0f, sb.h};
    }
    // The content card's rect, minus its own inner padding - this is the
    // actual area tab widgets are laid out in. Update() and Render() both
    // go through this so hit-testing always matches what got drawn.
    Rect InnerContentRect() const {
        Rect c = ContentRect();
        float p = theme_.padding;
        return {c.x + p, c.y + p, c.w - p * 2.0f > 0.0f ? c.w - p * 2.0f : 0.0f,
                c.h - p * 2.0f > 0.0f ? c.h - p * 2.0f : 0.0f};
    }

    struct ButtonSlot {
        Rect rect;
        std::string glyph;
        float hoverValue;
        int kind;        // 0 = close, 1 = minimize, 2 = custom
        int customIndex; // valid when kind == 2
    };
    // Lays out every title bar button right-to-left (close, minimize, then
    // custom buttons). One function shared by hit-testing and drawing so
    // they can never drift out of sync with each other.
    std::vector<ButtonSlot> LayoutTitleButtons() const;
};

} // namespace cui
