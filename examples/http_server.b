// Simple HTTP Server Example
// Run with: bishop run examples/http_server.b
// Then open http://localhost:8080 in your browser

import http;

fn handle_request(http.Request req) -> http.Response {
    if req.path == "/" {
        return http.text("Hello from Bishop HTTP Server!");
    }

    if req.path == "/about" {
        return http.text("Bishop is a modern programming language");
    }

    return http.not_found();
}

fn main() {
    http.serve(8080, handle_request);
}
