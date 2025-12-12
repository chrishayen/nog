/**
 * @file http.hpp
 * @brief Nog HTTP runtime library.
 *
 * Provides async HTTP server functionality for Nog programs.
 * Uses ASIO for async I/O and llhttp for HTTP parsing.
 *
 * Non-template implementations are in http.cpp (libnog_http.a).
 * This header is precompiled (http.hpp.gch) for faster compilation.
 */

#pragma once

// Base standard library headers (from std.hpp PCH)
#include <nog/std.hpp>

// Additional headers for HTTP
#include <tuple>

// ASIO headers (the slow ones to compile)
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/experimental/channel.hpp>
#include <asio/experimental/awaitable_operators.hpp>

// HTTP parser
#include <llhttp.h>

namespace http {

/**
 * HTTP request structure.
 */
struct Request {
    std::string method;
    std::string path;
    std::string body;
};

/**
 * HTTP response structure.
 */
struct Response {
    int status;
    std::string content_type;
    std::string body;
};

/**
 * HTTP parser context for llhttp.
 */
struct HttpParserContext {
    std::string method;
    std::string url;
    std::string body;
    bool message_complete = false;
};

namespace detail {

/**
 * llhttp callback for method.
 */
int on_method(llhttp_t* parser, const char* at, size_t len);

/**
 * llhttp callback for URL.
 */
int on_url(llhttp_t* parser, const char* at, size_t len);

/**
 * llhttp callback for body.
 */
int on_body(llhttp_t* parser, const char* at, size_t len);

/**
 * llhttp callback for message complete.
 */
int on_message_complete(llhttp_t* parser);

}  // namespace detail

/**
 * Creates a 200 OK text/plain response.
 */
Response text(const std::string& content);

/**
 * Creates a 200 OK application/json response.
 */
Response json(const std::string& content);

/**
 * Creates a 404 Not Found response.
 */
Response not_found();

/**
 * Formats an HTTP response for sending over the wire.
 */
std::string format_response(const Response& resp);

/**
 * Async request reader - reads until complete HTTP message is received.
 */
asio::awaitable<Request> read_request(asio::ip::tcp::socket& socket);

/**
 * Handle a single connection with an async handler.
 * Template must stay in header.
 */
template<typename Handler>
asio::awaitable<void> handle_connection_async(asio::ip::tcp::socket socket, Handler handler) {
    try {
        Request req = co_await read_request(socket);
        Response resp = co_await handler(req);

        std::string response_str = format_response(resp);
        co_await asio::async_write(socket, asio::buffer(response_str), asio::use_awaitable);
    } catch (const std::exception& e) {
        // Connection closed or error
    }
}

/**
 * Main serve function - simple single-handler version.
 * Handler must be an async function returning asio::awaitable<Response>.
 * Template must stay in header.
 */
template<typename Handler>
asio::awaitable<void> serve(int port, Handler handler) {
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor(executor);

    try {
        acceptor.open(asio::ip::tcp::v4());
        acceptor.set_option(asio::socket_base::reuse_address(true));
        acceptor.bind({asio::ip::tcp::v4(), static_cast<asio::ip::port_type>(port)});
        acceptor.listen();
    } catch (const std::system_error& e) {
        std::cerr << "Error: Failed to bind to port " << port << ": " << e.what() << std::endl;
        co_return;
    }

    std::cout << "HTTP server listening on port " << port << std::endl;

    while (true) {
        asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(executor, handle_connection_async(std::move(socket), handler), asio::detached);
    }
}

/**
 * App struct for routing-based HTTP server.
 */
struct App {
    std::vector<std::tuple<std::string, std::string, std::function<asio::awaitable<Response>(Request)>>> routes;

    /**
     * Register a GET route handler.
     */
    void get(const std::string& path, std::function<asio::awaitable<Response>(Request)> handler);

    /**
     * Register a POST route handler.
     */
    void post(const std::string& path, std::function<asio::awaitable<Response>(Request)> handler);

    /**
     * Route a request to the appropriate handler.
     */
    asio::awaitable<Response> route(const Request& req);

    /**
     * Start listening on the given port.
     */
    asio::awaitable<void> listen(int port);
};

}  // namespace http
