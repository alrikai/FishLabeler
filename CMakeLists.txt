cmake_minimum_required(VERSION 3.1.0)     
project(FishLabeler CXX)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -std=c++14 -Wall")
set(CMAKE_INCLUDE_CURRENT_DIR on)

#boost
find_package(Boost COMPONENTS filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS}) 

#OpenCV
FIND_PACKAGE(OpenCV COMPONENTS core highgui imgproc REQUIRED)  
if(OpenCV_VERSION VERSION_LESS "3.0")
	MESSAGE("Using OpenCV vers. ${OpenCV_VERSION}") 
else()
    #3.0 moved imwrite into imgcodecs, which doesn't exist in 2.X 
    FIND_PACKAGE(OpenCV COMPONENTS core highgui imgproc imgcodecs REQUIRED) 
	MESSAGE("Using OpenCV vers. ${OpenCV_VERSION}") 
endif()
include_directories(${OpenCV_INCLUDE_DIRS})
message("OpenCV include: " ${OpenCV_INCLUDE_DIRS})
message("OpenCV link: " ${OpenCV_LIBS})

#Qt5
set(CMAKE_AUTOMOC ON)
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

#make the UI application
set(FLSRCS main.cpp VideoReader.cpp VideoWindow.cpp FrameViewer.cpp VideoLogger.cpp)
set(FLHDRS VideoReader.hpp VideoWindow.hpp FrameViewer.hpp VideoLogger.hpp) 
add_executable(FishLabeler ${FLSRCS} ${FLHDRS})
target_link_libraries(FishLabeler ${Boost_LIBRARIES} ${OpenCV_LIBS} Qt5::Widgets) 

