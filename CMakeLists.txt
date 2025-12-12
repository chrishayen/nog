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

# Nog compiler library
add_library(nog_lib
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/typechecker/typechecker.cpp
    src/typechecker/strings.cpp
    src/codegen/codegen.cpp
    src/project/project.cpp
    src/project/module.cpp
    src/stdlib/http.cpp
)
target_link_libraries(nog_lib fmt::fmt tomlplusplus::tomlplusplus)
target_include_directories(nog_lib PUBLIC ${llhttp_SOURCE_DIR}/include)

# Nog compiler executable
add_executable(nog src/main.cpp)
target_link_libraries(nog nog_lib)

# Copy runtime headers and libraries to build directory for development
add_custom_target(nog_runtime_headers ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/nog
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/src/runtime/std/std.hpp
        ${CMAKE_BINARY_DIR}/include/nog/std.hpp
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/src/runtime/http/http.hpp
        ${CMAKE_BINARY_DIR}/include/nog/http.hpp
    COMMAND ${CMAKE_COMMAND} -E copy
        ${llhttp_SOURCE_DIR}/include/llhttp.h
        ${CMAKE_BINARY_DIR}/include/llhttp.h
    COMMENT "Copying runtime headers"
)
add_dependencies(nog_runtime_headers llhttp_static)

# Copy llhttp library after it's built
add_custom_command(TARGET nog_runtime_headers POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:llhttp_static>
        ${CMAKE_BINARY_DIR}/lib/
    COMMENT "Copying llhttp library"
)

# HTTP runtime library (compiled once, linked by user programs)
add_library(nog_http_runtime STATIC
    src/runtime/http/http.cpp
)
target_compile_features(nog_http_runtime PRIVATE cxx_std_23)
target_include_directories(nog_http_runtime PRIVATE
    ${CMAKE_SOURCE_DIR}/external/asio/include
    ${CMAKE_BINARY_DIR}/include
    ${llhttp_SOURCE_DIR}/include
)
target_link_libraries(nog_http_runtime llhttp_static)
add_dependencies(nog_http_runtime nog_runtime_headers)

# Copy nog_http library to lib/ after build
add_custom_command(TARGET nog_http_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:nog_http_runtime>
        ${CMAKE_BINARY_DIR}/lib/
    COMMENT "Copying nog_http runtime library"
)

# Precompile std.hpp for faster compilation of non-http programs
# GCC looks for .gch files in the same directory as the .hpp file
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/include/nog/std.hpp.gch
    COMMAND ${CMAKE_CXX_COMPILER} -std=c++23
            -x c++-header
            ${CMAKE_BINARY_DIR}/include/nog/std.hpp
            -o ${CMAKE_BINARY_DIR}/include/nog/std.hpp.gch
    DEPENDS nog_runtime_headers
    COMMENT "Precompiling std.hpp"
)

# Precompile http.hpp for faster compilation of http programs
# http.hpp includes std.hpp, so both are in this PCH
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/include/nog/http.hpp.gch
    COMMAND ${CMAKE_CXX_COMPILER} -std=c++23
            -I${CMAKE_SOURCE_DIR}/external/asio/include
            -I${CMAKE_BINARY_DIR}/include
            -x c++-header
            ${CMAKE_BINARY_DIR}/include/nog/http.hpp
            -o ${CMAKE_BINARY_DIR}/include/nog/http.hpp.gch
    DEPENDS nog_runtime_headers ${CMAKE_BINARY_DIR}/include/nog/std.hpp.gch
    COMMENT "Precompiling http.hpp"
)

add_custom_target(precompile_headers ALL
    DEPENDS ${CMAKE_BINARY_DIR}/include/nog/std.hpp.gch
            ${CMAKE_BINARY_DIR}/include/nog/http.hpp.gch
)

# Documentation generator tool
add_executable(docgen tools/docgen.cpp)
target_compile_features(docgen PRIVATE cxx_std_23)
