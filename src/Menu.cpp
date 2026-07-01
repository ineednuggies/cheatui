#include "Menu.hpp"
#include <algorithm>

namespace cui {

Menu::Menu(std::string title, Rect bounds)
    : title_(std::move(title)), bounds_(bounds), indicatorY_(0.0f, 0.2f, easing::CubicInOut) {
    liveAccent_ = theme_.accent;
}

Tab& Menu::AddTab(const std::string& name) {
    tabs_.push_back(std::make_unique<Tab>(name));
    if (tabs_.size() == 1) indicatorY_.SetTarget(0.0f, true);
    return *tabs_.back();
}

void Menu::Update(const InputState& in, float dt) {
    if (!visible_) return;

    if (theme_.rainbowAccent) {
        liveAccent_ = hueCycler_.Update(dt);
        theme_.accent = liveAccent_;
        theme_.accentDim = Color(liveAccent_.r, liveAccent_.g, liveAccent_.b, 90);
    }

    Rect titleBar = TitleBarRect();
    if (in.mousePressed && titleBar.Contains(in.mouseX, in.mouseY)) {
        draggingWindow_ = true;
        dragOffsetX_ = in.mouseX - bounds_.x;
        dragOffsetY_ = in.mouseY - bounds_.y;
    }
    if (in.mouseReleased) draggingWindow_ = false;
    if (draggingWindow_) {
        bounds_.x = in.mouseX - dragOffsetX_;
        bounds_.y = in.mouseY - dragOffsetY_;
    }

    Rect sidebar = SidebarRect();
    float tabH = 34.0f;
    for (size_t i = 0; i < tabs_.size(); ++i) {
        Rect tabRect{sidebar.x, sidebar.y + tabH * static_cast<float>(i), sidebar.w, tabH};
        if (tabRect.Contains(in.mouseX, in.mouseY) && in.mousePressed) {
            active_ = static_cast<int>(i);
            indicatorY_.SetTarget(sidebar.y + tabH * static_cast<float>(i));
        }
    }
    indicatorY_.Update(dt);

    if (!tabs_.empty()) tabs_[active_]->Update(in, ContentRect(), theme_, dt);
}

void Menu::Render(IRenderer& r) const {
    if (!visible_) return;

    r.FillRoundedRect(bounds_.x, bounds_.y, bounds_.w, bounds_.h, theme_.cornerRadius, theme_.background);
    r.StrokeRect(bounds_.x, bounds_.y, bounds_.w, bounds_.h, theme_.border, 1.0f);

    Rect titleBar = TitleBarRect();
    r.FillRoundedRect(titleBar.x, titleBar.y, titleBar.w, titleBar.h, theme_.cornerRadius, theme_.sidebar);
    r.Text(titleBar.x + theme_.padding, titleBar.y + titleBar.h * 0.5f - 3.5f, title_, theme_.text, 1.0f);
    r.FillCircle(titleBar.x + titleBar.w - 16.0f, titleBar.y + titleBar.h * 0.5f, 3.0f, theme_.accent);

    Rect sidebar = SidebarRect();
    r.FillRect(sidebar.x, sidebar.y, sidebar.w, sidebar.h, theme_.sidebar);

    float tabH = 34.0f;
    // animated selection indicator: a thin accent bar that slides to the active tab
    r.FillRect(sidebar.x, indicatorY_.Value(), 3.0f, tabH, theme_.accent);

    for (size_t i = 0; i < tabs_.size(); ++i) {
        Rect tabRect{sidebar.x, sidebar.y + tabH * static_cast<float>(i), sidebar.w, tabH};
        bool isActive = static_cast<int>(i) == active_;
        if (isActive) r.FillRect(tabRect.x, tabRect.y, tabRect.w, tabRect.h, theme_.accentDim);
        r.Text(tabRect.x + theme_.padding + 6.0f, tabRect.y + tabH * 0.5f - 3.5f, tabs_[i]->Name(),
               isActive ? theme_.text : theme_.textDim, 1.0f);
    }

    r.FillRect(sidebar.x + sidebar.w, sidebar.y, 1.0f, sidebar.h, theme_.border);

    if (!tabs_.empty()) {
        Rect content = ContentRect();
        r.PushClip(content.x - theme_.padding, content.y - theme_.padding,
                   content.w + theme_.padding * 2.0f, content.h + theme_.padding * 2.0f);
        tabs_[active_]->Render(r, theme_, content);
        r.PopClip();
    }
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
