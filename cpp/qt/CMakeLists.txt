cmake_minimum_required(VERSION 3.16)

project(untitled LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(untitled
	main.cpp
	mainwindow.cpp
	mainwindow.h
	mainwindow.ui
)

target_link_libraries(untitled PRIVATE Qt5::Widgets)
