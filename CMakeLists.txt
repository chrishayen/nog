cmake_minimum_required(VERSION 3.16)
project(nog VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR}/src)

find_package(fmt REQUIRED)
find_package(tomlplusplus REQUIRED)

# Fetch llhttp for HTTP parsing
include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    llhttp
    URL https://github.com/nodejs/llhttp/archive/refs/tags/release/v9.3.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(llhttp)

add_library(nog_lib
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/typechecker/typechecker.cpp
    src/typechecker/strings.cpp
    src/codegen/codegen.cpp
    src/project/project.cpp
    src/project/module.cpp
    src/docgen/docgen.cpp
    src/stdlib/http.cpp
)
target_link_libraries(nog_lib fmt::fmt tomlplusplus::tomlplusplus)
target_include_directories(nog_lib PUBLIC ${llhttp_SOURCE_DIR}/include)

add_executable(nog src/main.cpp)
target_link_libraries(nog nog_lib)
