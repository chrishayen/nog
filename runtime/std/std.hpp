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
#include <boost/asio/spawn.hpp>

namespace nog::rt {

/**
 * Thread-local yield context for Asio integration (used by HTTP).
 */
inline thread_local boost::asio::yield_context* current_yield = nullptr;

/**
 * Global io_context for async I/O (used by HTTP).
 */
inline boost::asio::io_context* global_io_context = nullptr;

/**
 * Returns the current yield context (for HTTP async I/O).
 */
inline boost::asio::yield_context& yield() {
    if (!current_yield) {
        throw std::runtime_error("blocking operation used outside async context");
    }
    return *current_yield;
}

/**
 * Returns the global io_context (for HTTP).
 */
inline boost::asio::io_context& io_context() {
    if (!global_io_context) {
        throw std::runtime_error("io_context not initialized");
    }
    return *global_io_context;
}

/**
 * RAII scope that sets the current yield context (for HTTP).
 */
class YieldScope {
public:
    explicit YieldScope(boost::asio::yield_context& y) : prev_(current_yield) {
        current_yield = &y;
    }
    ~YieldScope() { current_yield = prev_; }
    YieldScope(const YieldScope&) = delete;
    YieldScope& operator=(const YieldScope&) = delete;
private:
    boost::asio::yield_context* prev_;
};

/**
 * Typed channel for communication between fibers.
 * Wraps boost::fibers::buffered_channel with send/recv API.
 * Uses capacity of 1 to approximate unbuffered semantics while
 * providing try_pop() for select statement polling.
 */
template<typename T>
class Channel {
public:
    Channel() : ch_(2) {}  // capacity of 2 (minimum for Boost.Fiber)

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
