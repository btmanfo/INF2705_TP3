cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

project(INF2705_TP3 LANGUAGES CXX)

find_package(glm CONFIG REQUIRED)
find_package(SFML 3 COMPONENTS System Window Graphics CONFIG REQUIRED)
find_package(glbinding CONFIG REQUIRED)

add_executable(TP3
    src/main.cpp src/model.cpp src/windmill.cpp
    imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp imgui/imgui_impl_opengl3.cpp
)
target_compile_features(TP3 PRIVATE cxx_std_20)
target_include_directories(TP3 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(TP3 PRIVATE glm::glm SFML::System SFML::Window SFML::Graphics
    glbinding::glbinding glbinding::glbinding-aux)
if(MSVC)
    target_compile_definitions(TP3 PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()
set_target_properties(TP3 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src")

