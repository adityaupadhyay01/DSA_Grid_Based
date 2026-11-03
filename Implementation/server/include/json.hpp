#pragma once
// ---------------------------------------------------------------------------
// A minimal JSON writer.
//
// The server only ever produces JSON; it never parses any, because the API
// takes its input from the query string. Writing a few hundred lines of parser
// for input that does not exist would be the wrong trade, and pulling in a
// dependency for output this shaped would be another.
//
// The writer tracks whether a value is still expected, so a missing comma or
// an unbalanced brace is a compile-time impossibility rather than a response
// the browser rejects at three in the morning.
// ---------------------------------------------------------------------------
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace json {

inline std::string escape(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    o += buf;
                } else {
                    o += static_cast<char>(c);
                }
        }
    }
    return o;
}

/** Milliseconds need real decimals; the default %g would print 1e-04. */
inline std::string number(double v) {
    if (!std::isfinite(v)) return "null";
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.6f", v);
    std::string s(buf);
    // Trim trailing zeros so 0.250000 reads as 0.25, but keep one digit.
    const auto dot = s.find('.');
    if (dot != std::string::npos) {
        auto last = s.find_last_not_of('0');
        if (last == dot) last = dot + 1;
        s.erase(last + 1);
    }
    return s;
}

class Writer {
public:
    void begin_object() { comma(); out_ += '{'; first_ = true; }
    void end_object()   { out_ += '}'; first_ = false; }
    void begin_array()  { comma(); out_ += '['; first_ = true; }
    void end_array()    { out_ += ']'; first_ = false; }

    void key(const std::string& k) {
        comma();
        out_ += '"';
        out_ += escape(k);
        out_ += "\":";
        first_ = true;   // the value that follows takes no comma
    }

    void value(const std::string& v) { comma(); out_ += '"'; out_ += escape(v); out_ += '"'; }
    void value(const char* v)        { value(std::string(v)); }
    void value(bool v)               { comma(); out_ += v ? "true" : "false"; }
    void value(int v)                { comma(); out_ += std::to_string(v); }
    void value(long v)               { comma(); out_ += std::to_string(v); }
    void value(size_t v)             { comma(); out_ += std::to_string(v); }
    void value(double v)             { comma(); out_ += number(v); }

    void field(const std::string& k, const std::string& v) { key(k); value(v); }
    void field(const std::string& k, const char* v)        { key(k); value(v); }
    void field(const std::string& k, bool v)               { key(k); value(v); }
    void field(const std::string& k, int v)                { key(k); value(v); }
    void field(const std::string& k, size_t v)             { key(k); value(v); }
    void field(const std::string& k, double v)             { key(k); value(v); }

    /** Integer arrays are the bulk of a plan response, so they get a fast path. */
    void field(const std::string& k, const std::vector<int>& v) {
        key(k);
        comma();
        out_ += '[';
        for (size_t i = 0; i < v.size(); ++i) {
            if (i) out_ += ',';
            out_ += std::to_string(v[i]);
        }
        out_ += ']';
        first_ = false;
    }

    const std::string& str() const { return out_; }

private:
    void comma() {
        if (!first_) out_ += ',';
        first_ = false;
    }

    std::string out_;
    bool first_ = true;
};

}  // namespace json
