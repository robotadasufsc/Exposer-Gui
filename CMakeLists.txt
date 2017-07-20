cmake_minimum_required(VERSION 2.6)
PROJECT(exposer-gui C CXX)

FIND_PACKAGE(Qt5 COMPONENTS Gui Widgets Charts SerialPort)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "GCC detected, adding compile flags")
    add_definitions ("-Wall -Wunused")
endif(CMAKE_COMPILER_IS_GNUCXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(EXPOSERGUI_PART_UI
    src/mainwindow.ui
)

set(EXPOSERGUI_PART_SRCS
    src/main.cpp
    src/mainwindow.cpp
    src/seriallayer.cpp
    src/exposervariables.cpp
    submodules/QDarkStyleSheet/qdarkstyle/style.qss
    submodules/QDarkStyleSheet/qdarkstyle/style.qrc
)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

file(COPY submodules DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

qt5_wrap_ui(EXPOSERGUI_PART_SRCS ${EXPOSERGUI_PART_UI})
ADD_EXECUTABLE(exposer-gui ${EXPOSERGUI_PART_SRCS})
TARGET_LINK_LIBRARIES(exposer-gui
	Qt5::Gui
	Qt5::Widgets
	Qt5::Charts
	Qt5::SerialPort
)
