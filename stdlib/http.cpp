/**
 * @file http.cpp
 * @brief Built-in HTTP module implementation.
 *
 * Creates the AST definitions for the http module.
 * The actual runtime is in src/runtime/http/http.hpp and linked as a library.
 */

/**
 * @nog_struct Request
 * @module http
 * @description Represents an incoming HTTP request.
 * @field method str - HTTP method (GET, POST, PUT, DELETE, etc.)
 * @field path str - Request path (e.g., "/users/123")
 * @field body str - Request body content
 * @example
 * async fn handle(http.Request req) -> http.Response {
 *     if req.method == "POST" {
 *         return http.text("Received: " + req.body);
 *     }
 *     return http.not_found();
 * }
 */

/**
 * @nog_struct Response
 * @module http
 * @description Represents an HTTP response to send back to the client.
 * @field status int - HTTP status code (200, 404, 500, etc.)
 * @field content_type str - Content-Type header value
 * @field body str - Response body content
 * @example
 * resp := http.Response { status: 200, content_type: "text/html", body: "<h1>Hello</h1>" };
 */

/**
 * @nog_struct App
 * @module http
 * @description HTTP application with routing support.
 * @example
 * app := http.App {};
 * app.get("/", home_handler);
 * app.post("/submit", submit_handler);
 * await app.listen(8080);
 */

/**
 * @nog_fn text
 * @module http
 * @description Creates a 200 OK response with text/plain content type.
 * @param content str - Response body text
 * @returns http.Response - A text response
 * @example return http.text("Hello, World!");
 */

/**
 * @nog_fn json
 * @module http
 * @description Creates a 200 OK response with application/json content type.
 * @param content str - JSON string to send
 * @returns http.Response - A JSON response
 * @example return http.json("{\"status\": \"ok\"}");
 */

/**
 * @nog_fn not_found
 * @module http
 * @description Creates a 404 Not Found response.
 * @returns http.Response - A 404 response
 * @example
 * if path == "/unknown" {
 *     return http.not_found();
 * }
 */

/**
 * @nog_fn serve
 * @module http
 * @async
 * @description Starts an HTTP server on the specified port with a single handler function.
 * @param port int - Port number to listen on
 * @param handler fn(http.Request) -> http.Response - Handler function for all requests
 * @example
 * async fn handle(http.Request req) -> http.Response {
 *     return http.text("Hello!");
 * }
 * await http.serve(8080, handle);
 */

/**
 * @nog_method get
 * @type http.App
 * @description Registers a handler for GET requests at the specified path.
 * @param path str - URL path to match
 * @param handler fn(http.Request) -> http.Response - Handler function
 * @example
 * app.get("/", home);
 * app.get("/about", about);
 */

/**
 * @nog_method post
 * @type http.App
 * @description Registers a handler for POST requests at the specified path.
 * @param path str - URL path to match
 * @param handler fn(http.Request) -> http.Response - Handler function
 * @example app.post("/submit", handle_submit);
 */

/**
 * @nog_method listen
 * @type http.App
 * @async
 * @description Starts the HTTP server and begins listening for requests.
 * @param port int - Port number to listen on
 * @example await app.listen(8080);
 */

#include "http.hpp"

using namespace std;

namespace nog::stdlib {

/**
 * Checks if a module name is a built-in stdlib module.
 */
bool is_builtin_module(const string& name) {
    return name == "http" || name == "fs";
}

/**
 * Creates the AST for the built-in http module.
 */
unique_ptr<Program> create_http_module() {
    auto program = make_unique<Program>();

    // Request :: struct { method str, path str, body str }
    auto request = make_unique<StructDef>();
    request->name = "Request";
    request->visibility = Visibility::Public;
    request->fields.push_back({"method", "str", ""});
    request->fields.push_back({"path", "str", ""});
    request->fields.push_back({"body", "str", ""});
    program->structs.push_back(move(request));

    // Response :: struct { status int, content_type str, body str }
    auto response = make_unique<StructDef>();
    response->name = "Response";
    response->visibility = Visibility::Public;
    response->fields.push_back({"status", "int", ""});
    response->fields.push_back({"content_type", "str", ""});
    response->fields.push_back({"body", "str", ""});
    program->structs.push_back(move(response));

    // fn text(str content) -> http.Response
    auto text_fn = make_unique<FunctionDef>();
    text_fn->name = "text";
    text_fn->visibility = Visibility::Public;
    text_fn->params.push_back({"str", "content"});
    text_fn->return_type = "http.Response";
    program->functions.push_back(move(text_fn));

    // fn json(str content) -> http.Response
    auto json_fn = make_unique<FunctionDef>();
    json_fn->name = "json";
    json_fn->visibility = Visibility::Public;
    json_fn->params.push_back({"str", "content"});
    json_fn->return_type = "http.Response";
    program->functions.push_back(move(json_fn));

    // fn not_found() -> http.Response
    auto not_found_fn = make_unique<FunctionDef>();
    not_found_fn->name = "not_found";
    not_found_fn->visibility = Visibility::Public;
    not_found_fn->return_type = "http.Response";
    program->functions.push_back(move(not_found_fn));

    // fn serve(int port, fn(http.Request) -> http.Response handler)
    auto serve_fn = make_unique<FunctionDef>();
    serve_fn->name = "serve";
    serve_fn->visibility = Visibility::Public;
    serve_fn->params.push_back({"int", "port"});
    serve_fn->params.push_back({"fn(http.Request) -> http.Response", "handler"});
    program->functions.push_back(move(serve_fn));

    // App :: struct { } (internal routing state managed in C++)
    auto app_struct = make_unique<StructDef>();
    app_struct->name = "App";
    app_struct->visibility = Visibility::Public;
    program->structs.push_back(move(app_struct));

    // App :: get(self, str path, fn(http.Request) -> http.Response handler)
    auto get_method = make_unique<MethodDef>();
    get_method->struct_name = "App";
    get_method->name = "get";
    get_method->visibility = Visibility::Public;
    get_method->params.push_back({"http.App", "self"});
    get_method->params.push_back({"str", "path"});
    get_method->params.push_back({"fn(http.Request) -> http.Response", "handler"});
    program->methods.push_back(move(get_method));

    // App :: post(self, str path, fn(http.Request) -> http.Response handler)
    auto post_method = make_unique<MethodDef>();
    post_method->struct_name = "App";
    post_method->name = "post";
    post_method->visibility = Visibility::Public;
    post_method->params.push_back({"http.App", "self"});
    post_method->params.push_back({"str", "path"});
    post_method->params.push_back({"fn(http.Request) -> http.Response", "handler"});
    program->methods.push_back(move(post_method));

    // App :: listen(self, int port)
    auto listen_method = make_unique<MethodDef>();
    listen_method->struct_name = "App";
    listen_method->name = "listen";
    listen_method->visibility = Visibility::Public;
    listen_method->params.push_back({"http.App", "self"});
    listen_method->params.push_back({"int", "port"});
    program->methods.push_back(move(listen_method));

    return program;
}

/**
 * Returns empty - http.hpp is now included at the top of generated code
 * for precompiled header support. GCC requires PCH to be the first include.
 */
string generate_http_runtime() {
    return "";
}

}  // namespace nog::stdlib
