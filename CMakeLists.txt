cmake_minimum_required(VERSION 3.16)
project(cheatui CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Turn this off if you're pulling cheatui in with add_subdirectory() and
# don't want its example executables built alongside your own project.
option(CHEATUI_BUILD_EXAMPLES "Build the example programs" ON)

add_library(cheatui STATIC
    src/Canvas.cpp
    src/Widgets.cpp
    src/Menu.cpp
)
target_include_directories(cheatui PUBLIC include)
target_compile_options(cheatui PRIVATE -Wall -Wextra)

if(CHEATUI_BUILD_EXAMPLES)
    add_executable(cheatui_demo examples/demo.cpp)
    target_link_libraries(cheatui_demo PRIVATE cheatui)

    add_executable(cheatui_getting_started examples/getting_started.cpp)
    target_link_libraries(cheatui_getting_started PRIVATE cheatui)
endif()
