cmake_minimum_required(VERSION 3.21)
project(TavrynexoPlayer VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

if(NOT DEFINED MPV_ROOT)
    set(MPV_ROOT "C:/libmpv-sdk")
endif()

find_library(MPV_LIBRARY
    NAMES libmpv-2 mpv
    PATHS "${MPV_ROOT}/lib" "${MPV_ROOT}"
    REQUIRED
)

add_executable(${PROJECT_NAME} WIN32
    src/main.cpp
    src/player/MpvVideoWidget.cpp
    src/ui/IconFactory.cpp
    src/ui/PlayerControls.cpp
    src/ui/PlaylistWidget.cpp
    src/ui/Dialogs.cpp
    src/ui/MainWindow.cpp
    resources/app.rc
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${MPV_ROOT}/include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    ${MPV_LIBRARY}
)
