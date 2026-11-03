#pragma once
// ---------------------------------------------------------------------------
// A small HTTP/1.1 server: POSIX sockets, a fixed worker pool, and nothing else.
//
// Dependency-free on purpose. This has to build with the same one-line g++
// command as the rest of the project, on a lab machine, with no network. A
// project whose demonstration needs `npm install` to work at all has a second
// way to fail in front of an audience.
//
// What it does: parses a request line and headers, routes by path, serves
// static files with a traversal guard, and shuts down cleanly on SIGINT.
// What it does not do: keep-alive, chunked encoding, TLS, uploads. Nothing
// here needs them, and each would be a silent correctness trap if written
// badly.
// ---------------------------------------------------------------------------
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace http {

// --------------------------------------------------------------------------
// Request / response
// --------------------------------------------------------------------------
struct Request {
    std::string method;
    std::string path;
    std::map<std::string, std::string> query;
    std::map<std::string, std::string> headers;
    std::string id;          // echoed back, and printed in the access log

    std::string param(const std::string& key, const std::string& fallback = "") const {
        const auto it = query.find(key);
        return it == query.end() ? fallback : it->second;
    }

    /** Returns false when the value is present but not an integer. */
    bool int_param(const std::string& key, int fallback, int* out) const {
        const auto it = query.find(key);
        if (it == query.end() || it->second.empty()) { *out = fallback; return true; }
        try {
            size_t used = 0;
            const int v = std::stoi(it->second, &used);
            if (used != it->second.size()) return false;
            *out = v;
            return true;
        } catch (...) {
            return false;
        }
    }
};

struct Response {
    int status = 200;
    std::string content_type = "application/json; charset=utf-8";
    std::string body;
    std::map<std::string, std::string> headers;

    static Response json(const std::string& body, int status = 200) {
        Response r;
        r.status = status;
        r.body = body;
        return r;
    }
    static Response text(const std::string& body, int status = 200) {
        Response r;
        r.status = status;
        r.content_type = "text/plain; charset=utf-8";
        r.body = body;
        return r;
    }
};

inline const char* status_text(int code) {
    switch (code) {
        case 200: return "OK";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 503: return "Service Unavailable";
        default:  return "OK";
    }
}

// --------------------------------------------------------------------------
// Percent-decoding, so a query value can contain a space or a slash
// --------------------------------------------------------------------------
inline std::string url_decode(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '+') {
            out += ' ';
        } else if (in[i] == '%' && i + 2 < in.size()) {
            const auto hex = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            const int hi = hex(in[i + 1]), lo = hex(in[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out += static_cast<char>(hi * 16 + lo);
                i += 2;
            } else {
                out += in[i];
            }
        } else {
            out += in[i];
        }
    }
    return out;
}

// --------------------------------------------------------------------------
// Static files
// --------------------------------------------------------------------------
inline std::string content_type_for(const std::string& path) {
    const auto dot = path.find_last_of('.');
    const std::string ext = dot == std::string::npos ? "" : path.substr(dot);
    if (ext == ".html") return "text/html; charset=utf-8";
    if (ext == ".js" || ext == ".mjs") return "text/javascript; charset=utf-8";
    if (ext == ".css")  return "text/css; charset=utf-8";
    if (ext == ".json" || ext == ".map") return "application/json; charset=utf-8";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".png")  return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".ico")  return "image/x-icon";
    if (ext == ".woff2") return "font/woff2";
    if (ext == ".woff")  return "font/woff";
    if (ext == ".txt")  return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

/**
 * Resolves a URL path inside the web root, refusing anything that climbs out.
 * Rejecting ".." textually is not enough once percent-encoding exists, so the
 * check is done on the resolved path.
 */
inline bool resolve_static(const std::string& root, const std::string& url_path,
                           std::string* resolved) {
    if (root.empty()) return false;

    std::string rel = url_path;
    if (!rel.empty() && rel.front() == '/') rel.erase(0, 1);
    if (rel.empty()) rel = "index.html";

    const std::string candidate = root + "/" + rel;

    char real_root[4096], real_file[4096];
    if (!realpath(root.c_str(), real_root)) return false;
    if (!realpath(candidate.c_str(), real_file)) return false;

    const std::string r(real_root), f(real_file);
    if (f.size() < r.size() || f.compare(0, r.size(), r) != 0) return false;   // escaped the root
    if (f.size() > r.size() && f[r.size()] != '/') return false;               // sibling directory

    struct stat st {};
    if (stat(f.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return false;

    *resolved = f;
    return true;
}

inline bool read_file(const std::string& path, std::string* out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    *out = ss.str();
    return true;
}

// --------------------------------------------------------------------------
// The server
// --------------------------------------------------------------------------
using Handler = std::function<Response(const Request&)>;

class Server {
public:
    Server(std::string host, int port, size_t workers)
        : host_(std::move(host)), port_(port), workers_(workers ? workers : 4) {}

    void route(const std::string& path, Handler h) { routes_[path] = std::move(h); }

    /** Files under this directory are served when no route matches. */
    void static_root(std::string dir) { static_root_ = std::move(dir); }

    /** Unmatched paths fall back to this file, so client-side routing works. */
    void spa_fallback(bool on) { spa_ = on; }

    void set_logger(std::function<void(const std::string&)> log) { log_ = std::move(log); }

    bool listen_and_serve() {
        listener_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listener_ < 0) return false;

        int yes = 1;
        ::setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        sockaddr_in addr {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port_));
        addr.sin_addr.s_addr = (host_ == "0.0.0.0") ? htonl(INADDR_ANY) : htonl(INADDR_LOOPBACK);

        if (::bind(listener_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) return false;
        if (::listen(listener_, 64) < 0) return false;

        // A broken pipe on send() would otherwise kill the process when a
        // client disconnects mid-response.
        ::signal(SIGPIPE, SIG_IGN);

        for (size_t i = 0; i < workers_; ++i) threads_.emplace_back([this] { worker(); });

        while (running_.load()) {
            const int conn = ::accept(listener_, nullptr, nullptr);
            if (conn < 0) {
                if (!running_.load()) break;
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push(conn);
            }
            cv_.notify_one();
        }

        cv_.notify_all();
        for (auto& t : threads_) if (t.joinable()) t.join();
        if (listener_ >= 0) ::close(listener_);
        return true;
    }

    void stop() {
        running_.store(false);
        if (listener_ >= 0) ::shutdown(listener_, SHUT_RDWR);
        cv_.notify_all();
    }

private:
    void worker() {
        for (;;) {
            int conn = -1;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return !queue_.empty() || !running_.load(); });
                if (!running_.load() && queue_.empty()) return;
                conn = queue_.front();
                queue_.pop();
            }
            handle(conn);
            ::close(conn);
        }
    }

    void handle(int conn) {
        std::string head;
        char buf[8192];
        // A client that opens a connection and sends nothing must not hold a
        // worker forever.
        timeval tv { 5, 0 };
        ::setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        while (head.find("\r\n\r\n") == std::string::npos) {
            const ssize_t n = ::recv(conn, buf, sizeof(buf), 0);
            if (n <= 0) return;
            head.append(buf, static_cast<size_t>(n));
            if (head.size() > (1u << 20)) {
                send(conn, Response::text("request header too large", 413));
                return;
            }
        }

        Request req;
        req.id = next_id();

        std::istringstream stream(head);
        std::string line;
        std::getline(stream, line);
        {
            std::istringstream first(line);
            std::string target, version;
            first >> req.method >> target >> version;
            parse_target(target, &req);
        }
        while (std::getline(stream, line) && line != "\r") {
            const auto colon = line.find(':');
            if (colon == std::string::npos) continue;
            std::string k = line.substr(0, colon);
            std::string v = line.substr(colon + 1);
            while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.erase(0, 1);
            while (!v.empty() && (v.back() == '\r' || v.back() == ' ')) v.pop_back();
            for (auto& c : k) c = static_cast<char>(::tolower(c));
            req.headers[k] = v;
        }
        if (const auto it = req.headers.find("x-request-id");
            it != req.headers.end() && !it->second.empty() && it->second.size() <= 128) {
            req.id = it->second;
        }

        Response res = dispatch(req);
        send(conn, res);

        if (log_) {
            std::ostringstream l;
            l << req.method << ' ' << req.path << " -> " << res.status
              << ' ' << res.body.size() << "B id=" << req.id;
            log_(l.str());
        }
    }

    Response dispatch(const Request& req) {
        if (req.method != "GET" && req.method != "HEAD") {
            return Response::json(R"({"error":{"code":"METHOD_NOT_ALLOWED","message":"only GET is served"}})", 405);
        }

        const auto it = routes_.find(req.path);
        if (it != routes_.end()) {
            try {
                return it->second(req);
            } catch (const std::exception& e) {
                return Response::json(
                    std::string(R"({"error":{"code":"INTERNAL_ERROR","message":")") + e.what() + R"("}})", 500);
            }
        }

        if (!static_root_.empty()) {
            std::string file;
            if (resolve_static(static_root_, req.path, &file)) {
                std::string body;
                if (read_file(file, &body)) {
                    Response r;
                    r.content_type = content_type_for(file);
                    r.body = std::move(body);
                    // Hashed asset names may be cached hard; index.html must not be.
                    r.headers["Cache-Control"] =
                        req.path.find("/assets/") == 0 ? "public, max-age=31536000, immutable"
                                                       : "no-cache";
                    return r;
                }
            }
            if (spa_ && req.path.rfind("/api", 0) != 0 && req.path.rfind("/health", 0) != 0) {
                std::string body;
                if (read_file(static_root_ + "/index.html", &body)) {
                    Response r;
                    r.content_type = "text/html; charset=utf-8";
                    r.body = std::move(body);
                    r.headers["Cache-Control"] = "no-cache";
                    return r;
                }
            }
        }

        return Response::json(R"({"error":{"code":"NOT_FOUND","message":"no such endpoint"}})", 404);
    }

    static void parse_target(const std::string& target, Request* req) {
        const auto mark = target.find('?');
        req->path = url_decode(target.substr(0, mark));
        if (mark == std::string::npos) return;

        std::istringstream qs(target.substr(mark + 1));
        std::string pair;
        while (std::getline(qs, pair, '&')) {
            if (pair.empty()) continue;
            const auto eq = pair.find('=');
            if (eq == std::string::npos) req->query[url_decode(pair)] = "";
            else req->query[url_decode(pair.substr(0, eq))] = url_decode(pair.substr(eq + 1));
        }
    }

    void send(int conn, const Response& res) {
        std::ostringstream o;
        o << "HTTP/1.1 " << res.status << ' ' << status_text(res.status) << "\r\n"
          << "Content-Type: " << res.content_type << "\r\n"
          << "Content-Length: " << res.body.size() << "\r\n"
          << "X-Content-Type-Options: nosniff\r\n"
          << "Connection: close\r\n";
        for (const auto& [k, v] : res.headers) o << k << ": " << v << "\r\n";
        o << "\r\n" << res.body;

        const std::string out = o.str();
        size_t sent = 0;
        while (sent < out.size()) {
            const ssize_t n = ::send(conn, out.data() + sent, out.size() - sent, 0);
            if (n <= 0) return;
            sent += static_cast<size_t>(n);
        }
    }

    std::string next_id() {
        const unsigned long n = counter_.fetch_add(1);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "req-%08lx", n);
        return buf;
    }

    std::string host_;
    int port_;
    size_t workers_;
    int listener_ = -1;
    std::string static_root_;
    bool spa_ = false;

    std::map<std::string, Handler> routes_;
    std::vector<std::thread> threads_;
    std::queue<int> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_ { true };
    std::atomic<unsigned long> counter_ { 1 };
    std::function<void(const std::string&)> log_;
};

}  // namespace http
