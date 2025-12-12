/**
 * @file fs.hpp
 * @brief Built-in fs (filesystem) module header.
 *
 * Declares the AST creation functions for the fs module.
 * The actual runtime is in src/runtime/fs/fs.hpp.
 */

#pragma once

#include "parser/ast.hpp"
#include <memory>
#include <string>

namespace nog::stdlib {

/**
 * Creates the AST for the built-in fs module.
 */
std::unique_ptr<Program> create_fs_module();

/**
 * Generates the fs runtime code (empty - uses precompiled header).
 */
std::string generate_fs_runtime();

}  // namespace nog::stdlib
