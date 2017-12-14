cmake_minimum_required(VERSION 2.8.9)     
project(FishLabeler CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -std=c++14 -Wall")
set(CMAKE_INCLUDE_CURRENT_DIR on)

# link to ffmpeg
set(FFMPEG_LIBS avdevice avformat avfilter avcodec swscale avutil)
link_directories("/usr/lib64")  

set(SRCS VideoReader.cpp)
set(HDRS VideoReader.hpp VideoFrame.hpp) 

# Create the target 
add_library(FishLabelerCore ${SRCS} ${HDRS})
target_link_libraries(FishLabelerCore ${FFMPEG_LIBS}) # ${OpenCV_LIBS} ${LIBS})

set(FLSRC main.cpp)
add_executable(FishLabeler ${FLSRC})
target_link_libraries(FishLabeler FishLabelerCore ${FFMPEG_LIBS}) # ${OpenCV_LIBS} ${LIBS})

