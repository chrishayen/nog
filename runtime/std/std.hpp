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

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

namespace nog::rt {

/**
 * Thread-local yield context for the current goroutine.
 */
inline thread_local boost::asio::yield_context* current_yield = nullptr;

/**
 * Global io_context for the runtime.
 */
inline boost::asio::io_context* global_io_context = nullptr;

/**
 * Returns the current yield context.
 */
inline boost::asio::yield_context& yield() {
    if (!current_yield) {
        throw std::runtime_error("blocking operation used outside goroutine context");
    }
    return *current_yield;
}

/**
 * Returns the global io_context.
 */
inline boost::asio::io_context& io_context() {
    if (!global_io_context) {
        throw std::runtime_error("io_context not initialized");
    }
    return *global_io_context;
}

/**
 * RAII scope that sets the current yield context.
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
 * Typed channel for communication between goroutines.
 *
 * Thread-safe implementation using mutex for multi-threaded io_context.
 * Goroutines can run in parallel on multiple OS threads.
 */
template<typename T>
class Channel {
public:
    Channel() : has_value_(false), closed_(false) {}

    /**
     * Send a value through the channel. Blocks until received.
     */
    void send(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);

        while (has_value_ && !closed_) {
            lock.unlock();
            boost::asio::post(nog::rt::io_context(), nog::rt::yield());
            lock.lock();
        }

        if (closed_) {
            throw std::runtime_error("send on closed channel");
        }

        value_ = value;
        has_value_ = true;
    }

    /**
     * Receive a value from the channel. Blocks until available.
     */
    T recv() {
        std::unique_lock<std::mutex> lock(mutex_);

        while (!has_value_ && !closed_) {
            lock.unlock();
            boost::asio::post(nog::rt::io_context(), nog::rt::yield());
            lock.lock();
        }

        if (!has_value_ && closed_) {
            throw std::runtime_error("receive on closed channel");
        }

        T result = std::move(value_);
        has_value_ = false;
        return result;
    }

    /**
     * Try to receive a value without blocking. Returns pair<bool, T>.
     */
    std::pair<bool, T> try_recv() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (has_value_) {
            T result = std::move(value_);
            has_value_ = false;
            return {true, result};
        }

        return {false, T{}};
    }

    /**
     * Check if channel has a value ready.
     */
    bool ready() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return has_value_;
    }

    /**
     * Close the channel.
     */
    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
    }

private:
    T value_;
    bool has_value_;
    bool closed_;
    mutable std::mutex mutex_;
};

}  // namespace nog::rt
