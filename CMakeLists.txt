cmake_minimum_required(VERSION 2.8)
project( BroxOF )
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
FIND_PACKAGE( OpenCV REQUIRED )

add_executable( BroxOF main.cpp GaussianPyramid.cpp OpticalFlow.cpp Stochastic.cpp)
target_link_libraries( BroxOF ${OpenCV_LIBS} )
