cmake_minimum_required(VERSION 2.6)
PROJECT(exposer-gui)

FIND_PACKAGE(Qt5 COMPONENTS Gui Widgets Charts)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_AUTOUIC ON)

set(EXPOSERGUI_PART_UI
    mainwindow.ui
)

set(EXPOSERGUI_PART_SRCS
    main.cpp
    mainwindow.cpp
)
qt5_wrap_ui(EXPOSERGUI_PART_SRCS ${EXPOSERGUI_PART_UI})
ADD_EXECUTABLE(exposer-gui ${EXPOSERGUI_PART_SRCS})
TARGET_LINK_LIBRARIES(exposer-gui
	Qt5::Gui
	Qt5::Widgets
	Qt5::Charts
)
