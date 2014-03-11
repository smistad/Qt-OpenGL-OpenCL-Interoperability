cmake_minimum_required(VERSION 2.8)
project(Qt-Test)

add_subdirectory(OpenCLUtilityLibrary)
find_package(OpenCLUtilityLibrary PATHS "${Qt-Test_BINARY_DIR}/OpenCLUtilityLibrary" REQUIRED)
include(${OpenCLUtilityLibrary_USE_FILE})

set(QT_USE_QTOPENGL TRUE)
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
set(SOURCES main.cpp)
set(HEADERS main.hpp)
QT4_WRAP_CPP(HEADERS_MOC ${HEADERS})

find_package(Boost COMPONENTS thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS} ${PROJECT_BINARY_DIR})
link_directories(${Boost_LIBRARY_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

add_executable(test ${SOURCES} ${HEADERS_MOC})
target_link_libraries(test OpenCLUtilityLibrary ${QT_LIBRARIES} ${Boost_LIBRARIES})
