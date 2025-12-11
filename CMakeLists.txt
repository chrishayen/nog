cmake_minimum_required(VERSION 3.16)
project(nog VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(fmt REQUIRED)
find_package(tomlplusplus REQUIRED)

add_library(nog_lib
    src/lexer.cpp
    src/parser.cpp
    src/codegen.cpp
    src/typechecker.cpp
    src/project.cpp
    src/module.cpp
)
target_link_libraries(nog_lib fmt::fmt tomlplusplus::tomlplusplus)

add_executable(nog src/main.cpp)
target_link_libraries(nog nog_lib)
