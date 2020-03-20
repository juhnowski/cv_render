cmake_minimum_required(VERSION 3.15)
project(video)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SUPPRESS_REGENERATION true)

find_package( OpenCV REQUIRED )

set(INC_DIRS ${PROJECT_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS})

find_library (
        AVCODEC_LIB
        NAMES libavcodec.so.56
        PATHS /usr/local/lib
)

find_library (
        AVFORMAT_LIB
        NAMES libavformat.so.56
        PATHS /usr/local/lib
)

find_library (
        AVUTIL_LIB
        NAMES libavutil.so.56
        PATHS /usr/local/lib
)

find_library (
        AVFILTER_LIB
        NAMES libavfilter.so.56
        PATHS /usr/local/lib
)

find_library (
        SWSCALE_LIB
        NAMES libswscale.so.5
        PATHS /usr/local/lib
)

find_library (
        AVDEVICE_LIB
        NAMES libavdevice.so.56
        PATHS /usr/local/lib
)

#set(LIBS ${OpenCV_LIBS} -lavcodec -lavformat -lavutil -lswscale -lavfilter -lavdevice -lz -lm -lpthread)
set(LIBS ${OpenCV_LIBS} ${AVDEVICE_LIB} ${SWSCALE_LIB} ${AVFILTER_LIB} ${AVUTIL_LIB} ${AVFORMAT_LIB} ${AVCODEC_LIB} -lz -lm -lpthread)
set(PROJECT_INCLUDE_DIRS ${INC_DIRS})

add_executable(video main.cpp)

include_directories(${OpenCV_INCLUDE_DIRS})

target_link_libraries( ${PROJECT_NAME} ${OpenCV_LIBS} ${AVDEVICE_LIB} ${SWSCALE_LIB} ${AVFILTER_LIB} ${AVUTIL_LIB} ${AVFORMAT_LIB} ${AVCODEC_LIB} )

configure_file(test.sdp test.sdp COPYONLY)