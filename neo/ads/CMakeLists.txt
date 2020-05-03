cmake_minimum_required(VERSION 3.3)
set(ads_VERSION "2.3.2")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
project(QtAdvancedDockingSystem VERSION ${ads_VERSION})
option(BUILD_STATIC "Build the static library" OFF)
option(BUILD_EXAMPLES "Build the examples" ON)
set(REQUIRED_QT_VERSION 5.5.0)
find_package(Qt5Core ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt5Gui ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt5Widgets ${REQUIRED_QT_VERSION} REQUIRED)
message(STATUS "Found Qt ${Qt5Core_VERSION}")
set(ads_INCLUDE ${ads_INCLUDE} ${Qt5Core_INCLUDE_DIRS})
set(ads_INCLUDE ${ads_INCLUDE} "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(ads_LIBS ${ads_LIBS} ${Qt5Core_LIBRARIES})
set(ads_COMPILE_DEFINE ${ads_COMPILE_DEFINE} ${Qt5Core_COMPILE_DEFINITIONS})
set(ads_LIBS ${ads_LIBS} ${Qt5Gui_LIBRARIES})
set(ads_INCLUDE ${ads_INCLUDE} ${Qt5Gui_INCLUDE_DIRS})
set(ads_COMPILE_DEFINE ${ads_COMPILE_DEFINE} ${Qt5Gui_COMPILE_DEFINITIONS})
set(ads_LIBS ${ads_LIBS} ${Qt5Widgets_LIBRARIES})
set(ads_INCLUDE ${ads_INCLUDE} ${Qt5Widgets_INCLUDE_DIRS})
set(ads_COMPILE_DEFINE ${ads_COMPILE_DEFINE} ${Qt5Widgets_COMPILE_DEFINITIONS})
if(BUILD_STATIC)
    set(CMAKE_STATIC_LIBRARY_SUFFIX "_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
endif()
set(ads_SRCS
    src/ads_globals.cpp
    src/DockAreaTabBar.cpp
    src/DockAreaTitleBar.cpp
    src/DockAreaWidget.cpp
    src/DockContainerWidget.cpp
    src/DockManager.cpp
    src/DockOverlay.cpp
    src/DockSplitter.cpp
    src/DockWidget.cpp
    src/DockWidgetTab.cpp
	src/DockingStateReader.cpp
    src/ElidingLabel.cpp
    src/FloatingDockContainer.cpp
	src/FloatingDragPreview.cpp
	src/IconProvider.cpp
	src/DockComponentsFactory.cpp
    src/ads.qrc
    src/linux/FloatingWidgetTitleBar.cpp
)
set(ads_INSTALL_INCLUDE 
    src/ads_globals.h
    src/DockAreaTabBar.h
    src/DockAreaTitleBar.h
    src/DockAreaWidget.h
    src/DockContainerWidget.h
    src/DockManager.h
    src/DockOverlay.h
    src/DockSplitter.h
    src/DockWidget.h
    src/DockWidgetTab.h
	src/DockingStateReader.h
    src/ElidingLabel.h
    src/FloatingDockContainer.h
	src/FloatingDragPreview.h
	src/IconProvider.h
	src/DockComponentsFactory.h
    src/linux/FloatingWidgetTitleBar.h
)


add_library(qtadvanceddocking STATIC ${ads_SRCS})
target_compile_definitions(qtadvanceddocking PUBLIC ADS_STATIC)

target_include_directories(qtadvanceddocking PUBLIC 
    "$<BUILD_INTERFACE:${ads_INCLUDE}>"
    )
target_link_libraries(qtadvanceddocking PUBLIC ${ads_LIBS})
target_compile_definitions(qtadvanceddocking PRIVATE ${ads_COMPILE_DEFINE})
