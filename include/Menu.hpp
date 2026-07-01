#pragma once
#include "Tab.hpp"
#include "Theme.hpp"
#include "Animation.hpp"
#include "Config.hpp"
#include <vector>
#include <memory>
#include <string>

namespace cui {

// The whole menu window: draggable title bar, sidebar of tabs with a
// sliding selection indicator, content panel for the active tab's widgets.
// This is the class an application actually creates and drives.
class Menu {
public:
    Menu(std::string title, Rect bounds);

    Tab& AddTab(const std::string& name);

    // visible: whether the menu should currently be drawn/processed at all
    // (host app usually ties this to an insert-key toggle).
    void Update(const InputState& in, float dt);
    void Render(IRenderer& r) const;

    void SetVisible(bool v) { visible_ = v; }
    bool Visible() const { return visible_; }
    void ToggleVisible() { visible_ = !visible_; }

    Theme& GetTheme() { return theme_; }
    const Rect& Bounds() const { return bounds_; }

    // Config profile management - pushes every bound widget's current value
    // into a ConfigStore and writes it to <dir>/<name>.cfg, or the reverse.
    bool SaveProfile(const std::string& dir, const std::string& name) const;
    bool LoadProfile(const std::string& dir, const std::string& name);
    std::vector<std::string> ListProfiles(const std::string& dir) const { return ConfigStore::ListProfiles(dir); }

private:
    std::string title_;
    Rect bounds_;
    Theme theme_;
    std::vector<std::unique_ptr<Tab>> tabs_;
    int active_ = 0;
    bool visible_ = true;

    mutable AnimatedFloat indicatorY_;
    mutable HueCycler hueCycler_{45.0f};
    mutable Color liveAccent_;

    bool draggingWindow_ = false;
    float dragOffsetX_ = 0, dragOffsetY_ = 0;

    Rect TitleBarRect() const { return {bounds_.x, bounds_.y, bounds_.w, 36.0f}; }
    Rect SidebarRect() const { return {bounds_.x, bounds_.y + 36.0f, theme_.tabWidth, bounds_.h - 36.0f}; }
    Rect ContentRect() const {
        Rect sb = SidebarRect();
        float pad = theme_.padding;
        return {sb.x + sb.w + pad, bounds_.y + 36.0f + pad, bounds_.w - sb.w - pad * 2.0f, bounds_.h - 36.0f - pad * 2.0f};
    }
};

} // namespace cui
