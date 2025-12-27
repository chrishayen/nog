/**
 * @file std.hpp
 * @brief Base standard library header for all Bishop programs.
 *
 * This header is lightweight - no boost includes.
 * Boost-dependent features (Channel) are in separate headers.
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

// Error handling primitives
#include <bishop/error.hpp>

namespace bishop::rt {

// ============================================================================
// Runtime Functions (implemented in runtime.cpp)
// ============================================================================

/**
 * Initialize the fiber-asio scheduler.
 * Called automatically by run(), but can be called manually for tests.
 */
void init_runtime();

/**
 * Initialize and run the main function in a fiber context.
 * Sets up the fiber-asio scheduler.
 */
void run(std::function<void()> main_fn);

/**
 * Run a function in a fiber and wait for completion.
 * Assumes runtime is already initialized.
 */
void run_in_fiber(std::function<void()> fn);

/**
 * Spawn a new fiber (goroutine).
 */
void spawn(std::function<void()> fn);

/**
 * Sleep for the specified milliseconds, yielding to other fibers.
 */
void sleep_ms(int ms);

/**
 * Yield to other fibers.
 */
void yield();

// ============================================================================
// Arena Allocator (header-only, no boost dependency)
// ============================================================================

/**
 * Simple arena (bump) allocator for per-function memory management.
 */
class Arena {
public:
    static constexpr size_t DEFAULT_BLOCK_SIZE = 64 * 1024;

    Arena() = default;
    ~Arena() { reset(); }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    void* alloc(size_t size, size_t alignment = alignof(std::max_align_t)) {
        size_t aligned_offset = (offset_ + alignment - 1) & ~(alignment - 1);
        size_t new_offset = aligned_offset + size;

        if (blocks_.empty() || new_offset > current_block_size_) {
            allocate_block(std::max(size, DEFAULT_BLOCK_SIZE));
            aligned_offset = 0;
            new_offset = size;
        }

        void* ptr = static_cast<char*>(blocks_.back()) + aligned_offset;
        offset_ = new_offset;
        return ptr;
    }

    void reset() {
        for (void* block : blocks_) {
            std::free(block);
        }
        blocks_.clear();
        offset_ = 0;
        current_block_size_ = 0;
    }

private:
    void allocate_block(size_t size) {
        void* block = std::malloc(size);
        if (!block) throw std::bad_alloc();
        blocks_.push_back(block);
        current_block_size_ = size;
        offset_ = 0;
    }

    std::vector<void*> blocks_;
    size_t offset_ = 0;
    size_t current_block_size_ = 0;
};

inline thread_local std::vector<Arena*> arena_stack;

inline Arena* current_arena() {
    if (arena_stack.empty()) return nullptr;
    return arena_stack.back();
}

class FunctionArena {
public:
    FunctionArena() { arena_stack.push_back(&arena_); }
    ~FunctionArena() { arena_stack.pop_back(); }
    Arena& arena() { return arena_; }
private:
    Arena arena_;
};

template<typename T, typename... Args>
T* arena_new(Args&&... args) {
    Arena* arena = current_arena();
    if (!arena) {
        return new T(std::forward<Args>(args)...);
    }
    void* mem = arena->alloc(sizeof(T), alignof(T));
    return new(mem) T(std::forward<Args>(args)...);
}

}  // namespace bishop::rt

// Legacy compatibility - inline wrapper for sleep
namespace bishop::rt {
    inline void sleep(int ms) { sleep_ms(ms); }
}
