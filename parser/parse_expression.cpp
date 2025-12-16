/**
 * @file parse_expression.cpp
 * @brief Expression parsing entry point for the Nog parser.
 *
 * Delegates to parse_comparison (lowest precedence).
 * Individual precedence levels are in separate files.
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * Entry point for expression parsing. Delegates to comparison (lowest precedence).
 */
unique_ptr<ASTNode> parse_expression(ParserState& state) {
    return parse_comparison(state);
}

} // namespace parser
