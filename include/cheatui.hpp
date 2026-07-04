#pragma once
// Single include that pulls in the whole public API. Drop the include/
// folder into your project and #include "cheatui.hpp" - that's the whole
// setup. (If you'd rather only pull in what you use, every header below
// is self-contained and can be included on its own instead.)

#include "Color.hpp"
#include "Easing.hpp"
#include "Animation.hpp"
#include "Renderer.hpp"
#include "Theme.hpp"
#include "Themes.hpp"
#include "Style.hpp"
#include "Input.hpp"
#include "Config.hpp"
#include "Widgets.hpp"
#include "Tab.hpp"
#include "Menu.hpp"

// Canvas.hpp (the reference software-renderer backend) is intentionally
// left out of the umbrella header - it's only needed if you want to render
// without a real graphics API (the demos use it). Include it separately:
//   #include "Canvas.hpp"
