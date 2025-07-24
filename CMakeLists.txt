cmake_minimum_required(VERSION 3.5)

project(ConcurrentCalc VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

message(STATUS "Using Qt version ${QT_VERSION_MAJOR}")

set(PROJECT_HEADERS
    calcmainwindow.h
)

set(PROJECT_SOURCES
        main.cpp
        calcmainwindow.cpp
)

set(PROJECT_UIS
    ui/calcmainwindow.ui
)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ConcurrentCalc
        MANUAL_FINALIZATION
        ${PROJECT_HEADERS}
        ${PROJECT_SOURCES}
        ${PROJECT_UIS}
    )
    qt_finalize_executable(ConcurrentCalc)
else()
    add_executable(ConcurrentCalc
        ${PROJECT_HEADERS}
        ${PROJECT_SOURCES}
        ${PROJECT_UIS}
    )
    # qt5_wrap_ui(PROJECT_UIS ${PROJECT_UIS})
    target_sources(ConcurrentCalc PRIVATE ${PROJECT_UIS})
endif()

target_link_libraries(ConcurrentCalc PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(ConcurrentCalc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ConcurrentCalc
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
