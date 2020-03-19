cmake_minimum_required(VERSION 3.15)
project(video)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SUPPRESS_REGENERATION true)

find_package( OpenCV REQUIRED )

set(INC_DIRS ${PROJECT_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS})
set(LIBS ${OpenCV_LIBS} -lavcodec -lavformat -lavutil -lswscale -lavfilter -lavutil -lavdevice -lz -lm -lpthread)

set(PROJECT_INCLUDE_DIRS ${INC_DIRS})

add_executable(video main.cpp)

include_directories(${OpenCV_INCLUDE_DIRS})

target_link_libraries( ${PROJECT_NAME} ${OpenCV_LIBS} )

configure_file(test.sdp test.sdp COPYONLY)