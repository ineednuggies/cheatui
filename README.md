# cheatui

A small, dependency-free C++17 UI library for building themed overlay-style
menus: draggable window, tabbed sidebar with an animated selection bar,
smooth hover/toggle/fade animations everywhere, and a save/load config
system. No third-party libraries required.

![menu preview](preview/frame_00_initial.png)

It's renderer-agnostic. All drawing goes through a small `IRenderer`
interface, so you plug it into whatever you're already drawing with (D3D,
OpenGL, GDI, an existing ImGui draw list, whatever) and every widget,
animation, and the config system works as-is. A CPU software renderer
(`Canvas`) ships with the library so the examples build and run with zero
setup, including headless.

New here? Read **[TUTORIAL.md](TUTORIAL.md)** for a full walkthrough. This
README covers the shape of the project and how to get it building.

## Widgets

- **Checkbox** - on/off switch with a sliding pill
- **Slider** - drag to pick a value from a range, with an optional step and unit suffix
- **Stepper** - small integer count with -/+ buttons
- **RadioGroup** - a few mutually exclusive options shown side by side
- **ComboBox** - dropdown list for longer option sets
- **ColorPicker** - saturation/value square + hue strip
- **Keybind** - click, then press a key to bind it
- **Button** - plain action button
- **Label** - section header

Every widget shares the same shape (an id, `Update`, `Render`,
`SaveTo`/`LoadFrom`), so adding a new one is a matter of writing one more
small class - see the bottom of TUTORIAL.md.

## Project layout

```
include/           every public header (this is what you copy into your project)
  cheatui.hpp         single include that pulls in the whole public API
  Color.hpp           RGBA color, HSV conversion, lerp
  Easing.hpp          easing curves used by the animation system
  Animation.hpp       AnimatedFloat / AnimatedColor / HueCycler - the fade/slide primitives
  Renderer.hpp         IRenderer - implement this once per graphics backend
  Theme.hpp             color palette + spacing, all in one struct
  Input.hpp              per-frame mouse/keyboard snapshot
  Config.hpp               typed key/value store with .cfg file I/O
  Widgets.hpp                 every widget class
  Tab.hpp                        a page of widgets
  Menu.hpp                          the window itself
  Canvas.hpp                        reference CPU-rendered IRenderer implementation
  Font5x7.hpp                       bitmap font data used by Canvas
src/                implementations for the headers above that need one
examples/
  getting_started.cpp    the short walkthrough from TUTORIAL.md, runnable
  demo.cpp                bigger 5-tab showcase exercising every widget
tools/
  raw_to_png.py            converts Canvas's .raw frame dumps into real PNGs
```

## Building

Needs nothing but a C++17 compiler. Two options:

**Just compile it directly:**
```sh
g++ -std=c++17 -O2 -Iinclude src/*.cpp examples/getting_started.cpp -o getting_started
./getting_started
```

**Or with CMake:**
```sh
cmake -S . -B build
cmake --build build
./build/cheatui_getting_started
./build/cheatui_demo
```

Both examples write `.raw` frame dumps to `out/` (there's no window system
in a console program, so they render into an in-memory canvas and save
it). View one with:
```sh
python3 tools/raw_to_png.py out/getting_started.raw out/getting_started.png
```

## Using it in your own project

Copy `include/` and `src/` into your project (or `add_subdirectory()` it
if you're using CMake - see TUTORIAL.md section 1), then:

```cpp
#include "cheatui.hpp"

cui::Menu menu("My App", cui::Rect{60, 60, 520, 360});
cui::Tab& main = menu.AddTab("Main");
main.Add<cui::Checkbox>("main.enabled", "Enable feature", true);

// your loop:
menu.Update(inputSnapshot, deltaTime);
menu.Render(yourRenderer);
```

Full details, every widget type, config profiles, and theming are in
**[TUTORIAL.md](TUTORIAL.md)**.

## Putting this on GitHub

If you want your own copy of this hosted on GitHub so others can clone or
submodule it:

```sh
cd cheatui
git init
git add .
git commit -m "initial commit"
git branch -M main
git remote add origin https://github.com/yourname/cheatui.git
git push -u origin main
```

A GitHub Actions workflow is already included at
`.github/workflows/build.yml` - it builds the library and both examples
on every push and pull request, so anyone opening a PR gets an automatic
build check. Nothing else to configure; GitHub picks it up as soon as the
`.github/workflows/` folder is pushed.

If your repo is going to be used by other projects via CMake, tag
releases (`git tag v1.0.0 && git push --tags`) so consumers can pin to a
specific version instead of tracking `main`.

## Notes

- Text rendering uses a small bitmap font (`Font5x7.hpp`) built for this
  project so the examples have zero font-file dependencies. Swap
  `Canvas::Text` for your platform's real text API in a production
  backend for proper kerning, anti-aliasing, and Unicode support.
- The library is UI only - layout, input handling, animation, theming,
  config persistence. There's no automation, memory access, or networking
  anywhere in it. What a "Checkbox" or "Auto X" toggle *does* when it's on
  is entirely up to the application reading `.Value()`.
- MIT licensed - see `LICENSE`.
