#pragma once
#include "Widgets.hpp"
#include <memory>
#include <vector>
#include <string>

namespace cui {

// One page of widgets, laid out top to bottom. Menu owns a list of these
// and shows whichever one is active.
class Tab {
public:
    explicit Tab(std::string name) : name_(std::move(name)) {}

    template <typename T, typename... Args>
    T& Add(Args&&... args) {
        auto widget = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *widget;
        widgets_.push_back(std::move(widget));
        return ref;
    }

    void Update(const InputState& in, Rect contentArea, const Theme& theme, float dt) {
        float y = contentArea.y;
        for (auto& w : widgets_) {
            float h = w->Height(theme);
            Rect r{contentArea.x, y, contentArea.w, h};
            w->Update(in, r, dt);
            y += h + 6.0f;
        }
    }

    void Render(IRenderer& r, const Theme& theme, Rect contentArea) const {
        float y = contentArea.y;
        for (auto& w : widgets_) {
            float h = w->Height(theme);
            Rect rect{contentArea.x, y, contentArea.w, h};
            w->Render(r, theme, rect);
            y += h + 6.0f;
        }
    }

    void SaveAll(ConfigStore& cfg) const { for (auto& w : widgets_) w->SaveTo(cfg); }
    void LoadAll(const ConfigStore& cfg) { for (auto& w : widgets_) w->LoadFrom(cfg); }

    const std::string& Name() const { return name_; }

private:
    std::string name_;
    std::vector<std::unique_ptr<Widget>> widgets_;
};

} // namespace cui
