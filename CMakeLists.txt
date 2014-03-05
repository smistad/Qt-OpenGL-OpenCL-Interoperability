cmake_minimum_required(VERSION 2.8)
project(FLTK-Test)

add_subdirectory(OpenCLUtilityLibrary)
find_package(OpenCLUtilityLibrary PATHS "${FLTK-Test_BINARY_DIR}/OpenCLUtilityLibrary" REQUIRED)
include(${OpenCLUtilityLibrary_USE_FILE})

find_package(FLTK REQUIRED)
include_directories(${FLTK_INCLUDE_DIRS})

find_package(Boost COMPONENTS thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS} ${PROJECT_BINARY_DIR})
link_directories(${Boost_LIBRARY_DIRS})

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} `fltk-config --enable-threads`")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

add_executable(test main.cpp)
target_link_libraries(test OpenCLUtilityLibrary ${FLTK_LIBRARIES} ${Boost_LIBRARIES})