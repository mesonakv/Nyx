cmake_minimum_required(VERSION 3.20)
project(NyxEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 禁用异常和 RTTI，强制 UTF-8 源码编码
add_compile_options(/EHsc /GR- /utf-8)

# Vulkan
find_package(Vulkan REQUIRED)

# SDL2
set(SDL2_DIR "D:/Dev/NyxEngine/ThirdParty/SDL2/SDL2-2.32.10/cmake")
find_package(SDL2 REQUIRED)

# 源文件
file(GLOB_RECURSE NYX_ENGINE_SOURCES
    "NyxEngine/*.cpp"
    "Game/*.cpp"
    "main.cpp"
)

# ImGui 源文件
set(IMGUI_DIR "D:/Dev/NyxEngine/ThirdParty/imgui")
list(APPEND NYX_ENGINE_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

add_executable(NyxEngine ${NYX_ENGINE_SOURCES})

# 链接库
target_link_libraries(NyxEngine PRIVATE Vulkan::Vulkan SDL2::SDL2main SDL2::SDL2)

# 包含目录
target_include_directories(NyxEngine PRIVATE
    .
    NyxEngine
    NyxEngine/Core
    NyxEngine/Render
    NyxEngine/Scene
    Game
    ThirdParty/glm/glm
    ThirdParty/imgui
    ThirdParty/imgui/backends
    ThirdParty/SDL2/SDL2-2.32.10/include
)