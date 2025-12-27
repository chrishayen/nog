/**
 * @file http.cpp
 * @brief Bishop HTTP runtime library implementation.
 *
 * Contains non-template function implementations for the HTTP module.
 * Compiled into libbishop_http.a for linking with user programs.
 */

#include <bishop/http.hpp>

namespace http {

namespace detail {

int on_method(llhttp_t* parser, const char* at, size_t len) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->method.assign(at, len);
    return 0;
}

int on_url(llhttp_t* parser, const char* at, size_t len) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->url.assign(at, len);
    return 0;
}

int on_body(llhttp_t* parser, const char* at, size_t len) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->body.append(at, len);
    return 0;
}

int on_message_complete(llhttp_t* parser) {
    auto* ctx = static_cast<HttpParserContext*>(parser->data);
    ctx->message_complete = true;
    return 0;
}

}  // namespace detail

Response text(const std::string& content) {
    return Response{200, "text/plain", content};
}

Response json(const std::string& content) {
    return Response{200, "application/json", content};
}

Response not_found() {
    return Response{404, "text/plain", "Not Found"};
}

std::string format_response(const Response& resp) {
    std::string status_text;

    switch (resp.status) {
        case 200: status_text = "OK"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }

    std::string response = "HTTP/1.1 " + std::to_string(resp.status) + " " + status_text + "\r\n";
    response += "Content-Type: " + resp.content_type + "\r\n";
    response += "Content-Length: " + std::to_string(resp.body.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += resp.body;
    return response;
}

Request read_request(boost::asio::ip::tcp::socket& socket) {
    llhttp_t parser;
    llhttp_settings_t settings;
    llhttp_settings_init(&settings);
    settings.on_method = detail::on_method;
    settings.on_url = detail::on_url;
    settings.on_body = detail::on_body;
    settings.on_message_complete = detail::on_message_complete;

    HttpParserContext ctx;
    llhttp_init(&parser, HTTP_REQUEST, &settings);
    parser.data = &ctx;

    char buffer[8192];
    boost::system::error_code ec;

    // First read - yields fiber until data available
    size_t n = socket.async_read_some(
        boost::asio::buffer(buffer),
        boost::fibers::asio::yield[ec]);

    if (ec) {
        return Request{"GET", "/", ""};
    }

    llhttp_errno err = llhttp_execute(&parser, buffer, n);

    if (err != HPE_OK && !ctx.message_complete) {
        return Request{"GET", "/", ""};
    }

    // Continue reading only if message incomplete (large POST, slow client)
    while (!ctx.message_complete) {
        n = socket.async_read_some(
            boost::asio::buffer(buffer),
            boost::fibers::asio::yield[ec]);

        if (ec) {
            return Request{"GET", "/", ""};
        }

        err = llhttp_execute(&parser, buffer, n);

        if (err != HPE_OK && !ctx.message_complete) {
            return Request{"GET", "/", ""};
        }
    }

    return Request{ctx.method, ctx.url, ctx.body};
}

void App::get(const std::string& path, std::function<Response(Request)> handler) {
    routes.push_back({"GET", path, handler});
}

void App::post(const std::string& path, std::function<Response(Request)> handler) {
    routes.push_back({"POST", path, handler});
}

Response App::route(const Request& req) {
    for (const auto& [method, path, handler] : routes) {
        if (req.method == method && req.path == path) {
            return handler(req);
        }
    }

    return not_found();
}

void App::listen(int port) {
    boost::asio::ip::tcp::acceptor acceptor(bishop::rt::io_context());

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
        boost::asio::ip::tcp::socket socket(bishop::rt::io_context());

        // Yield fiber during accept
        acceptor.async_accept(socket, boost::fibers::asio::yield[ec]);

        if (!ec) {
            auto self = this;

            // Spawn handler as go routine (fiber)
            boost::fibers::fiber([self, socket = std::move(socket)]() mutable {
                handle_connection(std::move(socket), [self](const Request& req) {
                    return self->route(req);
                });
            }).detach();
        }
    }
}

}  // namespace http
