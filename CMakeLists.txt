cmake_minimum_required(VERSION 3.1.0)     
project(FishLabeler CXX)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -std=c++14 -Wall")
set(CMAKE_INCLUDE_CURRENT_DIR on)

#boost
find_package(Boost COMPONENTS filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS}) 

#OpenCV
FIND_PACKAGE(OpenCV COMPONENTS core highgui imgproc imgcodecs REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
message("OpenCV include: " ${OpenCV_INCLUDE_DIRS})
message("OpenCV link: " ${OpenCV_LIBS})

#Qt5
set(CMAKE_AUTOMOC ON)
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

#make the video reading library
set(SRCS VideoReader.cpp)
set(HDRS VideoReader.hpp VideoFrame.hpp) 
add_library(FishLabelerCore ${SRCS} ${HDRS})
target_link_libraries(FishLabelerCore ${Boost_LIBRARIES}) # ${OpenCV_LIBS} ${LIBS})

#make the UI application
set(FLSRCS main.cpp VideoWindow.cpp FrameViewer.cpp)
set(FLHDRS VideoWindow.hpp FrameViewer.hpp) 
add_executable(FishLabeler ${FLSRCS} ${FLHDRS})
target_link_libraries(FishLabeler FishLabelerCore ${Boost_LIBRARIES} ${OpenCV_LIBS} Qt5::Widgets) 

