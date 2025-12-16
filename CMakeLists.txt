cmake_minimum_required(VERSION 3.21)
project(CourseWorkApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_executable(CourseWorkApp
    src/main.cpp
    src/ui/LoginWindow.h
    src/ui/LoginWindow.cpp
)

target_link_libraries(CourseWorkApp PRIVATE Qt6::Widgets)
target_include_directories(CourseWorkApp PRIVATE src)