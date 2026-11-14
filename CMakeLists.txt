cmake_minimum_required(VERSION 3.21)
project(ReworkTime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Sql Svg Widgets)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(reworktime
    assets/app.rc
    src/main.cpp
    src/tracker.h   src/tracker.cpp
    src/store.h     src/store.cpp
    src/backend.h   src/backend.cpp
    src/tray.h      src/tray.cpp
)

qt_add_qml_module(reworktime
    URI ReworkTime
    VERSION 1.0
    QML_FILES qml/Main.qml
)

target_link_libraries(reworktime PRIVATE Qt6::Quick Qt6::Sql Qt6::Svg Qt6::Widgets user32 dwmapi psapi)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(reworktime PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
