import http.server
import socketserver

PORT = 9742

class MyHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        http.server.SimpleHTTPRequestHandler.end_headers(self)

with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
    print(f"Serving MMapper WASM at http://localhost:{PORT}/mmapper.html")
    print("Press Ctrl+C to stop")
    httpd.serve_forever()
