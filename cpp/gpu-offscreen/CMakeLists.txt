# CMakeList.txt : CMake project for offline, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.16)

project ("gpu-offscreen")

set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
set(TEST_DIR "${CMAKE_CURRENT_LIST_DIR}/test")

find_package(glfw3 CONFIG REQUIRED)
link_libraries(glfw)

find_package(Vulkan REQUIRED)
link_libraries(Vulkan::Vulkan)


add_subdirectory(${SOURCE_DIR})

if(BUILD_TESTING)
	add_subdirectory(${TEST_DIR})
endif()
