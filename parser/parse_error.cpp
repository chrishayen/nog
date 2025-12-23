/**
 * @file parse_error.cpp
 * @brief Error type parsing for the Nog parser.
 *
 * Handles error definitions: MyError :: err; and MyError :: err { fields }
 */

#include "parser.hpp"

using namespace std;

namespace parser {

/**
 * @nog_syntax Error Definition
 * @category Error Handling
 * @order 1
 * @description Define a custom error type.
 * @syntax Name :: err; or Name :: err { field type, ... }
 * @example
 * ParseError :: err;
 * IOError :: err { code int, path str }
 */
unique_ptr<ErrorDef> parse_error_def(ParserState& state, const string& name, Visibility vis) {
    Token err_tok = consume(state, TokenType::ERR);

    auto def = make_unique<ErrorDef>();
    def->name = name;
    def->visibility = vis;
    def->line = err_tok.line;

    // Check for optional field block or semicolon
    if (check(state, TokenType::SEMICOLON)) {
        advance(state);
        return def;
    }

    if (check(state, TokenType::LBRACE)) {
        advance(state);

        // Parse fields: name type, name type (with optional doc comments)
        while (!check(state, TokenType::RBRACE) && !check(state, TokenType::EOF_TOKEN)) {
            // Collect doc comment for this field
            string field_doc = collect_doc_comments(state);

            StructField field;
            field.name = consume(state, TokenType::IDENT).value;
            field.doc_comment = field_doc;

            if (is_type_token(state)) {
                field.type = token_to_type(current(state).type);
                advance(state);
            } else if (check(state, TokenType::IDENT)) {
                // Custom type (another struct or error)
                field.type = current(state).value;
                advance(state);
            }

            def->fields.push_back(field);

            if (check(state, TokenType::COMMA)) {
                advance(state);
            }
        }

        consume(state, TokenType::RBRACE);
    }

    return def;
}

} // namespace parser
