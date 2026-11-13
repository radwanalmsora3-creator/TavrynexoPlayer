cmake_minimum_required(VERSION 3.22)

project(TavrynexoPlayer
    VERSION 1.0.0
    DESCRIPTION "Tavrynexo Modern Cinematic Desktop Media Player"
    LANGUAGES C CXX
)

# فرض معيار C++20 الصارم
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# تفعيل أدوات Qt الأوتوماتيكية
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# البحث عن حزم Qt6
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Concurrent
)

# البحث عن مكتبات FFmpeg
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if(NOT WIN32)
    find_package(PkgConfig QUIET)
endif()
if(PKG_CONFIG_FOUND)
    pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
    pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
    pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)
    pkg_check_modules(SWSCALE REQUIRED IMPORTED_TARGET libswscale)
    pkg_check_modules(SWRESAMPLE REQUIRED IMPORTED_TARGET libswresample)
else()
    find_package(FFMPEG REQUIRED)
endif()

# البحث عن Vulkan
find_package(Vulkan REQUIRED)

# مجلدات التضمين
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan
    ${Vulkan_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
)

# ملفات المصدر (Sources & Headers)
set(HEADERS
    include/core/Demuxer.hpp
    include/core/VideoDecoder.hpp
    include/core/AudioDecoder.hpp
    include/core/AudioEngine.hpp
    include/core/ClockSync.hpp
    include/core/PlayerController.hpp
    include/vulkan/VulkanRenderer.hpp
)

set(SOURCES
    src/main.cpp
    src/core/Demuxer.cpp
    src/core/VideoDecoder.cpp
    src/core/AudioDecoder.cpp
    src/core/AudioEngine.cpp
    src/core/ClockSync.cpp
    src/core/PlayerController.cpp
    src/vulkan/VulkanRenderer.cpp
)

# ملف الموارد للـ QML
set(QML_RESOURCES
    qml/qml.qrc
)

# إنشاء التطبيق التنفيذي
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${QML_RESOURCES}
)

# ربط المكتبات
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Concurrent
)

if(PKG_CONFIG_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        PkgConfig::AVCODEC
        PkgConfig::AVFORMAT
        PkgConfig::AVUTIL
        PkgConfig::SWSCALE
        PkgConfig::SWRESAMPLE
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARIES})
endif()

# دعم مؤشرات الـ Threads والصوت لنظامي Linux و Windows
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ole32 winmm)
    # تخصيص التطبيق ليعمل كـ GUI App بدون شاشة كونسول سوداء في الـ Release
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl m)
endif()
