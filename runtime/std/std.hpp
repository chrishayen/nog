/**
 * @file std.hpp
 * @brief Base standard library header for all Nog programs.
 *
 * This header is precompiled (std.hpp.gch) for faster compilation.
 * Contains all standard library headers used by generated Nog code.
 */

#pragma once

#include <iostream>
#include <string>
#include <cstdint>
#include <optional>
#include <functional>
#include <memory>
#include <vector>
#include <algorithm>
#include <map>
#include <utility>
#include <thread>
#include <mutex>
#include <chrono>

#include <boost/fiber/all.hpp>
#include <boost/asio.hpp>

// Fiber-Asio integration for unified go routine model
#include <nog/fiber_asio/round_robin.hpp>

// Error handling primitives
#include <nog/error.hpp>

namespace nog::rt {

/**
 * Global io_context for fiber-asio integration.
 * Set by main() before spawning any fibers.
 */
inline std::shared_ptr<boost::asio::io_context> io_ctx;

/**
 * Returns the global io_context for async I/O.
 */
inline boost::asio::io_context& io_context() {
    if (!io_ctx) {
        throw std::runtime_error("io_context not initialized");
    }
    return *io_ctx;
}

/**
 * Typed channel for communication between fibers.
 * Wraps boost::fibers::buffered_channel with send/recv API.
 * Uses capacity of 1 to approximate unbuffered semantics while
 * providing try_pop() for select statement polling.
 */
template<typename T>
class Channel {
public:
    Channel() : ch_(2) {}  // capacity must be power of 2, min 2 for boost::fibers

    /**
     * Send a value through the channel. Blocks until space available.
     */
    void send(const T& value) {
        ch_.push(value);
    }

    /**
     * Receive a value from the channel. Blocks until available.
     */
    T recv() {
        T value;
        ch_.pop(value);
        return value;
    }

    /**
     * Try to receive a value without blocking. Returns pair<bool, T>.
     */
    std::pair<bool, T> try_recv() {
        T value;
        auto status = ch_.try_pop(value);
        if (status == boost::fibers::channel_op_status::success) {
            return {true, value};
        }
        return {false, T{}};
    }

    /**
     * Close the channel.
     */
    void close() {
        ch_.close();
    }

private:
    boost::fibers::buffered_channel<T> ch_;
};

/**
 * Sleeps for the specified number of milliseconds.
 * Yields to other fibers during sleep.
 */
inline void sleep(int ms) {
    boost::this_fiber::sleep_for(std::chrono::milliseconds(ms));
}

}  // namespace nog::rt
