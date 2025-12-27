cmake_minimum_required(VERSION 3.16)
project(bishop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR})

find_package(fmt REQUIRED)
find_package(tomlplusplus REQUIRED)
cmake_policy(SET CMP0167 NEW)
find_package(Boost REQUIRED COMPONENTS context fiber)

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

# Bishop compiler library
add_library(bishop_lib
    lexer/lexer.cpp
    parser/parser.cpp
    parser/parse_expression.cpp
    parser/parse_statement.cpp
    parser/parse_function.cpp
    parser/parse_struct.cpp
    parser/parse_error.cpp
    parser/parse_type.cpp
    parser/parse_import.cpp
    parser/parse_ffi.cpp
    parser/parse_return.cpp
    parser/parse_fail.cpp
    parser/parse_if.cpp
    parser/parse_while.cpp
    parser/parse_for.cpp
    parser/parse_select.cpp
    parser/parse_variable.cpp
    parser/parse_comparison.cpp
    parser/parse_additive.cpp
    parser/parse_primary.cpp
    parser/parse_postfix.cpp
    parser/parse_or.cpp
    typechecker/typechecker.cpp
    typechecker/check_expression.cpp
    typechecker/check_statement.cpp
    typechecker/check_function.cpp
    typechecker/check_literals.cpp
    typechecker/check_refs.cpp
    typechecker/check_binary.cpp
    typechecker/check_channel.cpp
    typechecker/check_list.cpp
    typechecker/check_function_call.cpp
    typechecker/check_method_call.cpp
    typechecker/check_field.cpp
    typechecker/check_struct_literal.cpp
    typechecker/check_variable_stmt.cpp
    typechecker/check_return_stmt.cpp
    typechecker/check_if_stmt.cpp
    typechecker/check_while_stmt.cpp
    typechecker/check_for_stmt.cpp
    typechecker/check_select_stmt.cpp
    typechecker/strings.cpp
    typechecker/lists.cpp
    codegen/codegen.cpp
    codegen/emit_type.cpp
    codegen/emit_expression.cpp
    codegen/emit_statement.cpp
    codegen/emit_function.cpp
    codegen/emit_struct.cpp
    codegen/emit_error.cpp
    codegen/emit_fail.cpp
    codegen/emit_or.cpp
    codegen/emit_ffi.cpp
    codegen/emit_if.cpp
    codegen/emit_while.cpp
    codegen/emit_for.cpp
    codegen/emit_select.cpp
    codegen/emit_with.cpp
    codegen/emit_print.cpp
    codegen/emit_assert.cpp
    codegen/emit_variable.cpp
    codegen/emit_literals.cpp
    codegen/emit_refs.cpp
    codegen/emit_binary.cpp
    codegen/emit_go_spawn.cpp
    codegen/emit_channel.cpp
    codegen/emit_list.cpp
    codegen/emit_method_call.cpp
    codegen/emit_function_call.cpp
    codegen/emit_field.cpp
    project/project.cpp
    project/module.cpp
    stdlib/http.cpp
    stdlib/fs.cpp
)
target_link_libraries(bishop_lib fmt::fmt tomlplusplus::tomlplusplus)
target_include_directories(bishop_lib PUBLIC ${llhttp_SOURCE_DIR}/include)

# Bishop compiler executable
add_executable(bishop main.cpp)
target_link_libraries(bishop bishop_lib)

# Copy runtime headers and libraries to build directory for development
add_custom_target(bishop_runtime_headers ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/bishop
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/bishop/fiber_asio
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/bishop/fiber_asio/detail
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/runtime/std/std.hpp
        ${CMAKE_BINARY_DIR}/include/bishop/std.hpp
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/runtime/std/error.hpp
        ${CMAKE_BINARY_DIR}/include/bishop/error.hpp
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/runtime/std/channel.hpp
        ${CMAKE_BINARY_DIR}/include/bishop/channel.hpp
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/runtime/http/http.hpp
        ${CMAKE_BINARY_DIR}/include/bishop/http.hpp
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/runtime/fs/fs.hpp
        ${CMAKE_BINARY_DIR}/include/bishop/fs.hpp
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/runtime/fiber_asio/round_robin.hpp
        ${CMAKE_BINARY_DIR}/include/bishop/fiber_asio/round_robin.hpp
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/runtime/fiber_asio/yield.hpp
        ${CMAKE_BINARY_DIR}/include/bishop/fiber_asio/yield.hpp
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/runtime/fiber_asio/detail/yield.hpp
        ${CMAKE_BINARY_DIR}/include/bishop/fiber_asio/detail/yield.hpp
    COMMAND ${CMAKE_COMMAND} -E copy
        ${llhttp_SOURCE_DIR}/include/llhttp.h
        ${CMAKE_BINARY_DIR}/include/llhttp.h
    COMMENT "Copying runtime headers"
)
add_dependencies(bishop_runtime_headers llhttp_static)

# Copy llhttp library after it's built
add_custom_command(TARGET bishop_runtime_headers POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:llhttp_static>
        ${CMAKE_BINARY_DIR}/lib/
    COMMENT "Copying llhttp library"
)

# Standard runtime library (Asio separate compilation for faster builds)
add_library(bishop_std_runtime STATIC
    runtime/asio_impl.cpp
    runtime/std/runtime.cpp
)
target_compile_features(bishop_std_runtime PRIVATE cxx_std_23)
target_compile_definitions(bishop_std_runtime PRIVATE BOOST_ASIO_SEPARATE_COMPILATION)
target_include_directories(bishop_std_runtime PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(bishop_std_runtime Boost::context Boost::fiber)
add_dependencies(bishop_std_runtime bishop_runtime_headers)

# Copy bishop_std library to lib/ after build
add_custom_command(TARGET bishop_std_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:bishop_std_runtime>
        ${CMAKE_BINARY_DIR}/lib/
    COMMENT "Copying bishop_std runtime library"
)

# HTTP runtime library (compiled once, linked by user programs)
add_library(bishop_http_runtime STATIC
    runtime/http/http.cpp
)
target_compile_features(bishop_http_runtime PRIVATE cxx_std_23)
target_compile_definitions(bishop_http_runtime PRIVATE BOOST_ASIO_SEPARATE_COMPILATION)
target_include_directories(bishop_http_runtime PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/include
    ${llhttp_SOURCE_DIR}/include
)
target_link_libraries(bishop_http_runtime bishop_std_runtime llhttp_static Boost::context Boost::fiber)
add_dependencies(bishop_http_runtime bishop_runtime_headers)

# Copy bishop_http library to lib/ after build
add_custom_command(TARGET bishop_http_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:bishop_http_runtime>
        ${CMAKE_BINARY_DIR}/lib/
    COMMENT "Copying bishop_http runtime library"
)

# Precompile std.hpp for faster compilation of non-http programs
# GCC looks for .gch files in the same directory as the .hpp file
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/include/bishop/std.hpp.gch
    COMMAND ${CMAKE_CXX_COMPILER} -std=c++23
            -I${Boost_INCLUDE_DIRS}
            -I${CMAKE_BINARY_DIR}/include
            -x c++-header
            ${CMAKE_BINARY_DIR}/include/bishop/std.hpp
            -o ${CMAKE_BINARY_DIR}/include/bishop/std.hpp.gch
    DEPENDS bishop_runtime_headers
    COMMENT "Precompiling std.hpp"
)

# Precompile http.hpp for faster compilation of http programs
# http.hpp includes std.hpp, so both are in this PCH
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/include/bishop/http.hpp.gch
    COMMAND ${CMAKE_CXX_COMPILER} -std=c++23
            -I${Boost_INCLUDE_DIRS}
            -I${CMAKE_BINARY_DIR}/include
            -x c++-header
            ${CMAKE_BINARY_DIR}/include/bishop/http.hpp
            -o ${CMAKE_BINARY_DIR}/include/bishop/http.hpp.gch
    DEPENDS bishop_runtime_headers ${CMAKE_BINARY_DIR}/include/bishop/std.hpp.gch
    COMMENT "Precompiling http.hpp"
)

add_custom_target(precompile_headers ALL
    DEPENDS ${CMAKE_BINARY_DIR}/include/bishop/std.hpp.gch
            ${CMAKE_BINARY_DIR}/include/bishop/http.hpp.gch
)

# Documentation generator tool
add_executable(docgen tools/docgen.cpp)
target_compile_features(docgen PRIVATE cxx_std_23)
