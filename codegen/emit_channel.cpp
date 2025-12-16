/**
 * @file emit_channel.cpp
 * @brief Channel emission for the Nog code generator.
 */

#include "codegen.hpp"
#include <fmt/format.h>

using namespace std;

namespace codegen {

/**
 * Emits a channel creation using ASIO experimental channel.
 */
string emit_channel_create(const ChannelCreate& channel) {
    string cpp_type = map_type(channel.element_type);
    return "asio::experimental::channel<void(asio::error_code, " + cpp_type + ")>(co_await asio::this_coro::executor, 1)";
}

} // namespace codegen
