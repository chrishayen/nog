/**
 * @file runtime.cpp
 * @brief Bishop fiber runtime implementation.
 *
 * Contains the boost fiber/asio implementation, hidden from user code.
 * User code only sees declarations in std.hpp.
 */

#define BOOST_ASIO_SEPARATE_COMPILATION
#include <boost/fiber/all.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include <bishop/fiber_asio/round_robin.hpp>

#include <functional>
#include <memory>
#include <chrono>

namespace bishop::rt {

// Global io_context - initialized by run()
static std::shared_ptr<boost::asio::io_context> g_io_ctx;

void init_runtime() {
    g_io_ctx = std::make_shared<boost::asio::io_context>();
    boost::fibers::use_scheduling_algorithm<
        boost::fibers::asio::round_robin>(g_io_ctx);
}

void run(std::function<void()> main_fn) {
    init_runtime();
    boost::fibers::fiber(main_fn).join();
}

void run_in_fiber(std::function<void()> fn) {
    boost::fibers::fiber(fn).join();
}

void spawn(std::function<void()> fn) {
    boost::fibers::fiber(fn).detach();
}

void sleep_ms(int ms) {
    boost::this_fiber::sleep_for(std::chrono::milliseconds(ms));
}

void yield() {
    boost::this_fiber::yield();
}

boost::asio::io_context& io_context() {
    if (!g_io_ctx) {
        throw std::runtime_error("Runtime not initialized");
    }
    return *g_io_ctx;
}

}  // namespace bishop::rt
