cmake_minimum_required(VERSION 3.8)
project(Cpp_Project_Updater LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Cpp_Project_Updater src/main.cpp)
target_include_directories(Cpp_Project_Updater PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    set(CMAKE_GENERATOR "Visual Studio 17 2022" CACHE STRING "Generator" FORCE)
endif()
