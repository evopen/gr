# CMakeList.txt : CMake project for offline, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.16)

project ("offline")

set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
set(TEST_DIR "${CMAKE_CURRENT_LIST_DIR}/test")

add_subdirectory(${SOURCE_DIR})

include(CTest)
if(BUILD_TESTING)
	add_subdirectory(${TEST_DIR})
endif()
