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

# ============================================================
# 引擎源文件
# ============================================================
# 显式列表。新增 .cpp 时手动加到这里。
#
# 为什么不使用 file(GLOB_RECURSE)：
#   - 误建的文件会被自动编译，导致链接期才发现重定义
#   - 无法控制哪些文件属于哪个 target
#   - 加文件时缺少"这一步是有意识决定"的仪式感
#
# 分层顺序：入口 → Audio → Core → Editor → Render → Resource → Scene → World

set(NYX_ENGINE_SOURCES
    # 引擎入口
    NyxEngine/NyxEngine.cpp

    # Audio
    NyxEngine/Audio/AudioClock.cpp

    # Input
    NyxEngine/Input/Win32InputBackend.cpp

    # Core
    NyxEngine/Core/EventBus.cpp
    NyxEngine/Core/FileSystem.cpp
    NyxEngine/Core/ImGuiManager.cpp
    NyxEngine/Core/InputSystem.cpp
    NyxEngine/Core/JobSystem.cpp
    NyxEngine/Core/JsonValue.cpp
    NyxEngine/Core/Logger.cpp
    NyxEngine/Core/Memory.cpp
    NyxEngine/Core/Platform.cpp
    NyxEngine/Core/VulkanContext.cpp

    # Editor
    NyxEngine/Editor/CameraPanel.cpp
    NyxEngine/Editor/DisplaySettingsPanel.cpp
    NyxEngine/Editor/EditorPanel.cpp
    NyxEngine/Editor/InputPanel.cpp          # ← 新增
    NyxEngine/Editor/LightingPanel.cpp
    NyxEngine/Editor/MaterialPanel.cpp
    NyxEngine/Editor/PerformancePanel.cpp
    NyxEngine/Editor/TargetMovementPanel.cpp

    # Render
    NyxEngine/Render/LightingSystem.cpp
    NyxEngine/Render/Renderer.cpp

    # Resource
    NyxEngine/Resource/MeshLoader.cpp
    NyxEngine/Resource/ResourceManager.cpp

    # Scene
    NyxEngine/Scene/Camera.cpp
    NyxEngine/Scene/Material.cpp

    # World
    NyxEngine/World/Player.cpp

    # Physics
    NyxEngine/Physics/Shapes/SphereShape.cpp
    NyxEngine/Physics/Shapes/CapsuleShape.cpp
    NyxEngine/Physics/Shapes/BoxShape.cpp
    NyxEngine/Physics/Broadphase/BruteForceBroadphase.cpp   # ← 新增
    NyxEngine/Physics/Narrowphase/Narrowphase.cpp   # ← 新增
    NyxEngine/Physics/PhysicsWorld.cpp                # ← 新增
)

# ============================================================
# 游戏源文件
# ============================================================

set(NYX_GAME_SOURCES
    Game/MyGame.cpp
    Game/Target/TargetManager.cpp
)

# ============================================================
# ImGui 源文件
# ============================================================

set(IMGUI_DIR "D:/Dev/NyxEngine/ThirdParty/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# ============================================================
# 可执行文件
# ============================================================

add_executable(NyxEngine
    main.cpp
    ${NYX_ENGINE_SOURCES}
    ${NYX_GAME_SOURCES}
    ${IMGUI_SOURCES}
)

# 链接库
target_link_libraries(NyxEngine PRIVATE Vulkan::Vulkan SDL2::SDL2main SDL2::SDL2)

# 包含目录
target_include_directories(NyxEngine PRIVATE
    .
    NyxEngine
    NyxEngine/Core
    NyxEngine/Render
    NyxEngine/Scene
    NyxEngine/Audio
    NyxEngine/World
    NyxEngine/Input
    Game
    ThirdParty/glm/glm
    ThirdParty/imgui
    ThirdParty/imgui/backends
    ThirdParty/SDL2/SDL2-2.32.10/include
)