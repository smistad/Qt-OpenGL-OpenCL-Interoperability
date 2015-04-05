cmake_minimum_required(VERSION 2.8)
project(Qt-Test)

add_subdirectory(OpenCLUtilityLibrary)
find_package(OpenCLUtilityLibrary PATHS "${Qt-Test_BINARY_DIR}/OpenCLUtilityLibrary" REQUIRED)
include(${OpenCLUtilityLibrary_USE_FILE})

set(SOURCES main.cpp)
set(QT_HEADERS main.hpp)
set(QT_USE_QTOPENGL TRUE)
find_package(Qt5Widgets)
find_package(Qt5OpenGL)
qt5_wrap_cpp(HEADERS_MOC ${QT_HEADERS})
set(LIBRARIES ${LIBRARIES} ${QT_LIBRARIES})
include_directories(${Qt5Widgets_INCLUDE_DIRS} ${Qt5OpenGL_INCLUDE_DIRS})

find_package(Boost COMPONENTS system thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS} ${PROJECT_BINARY_DIR})
link_directories(${Boost_LIBRARY_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_executable(test ${SOURCES} ${HEADERS_MOC})
target_link_libraries(test OpenCLUtilityLibrary ${QT_LIBRARIES} ${Boost_LIBRARIES})
qt5_use_modules(test Widgets OpenGL) # Add Qt 5 modules
