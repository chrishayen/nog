// Static File Server
// Run with: nog run examples/serve.n
// Then open http://localhost:8080 in your browser
// Serves files from the current directory

import http;
import fs;

fn handle(http.Request req) -> http.Response {
    path := "." + req.path;

    if path == "./" {
        path = "./index.html";
    }

    if !fs.exists(path) {
        return http.not_found();
    }

    if fs.is_dir(path) {
        return http.not_found();
    }

    content := fs.read_file(path);
    return http.text(content);
}

fn main() {
    print("Serving files from current directory on http://localhost:8080");
    http.serve(8080, handle);
}
