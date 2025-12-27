/**
 * @file emit_print.cpp
 * @brief Print statement emission for the Bishop code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std;

namespace codegen {

/**
 * Emits std::cout for multiple values.
 */
string print_multi(const vector<string>& args) {
    return fmt::format("std::cout << {} << std::endl", fmt::join(args, " << "));
}

} // namespace codegen
