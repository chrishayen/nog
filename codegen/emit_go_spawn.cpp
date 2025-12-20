/**
 * @file emit_go_spawn.cpp
 * @brief Goroutine spawn emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits a goroutine spawn using boost::asio::spawn.
 *
 * Note: Uses [&] capture. The caller must ensure captured variables
 * outlive the goroutine. Proper fix requires shared_ptr for channels.
 */
string emit_go_spawn(CodeGenState& state, const GoSpawn& spawn) {
    string call_code = emit(state, *spawn.call);

    string out = "boost::asio::spawn(*nog::rt::global_io_context, [&](boost::asio::yield_context yield) {\n";
    out += "\t\tnog::rt::YieldScope scope(yield);\n";
    out += "\t\t" + call_code + ";\n";
    out += "\t}, boost::asio::detached)";

    return out;
}

} // namespace codegen
