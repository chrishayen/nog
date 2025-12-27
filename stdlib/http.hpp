/**
 * @file http.hpp
 * @brief Built-in HTTP module for Bishop.
 *
 * Provides HTTP server functionality using ASIO for networking
 * and llhttp for HTTP parsing. Defines Request/Response types
 * and helper functions.
 */

#pragma once
#include "parser/ast.hpp"
#include <memory>
#include <string>

namespace nog::stdlib {

/**
 * Creates the AST for the built-in http module.
 * Contains:
 * - Request struct (method, path, body fields)
 * - Response struct (status, content_type, body fields)
 * - text(str) -> Response function
 * - json(str) -> Response function
 * - not_found() -> Response function
 * - serve(int, fn(Request) -> Response) async function
 */
std::unique_ptr<Program> create_http_module();

/**
 * Generates the C++ runtime code for the http module.
 * Includes ASIO TCP acceptor, llhttp parser integration,
 * and the serve loop implementation.
 */
std::string generate_http_runtime();

/**
 * Checks if a module name is a built-in stdlib module.
 */
bool is_builtin_module(const std::string& name);

}  // namespace nog::stdlib
