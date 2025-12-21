/**
 * @file http.hpp
 * @brief Nog HTTP runtime library.
 *
 * Provides HTTP server functionality for Nog programs.
 * Uses boost::fibers with Asio integration for async I/O.
 * HTTP handlers run as go routines (fibers).
 *
 * Non-template implementations are in http.cpp (libnog_http.a).
 * This header is precompiled (http.hpp.gch) for faster compilation.
 */

#pragma once

// Base standard library headers (includes nog::rt namespace with Channel, etc.)
#include <nog/std.hpp>

// Additional headers for HTTP
#include <tuple>

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
 * Reads an HTTP request from a socket (blocking in goroutine context).
 */
Request read_request(boost::asio::ip::tcp::socket& socket);

/**
 * Handle a single connection with a handler.
 * Template must stay in header.
 */
template<typename Handler>
void handle_connection(boost::asio::ip::tcp::socket socket, Handler handler) {
    try {
        Request req = read_request(socket);
        Response resp = handler(req);

        std::string response_str = format_response(resp);
        boost::system::error_code ec;
        boost::asio::write(socket, boost::asio::buffer(response_str), ec);
    } catch (const std::exception& e) {
        // Connection closed or error
    }
}

/**
 * Main serve function - simple single-handler version.
 * Template must stay in header.
 * Runs accept loop in current fiber, spawns handler fibers.
 */
template<typename Handler>
void serve(int port, Handler handler) {
    boost::asio::ip::tcp::acceptor acceptor(nog::rt::io_context());

    try {
        acceptor.open(boost::asio::ip::tcp::v4());
        acceptor.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor.bind({boost::asio::ip::tcp::v4(), static_cast<boost::asio::ip::port_type>(port)});
        acceptor.listen();
    } catch (const boost::system::system_error& e) {
        std::cerr << "Error: Failed to bind to port " << port << ": " << e.what() << std::endl;
        return;
    }

    std::cout << "HTTP server listening on port " << port << std::endl;

    while (true) {
        boost::system::error_code ec;
        boost::asio::ip::tcp::socket socket(nog::rt::io_context());

        // Yield fiber during accept
        acceptor.async_accept(socket, boost::fibers::asio::yield[ec]);

        if (!ec) {
            // Spawn handler as go routine (fiber)
            boost::fibers::fiber([socket = std::move(socket), handler]() mutable {
                handle_connection(std::move(socket), handler);
            }).detach();
        }
    }
}

/**
 * App struct for routing-based HTTP server.
 */
struct App {
    std::vector<std::tuple<std::string, std::string, std::function<Response(Request)>>> routes;

    /**
     * Register a GET route handler.
     */
    void get(const std::string& path, std::function<Response(Request)> handler);

    /**
     * Register a POST route handler.
     */
    void post(const std::string& path, std::function<Response(Request)> handler);

    /**
     * Route a request to the appropriate handler.
     */
    Response route(const Request& req);

    /**
     * Start listening on the given port.
     */
    void listen(int port);
};

}  // namespace http
