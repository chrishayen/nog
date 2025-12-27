/**
 * @file emit_go_spawn.cpp
 * @brief Goroutine spawn emission for the Bishop code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits a goroutine spawn using bishop::rt::spawn().
 *
 * Note: Uses [&] capture. The caller must ensure captured variables
 * outlive the goroutine. Proper fix requires shared_ptr for channels.
 */
string emit_go_spawn(CodeGenState& state, const GoSpawn& spawn) {
    string call_code = emit(state, *spawn.call);

    string out = "bishop::rt::spawn([&]() {\n";
    out += "\t\t" + call_code + ";\n";
    out += "\t})";

    return out;
}

} // namespace codegen
