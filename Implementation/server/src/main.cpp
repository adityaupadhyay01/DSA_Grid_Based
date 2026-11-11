// ---------------------------------------------------------------------------
// The backend.
//
// A handful of endpoints over the planning engine, plus the built React app
// served from the same origin. The engine is linked in, so a request reaches
// A* as a function call: there is no second process and nothing is serialised
// between the HTTP layer and the algorithm.
//
//   ./bin/server                       127.0.0.1:8080, API only
//   ./bin/server --web web/dist        also serve the built frontend
//   ./bin/server --port 9000 --host 0.0.0.0 --workers 8
//
// Endpoints
//   GET /api/scenarios   the three maps, their cells and their start/end points
//   GET /api/plan        one route on a built-in map, with the search recorded
//   POST /api/route      the same, on a map the caller drew (JSON body)
//   GET /api/compare     the same route with all six OPEN lists
//   GET /health/live     the process is up
//   GET /health/ready    the process can do useful work
// ---------------------------------------------------------------------------
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "http.hpp"
#include "json.hpp"
#include "planner.hpp"

namespace {

http::Server* g_server = nullptr;
const auto g_started = std::chrono::steady_clock::now();

void on_signal(int) {
    // Only async-signal-safe work here: ask the accept loop to stop and
    // return. Logging from a handler is a good way to deadlock.
    if (g_server) g_server->stop();
}

// --------------------------- error envelope ---------------------------
// The same shape for every failure, so the TypeScript client has one branch.
http::Response fail(int status, const std::string& code, const std::string& message,
                    const std::string& request_id) {
    json::Writer w;
    w.begin_object();
    w.key("error");
    w.begin_object();
    w.field("code", code);
    w.field("message", message);
    w.field("requestId", request_id);
    w.end_object();
    w.end_object();
    return http::Response::json(w.str(), status);
}

// --------------------------- GET /api/scenarios ---------------------------
std::string scenarios_payload() {
    json::Writer w;
    w.begin_object();

    w.key("scenarios");
    w.begin_array();
    for (const auto& s : planner::scenarios()) {
        w.begin_object();
        w.field("id", s.id);
        w.field("name", s.name);
        w.field("blurb", s.blurb);
        w.field("width", s.w);
        w.field("height", s.h);
        w.field("openCells", s.open_cells());

        // One digit per cell. At 1600 characters this is small enough to send
        // whole and trivially cacheable, and the client unpacks it in one pass.
        std::string packed;
        packed.reserve(s.kind.size());
        for (uint8_t k : s.kind) packed.push_back(static_cast<char>('0' + k));
        w.field("cells", packed);

        const auto points = [&](const char* key, const std::vector<scenario::Point>& src) {
            w.key(key);
            w.begin_array();
            for (const auto& p : src) {
                w.begin_object();
                w.field("name", p.name);
                w.field("x", p.x);
                w.field("y", p.y);
                w.end_object();
            }
            w.end_array();
        };
        points("starts", s.starts);
        points("ends", s.ends);
        w.end_object();
    }
    w.end_array();

    w.key("structures");
    w.begin_array();
    for (int i = 0; i < planner::structure_count(); ++i) w.value(planner::structure_name(i));
    w.end_array();

    w.key("cellKindLabels");
    w.begin_object();
    for (uint8_t k = 0; k <= scenario::BLOCKED; ++k)
        w.field(std::to_string(k), scenario::kind_name(k));
    w.end_object();

    w.end_object();
    return w.str();
}

// --------------------------- shared query parsing ---------------------------
struct Query {
    std::string scenarioId;
    int startIndex = 0, endIndex = 0, structure = 1, reps = 5;
};

/** Returns an error response when the query is unusable, or nullopt-ish via ok. */
bool read_query(const http::Request& req, Query* q, http::Response* err, bool want_structure) {
    q->scenarioId = req.param("scenarioId");
    if (q->scenarioId.empty()) {
        *err = fail(400, "BAD_REQUEST", "scenarioId is required", req.id);
        return false;
    }
    const auto bad = [&](const char* key) {
        *err = fail(400, "BAD_REQUEST", std::string(key) + " must be a whole number", req.id);
        return false;
    };
    if (!req.int_param("startIndex", 0, &q->startIndex)) return bad("startIndex");
    if (!req.int_param("endIndex", 0, &q->endIndex)) return bad("endIndex");
    if (!req.int_param("reps", 5, &q->reps)) return bad("reps");
    if (want_structure && !req.int_param("structure", 1, &q->structure)) return bad("structure");

    // Bounds are checked here as well as in the engine, so a hostile query
    // cannot turn into a long-running request.
    if (q->reps < 1 || q->reps > 51) {
        *err = fail(400, "BAD_REQUEST", "reps must be between 1 and 51", req.id);
        return false;
    }
    if (want_structure && (q->structure < 0 || q->structure >= planner::structure_count())) {
        *err = fail(400, "BAD_REQUEST", "structure must be between 0 and 5", req.id);
        return false;
    }
    return true;
}

// --------------------------- the plan response ---------------------------
// One shape for a built-in map (GET /api/plan) and a drawn one (POST
// /api/route), so the browser has a single type to decode.
std::string plan_payload(const planner::PlanResult& r) {
    json::Writer w;
    w.begin_object();
    w.field("engine", "c++");
    w.field("scenarioId", r.scenarioId);
    w.field("structure", r.structure);
    w.field("found", r.found);
    w.field("cost", r.cost);
    w.field("expansions", r.expansions);
    w.field("inserts", r.inserts);
    w.field("extracts", r.extracts);
    w.field("stalePops", r.stalePops);
    w.field("peakOpen", r.peakOpen);
    w.field("openCells", r.openCells);
    w.field("ms", r.ms);
    w.field("reps", r.reps);
    w.field("path", r.path);
    // The order A* really closed cells in. The browser replays this rather
    // than running its own search and claiming the two agree.
    w.field("order", r.order);
    // The grid route shortened to the waypoints a drone would actually fly.
    w.field("smoothPath", r.smoothPath);
    w.field("gridLength", r.gridLength);
    w.field("smoothLength", r.smoothLength);
    w.end_object();
    return w.str();
}

// --------------------------- GET /api/plan ---------------------------
http::Response handle_plan(const http::Request& req) {
    Query q;
    http::Response err;
    if (!read_query(req, &q, &err, true)) return err;

    planner::PlanResult r;
    try {
        r = planner::plan(q.scenarioId, q.startIndex, q.endIndex, q.structure, q.reps);
    } catch (const std::invalid_argument& e) {
        // Bad input is the caller's mistake, not a server fault.
        const std::string what = e.what();
        const int status = what.rfind("no scenario", 0) == 0 ? 404 : 400;
        return fail(status, status == 404 ? "NOT_FOUND" : "BAD_REQUEST", what, req.id);
    }

    return http::Response::json(plan_payload(r));
}

// --------------------------- POST /api/route ---------------------------
// A map the caller drew. It is ten thousand characters, so it travels as a
// JSON body rather than in the URL:
//   { "width":100, "height":100, "cells":"000...", "start":303, "end":9696,
//     "structure":1, "reps":5 }
// `structure` and `reps` are optional. The structure defaults to the binary
// heap, and 0 (the unsorted array, O(n^2) overall) is refused here because on
// a map this size it would turn one request into seconds of work.
http::Response handle_route(const http::Request& req) {
    json::Object body;
    std::string why;
    if (!json::parse_flat_object(req.body, &body, &why))
        return fail(400, "BAD_JSON", why, req.id);

    static const char* const known[] = { "width", "height", "cells", "start", "end",
                                         "structure", "reps" };
    for (const auto& kv : body) {
        bool ok = false;
        for (const char* k : known) if (kv.first == k) ok = true;
        if (!ok) return fail(400, "BAD_REQUEST", "unknown field '" + kv.first + "'", req.id);
    }

    http::Response err;
    const auto integer = [&](const char* key, bool required, int fallback, int* out) {
        const auto it = body.find(key);
        if (it == body.end() || it->second.type == json::Value::kNull) {
            if (required) {
                err = fail(400, "BAD_REQUEST", std::string(key) + " is required", req.id);
                return false;
            }
            *out = fallback;
            return true;
        }
        if (!json::as_int(it->second, out)) {
            err = fail(400, "BAD_REQUEST", std::string(key) + " must be a whole number", req.id);
            return false;
        }
        return true;
    };

    int width = 0, height = 0, start = 0, end = 0, structure = 1, reps = 5;
    if (!integer("width", true, 0, &width) || !integer("height", true, 0, &height) ||
        !integer("start", true, 0, &start) || !integer("end", true, 0, &end) ||
        !integer("structure", false, 1, &structure) || !integer("reps", false, 5, &reps))
        return err;

    const auto cells = body.find("cells");
    if (cells == body.end() || cells->second.type != json::Value::kString)
        return fail(400, "BAD_REQUEST", "cells must be a string of digits", req.id);

    if (reps < 1 || reps > 51)
        return fail(400, "BAD_REQUEST", "reps must be between 1 and 51", req.id);
    if (structure < 1 || structure >= planner::structure_count())
        return fail(400, "BAD_REQUEST",
                    "structure must be between 1 and 5 for a drawn map", req.id);

    planner::PlanResult r;
    try {
        const scenario::Scenario s =
            planner::custom_scenario(width, height, cells->second.text, start, end);
        r = planner::plan_on(s, 0, 0, structure, reps);
    } catch (const std::invalid_argument& e) {
        return fail(400, "BAD_REQUEST", e.what(), req.id);
    }
    return http::Response::json(plan_payload(r));
}

// --------------------------- GET /api/compare ---------------------------
http::Response handle_compare(const http::Request& req) {
    Query q;
    http::Response err;
    if (!read_query(req, &q, &err, false)) return err;

    planner::CompareResult c;
    try {
        c = planner::compare(q.scenarioId, q.startIndex, q.endIndex, q.reps);
    } catch (const std::invalid_argument& e) {
        const std::string what = e.what();
        const int status = what.rfind("no scenario", 0) == 0 ? 404 : 400;
        return fail(status, status == 404 ? "NOT_FOUND" : "BAD_REQUEST", what, req.id);
    }

    bool agree = true;
    for (const auto& t : c.results) if (t.cost != c.results.front().cost) agree = false;

    json::Writer w;
    w.begin_object();
    w.field("engine", "c++");
    w.field("scenarioId", c.scenarioId);
    w.field("reps", c.reps);
    w.key("results");
    w.begin_array();
    for (const auto& t : c.results) {
        w.begin_object();
        w.field("structure", t.structure);
        w.field("ms", t.ms);
        w.field("cost", t.cost);
        w.field("expansions", t.expansions);
        w.field("inserts", t.inserts);
        w.field("extracts", t.extracts);
        w.end_object();
    }
    w.end_array();
    // Six independent implementations agreeing is the check that caught the
    // bucket queue sized at C+1. Surfacing it keeps that check visible.
    w.field("allAgreeOnCost", agree);
    w.end_object();
    return http::Response::json(w.str());
}

// --------------------------- health ---------------------------
http::Response handle_ready(const http::Request&) {
    const bool loaded = !planner::scenarios().empty();
    const auto up = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::steady_clock::now() - g_started).count();

    json::Writer w;
    w.begin_object();
    w.field("status", loaded ? "ok" : "degraded");
    w.key("engine");
    w.begin_object();
    w.field("loaded", loaded);
    w.field("scenarios", static_cast<int>(planner::scenarios().size()));
    w.field("structures", planner::structure_count());
    w.end_object();
    w.field("uptimeSeconds", static_cast<int>(up));
    w.field("version", "2.1.0");
    w.end_object();
    return http::Response::json(w.str(), loaded ? 200 : 503);
}

/**
 * Asks a server already listening on this port whether it is ready, and exits
 * 0 or 1. Container health checks need an HTTP probe, and shipping curl into
 * the runtime image purely for that would be a whole package of attack
 * surface for one request. The binary is already there, so it can probe
 * itself.
 */
int run_healthcheck(const std::string& host, int port) {
    const int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 1;

    timeval tv { 3, 0 };
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (::inet_pton(AF_INET, host == "0.0.0.0" ? "127.0.0.1" : host.c_str(), &addr.sin_addr) != 1) {
        ::close(sock);
        return 1;
    }
    if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::close(sock);
        return 1;
    }

    const std::string req = "GET /health/ready HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    if (::send(sock, req.data(), req.size(), 0) < 0) { ::close(sock); return 1; }

    char buf[256];
    const ssize_t n = ::recv(sock, buf, sizeof(buf) - 1, 0);
    ::close(sock);
    if (n <= 0) return 1;
    buf[n] = '\0';
    return std::strstr(buf, "200 OK") ? 0 : 1;
}

const char* env_or(const char* key, const char* fallback) {
    const char* v = std::getenv(key);
    return (v && *v) ? v : fallback;
}

}  // namespace

int main(int argc, char** argv) {
    std::string host = env_or("HOST", "127.0.0.1");
    std::string web = env_or("WEB_DIST", "");
    int port = std::atoi(env_or("PORT", "8080"));
    int workers = std::atoi(env_or("WORKERS", "4"));
    bool healthcheck = false;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const auto next = [&](const char* what) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << what << " needs a value\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--port") port = std::atoi(next("--port").c_str());
        else if (a == "--host") host = next("--host");
        else if (a == "--web") web = next("--web");
        else if (a == "--workers") workers = std::atoi(next("--workers").c_str());
        else if (a == "--healthcheck") healthcheck = true;
        else if (a == "--help") {
            std::cout << "usage: server [--host H] [--port P] [--web DIR] [--workers N]\n"
                      << "       server --healthcheck    probe a running server, exit 0 if ready\n";
            return 0;
        } else {
            std::cerr << "unknown argument: " << a << "\n";
            return 2;
        }
    }

    if (healthcheck) return run_healthcheck(host, port);

    // Build the scenarios before listening. If a map were malformed, failing
    // at boot is far easier to diagnose than failing on the first request.
    const size_t map_count = planner::scenarios().size();
    if (map_count == 0) {
        std::cerr << "no scenarios built — refusing to start\n";
        return 1;
    }

    http::Server server(host, port, static_cast<size_t>(workers));
    g_server = &server;

    server.route("/api/scenarios", [](const http::Request&) {
        // Fixed data built once, so it is cached by the client too.
        static const std::string payload = scenarios_payload();
        http::Response r = http::Response::json(payload);
        r.headers["Cache-Control"] = "public, max-age=300";
        return r;
    });
    server.route("/api/plan", handle_plan);
    server.route_post("/api/route", handle_route);
    server.route("/api/compare", handle_compare);
    server.route("/health/live", [](const http::Request&) {
        return http::Response::json(R"({"status":"ok"})");
    });
    server.route("/health/ready", handle_ready);

    if (!web.empty()) {
        server.static_root(web);
        server.spa_fallback(true);
    }
    server.set_logger([](const std::string& line) { std::cout << line << std::endl; });

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    std::cout << "Drone path planning backend\n"
              << "  http://" << host << ":" << port << "/\n"
              << "  " << map_count << " maps, " << planner::structure_count()
              << " OPEN-list structures, " << workers << " workers\n"
              << (web.empty() ? "  API only (pass --web web/dist to serve the frontend)\n"
                              : "  serving the frontend from " + web + "\n")
              << std::flush;

    if (!server.listen_and_serve()) {
        std::cerr << "could not bind " << host << ":" << port
                  << " — " << std::strerror(errno) << "\n";
        return 1;
    }
    std::cout << "shut down cleanly" << std::endl;
    return 0;
}
