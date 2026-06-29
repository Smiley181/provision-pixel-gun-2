cmake_minimum_required(VERSION 3.16)
project(PixelGun2Cheat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui")

add_library(cheat SHARED
    src/main.cpp
    src/il2cpp.cpp
    src/renderer.cpp
    src/unity.cpp
    src/features.cpp
    src/config.cpp
    src/menu.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_dx11.cpp
    ${IMGUI_DIR}/imgui_impl_win32.cpp
)

target_include_directories(cheat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${IMGUI_DIR}
)

target_link_libraries(cheat PRIVATE
    d3d11
    dxgi
    winmm
)

set_target_properties(cheat PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    target_compile_options(cheat PRIVATE /W4 /wd4100 /wd4189 /wd4996 /utf-8 /EHa)
    set_target_properties(cheat PROPERTIES
        LINK_FLAGS "/NXCOMPAT /DYNAMICBASE"
    )
endif()
