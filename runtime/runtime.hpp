/**
 * @file runtime.hpp
 * @brief Master include for Nog runtime code generation helpers.
 *
 * This header brings in all the runtime helper functions used by CodeGen
 * to emit C++ code. Each module handles a specific aspect of code generation:
 *   - types.hpp: Nog-to-C++ type mapping
 *   - literals.hpp: String, number, bool, none literals
 *   - expressions.hpp: Binary ops, function calls, method calls
 *   - statements.hpp: Variable decls, assignments, if/while
 *   - functions.hpp: Function definitions
 *   - structs.hpp: Struct definitions, literals, field access
 *   - io.hpp: print() and assert_eq()
 */

#pragma once

#include "types.hpp"
#include "literals.hpp"
#include "expressions.hpp"
#include "statements.hpp"
#include "functions.hpp"
#include "structs.hpp"
#include "io.hpp"
