cmake_minimum_required(VERSION 3.25)
project(M1Arc VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(FetchContent)

# Same RED4ext SDK pin as the saved native M1Core prototype.
set(RED4EXT_HEADER_ONLY ON CACHE BOOL "" FORCE)
set(RED4EXT_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    red4ext_sdk
    GIT_REPOSITORY https://github.com/wopss/RED4ext.SDK.git
    GIT_TAG a34f0ad0a9e4f37cbe1fe001e4ff68c2949d0665
)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
# Match the verified m1core/core renderer's 1.91.1 docking API.
# Its Win32 and DX12 backend implementations are preserved under src/backends.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.1-docking
)
FetchContent_MakeAvailable(red4ext_sdk nlohmann_json imgui)

add_library(m1arc_imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    src/backends/win32.cpp
    src/backends/dx12.cpp
)
target_include_directories(m1arc_imgui PUBLIC
    "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(m1arc_imgui PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)
target_link_libraries(m1arc_imgui PUBLIC
    d3d12 dxgi d3dcompiler dwmapi imm32 user32)

add_library(m1arc_state STATIC src/state.cpp)
target_include_directories(m1arc_state PUBLIC src)
target_link_libraries(m1arc_state PUBLIC nlohmann_json::nlohmann_json)
target_compile_definitions(m1arc_state PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)

add_library(m1arc_registry STATIC src/registry.cpp)
target_include_directories(m1arc_registry PUBLIC src)
target_link_libraries(m1arc_registry PUBLIC m1arc_state)

if(WIN32)
    add_library(m1arc SHARED
        src/plugin.cpp
        src/graphics.cpp
        src/input.cpp
        src/ui.cpp
        src/fonts.cpp
    )
    target_include_directories(m1arc PRIVATE src)
    target_link_libraries(m1arc PRIVATE
        m1arc_registry RED4ext::SDK m1arc_imgui
        d3d12 dxgi user32)
    target_compile_definitions(m1arc PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE RED4EXT_HEADER_ONLY)
    set_target_properties(m1arc PROPERTIES
        OUTPUT_NAME "m1arc" PREFIX "")
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(m1arc_registry_tests tests/registry_tests.cpp)
    target_link_libraries(m1arc_registry_tests PRIVATE m1arc_registry)
    add_test(NAME registry COMMAND m1arc_registry_tests)

    add_executable(m1arc_dev_fixture_tests tests/dev_fixture_tests.cpp)
    target_link_libraries(m1arc_dev_fixture_tests PRIVATE m1arc_registry)
    add_test(NAME dev_fixture COMMAND m1arc_dev_fixture_tests)

    add_executable(m1arc_state_tests tests/state_tests.cpp)
    target_link_libraries(m1arc_state_tests PRIVATE m1arc_state)
    add_test(NAME persistence COMMAND m1arc_state_tests)
endif()
