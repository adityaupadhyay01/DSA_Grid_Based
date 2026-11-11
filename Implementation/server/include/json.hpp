#pragma once
// ---------------------------------------------------------------------------
// A minimal JSON writer, and a deliberately narrow JSON reader.
//
// Almost everything the API takes arrives in the query string. The one
// exception is a map somebody has drawn, which is ten thousand characters and
// cannot sensibly live in a URL, so POST /api/route carries it as a JSON body.
//
// That body is always a single flat object of strings, numbers and booleans.
// The reader accepts exactly that and refuses everything else, including
// nested objects and arrays: a parser that accepts less is a parser with less
// to get wrong, and nothing here needs more.
//
// The writer tracks whether a value is still expected, so a missing comma or
// an unbalanced brace is a compile-time impossibility rather than a response
// the browser rejects at three in the morning.
// ---------------------------------------------------------------------------
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
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

// ---------------------------------------------------------------------------
// Reading
// ---------------------------------------------------------------------------
struct Value {
    enum Type { kNull, kBool, kNumber, kString };
    Type type = kNull;
    bool boolean = false;
    double number = 0.0;
    std::string text;
};

using Object = std::map<std::string, Value>;

namespace detail {

inline void append_utf8(std::string* out, unsigned long cp) {
    if (cp < 0x80) {
        *out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        *out += static_cast<char>(0xC0 | (cp >> 6));
        *out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        *out += static_cast<char>(0xE0 | (cp >> 12));
        *out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        *out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        *out += static_cast<char>(0xF0 | (cp >> 18));
        *out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        *out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        *out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

class Parser {
public:
    static const size_t kMaxFields = 32;

    Parser(const std::string& text, std::string* why) : s_(text), why_(why) {}

    bool object(Object* out) {
        skip();
        if (!take('{')) return fail("the body must be a JSON object");
        skip();
        if (peek() == '}') { ++i_; return finish(); }

        for (;;) {
            skip();
            if (peek() != '"') return fail("expected a quoted field name");
            std::string key;
            if (!string(&key)) return false;
            skip();
            if (!take(':')) return fail("expected ':' after a field name");
            skip();

            Value v;
            if (!value(&v)) return false;
            if (out->count(key)) return fail("duplicate field '" + key + "'");
            if (out->size() >= kMaxFields) return fail("too many fields");
            (*out)[key] = v;

            skip();
            if (take(',')) continue;
            if (take('}')) return finish();
            return fail("expected ',' or '}'");
        }
    }

private:
    bool fail(const std::string& why) {
        if (why_) *why_ = why;
        return false;
    }
    bool finish() {
        skip();
        if (i_ != s_.size()) return fail("unexpected text after the JSON object");
        return true;
    }
    char peek() const { return i_ < s_.size() ? s_[i_] : '\0'; }
    bool take(char c) {
        if (i_ < s_.size() && s_[i_] == c) { ++i_; return true; }
        return false;
    }
    void skip() {
        while (i_ < s_.size() &&
               (s_[i_] == ' ' || s_[i_] == '\t' || s_[i_] == '\n' || s_[i_] == '\r')) ++i_;
    }
    bool word(const char* w) {
        const std::string t(w);
        if (s_.compare(i_, t.size(), t) != 0) return false;
        i_ += t.size();
        return true;
    }
    static bool digit(char c) { return c >= '0' && c <= '9'; }

    bool value(Value* v) {
        const char c = peek();
        if (c == '"') { v->type = Value::kString; return string(&v->text); }
        if (c == 't') { if (!word("true"))  return fail("invalid value"); v->type = Value::kBool; v->boolean = true;  return true; }
        if (c == 'f') { if (!word("false")) return fail("invalid value"); v->type = Value::kBool; v->boolean = false; return true; }
        if (c == 'n') { if (!word("null"))  return fail("invalid value"); v->type = Value::kNull; return true; }
        if (c == '-' || digit(c)) { v->type = Value::kNumber; return number(&v->number); }
        if (c == '{' || c == '[') return fail("nested objects and arrays are not accepted");
        return fail("unexpected character where a value was expected");
    }

    // The JSON number grammar, checked by hand: strtod alone would also take
    // "0x1p3", "inf", "+5" and ".5", none of which are JSON.
    bool number(double* out) {
        size_t j = i_;
        const size_t n = s_.size();
        if (j < n && s_[j] == '-') ++j;
        if (j >= n) return fail("invalid number");
        if (s_[j] == '0') {
            ++j;
        } else if (s_[j] >= '1' && s_[j] <= '9') {
            while (j < n && digit(s_[j])) ++j;
        } else {
            return fail("invalid number");
        }
        if (j < n && s_[j] == '.') {
            ++j;
            if (j >= n || !digit(s_[j])) return fail("invalid number");
            while (j < n && digit(s_[j])) ++j;
        }
        if (j < n && (s_[j] == 'e' || s_[j] == 'E')) {
            ++j;
            if (j < n && (s_[j] == '+' || s_[j] == '-')) ++j;
            if (j >= n || !digit(s_[j])) return fail("invalid number");
            while (j < n && digit(s_[j])) ++j;
        }
        const std::string tok = s_.substr(i_, j - i_);
        const double d = std::strtod(tok.c_str(), nullptr);
        if (!std::isfinite(d)) return fail("number out of range");
        *out = d;
        i_ = j;
        return true;
    }

    bool hex4(unsigned long* out) {
        if (i_ + 4 > s_.size()) return false;
        unsigned long v = 0;
        for (int k = 0; k < 4; ++k) {
            const char c = s_[i_ + static_cast<size_t>(k)];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= static_cast<unsigned long>(c - '0');
            else if (c >= 'a' && c <= 'f') v |= static_cast<unsigned long>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= static_cast<unsigned long>(c - 'A' + 10);
            else return false;
        }
        i_ += 4;
        *out = v;
        return true;
    }

    bool string(std::string* out) {
        ++i_;   // the opening quote
        out->clear();
        for (;;) {
            if (i_ >= s_.size()) return fail("unterminated string");
            const unsigned char c = static_cast<unsigned char>(s_[i_++]);
            if (c == '"') return true;
            if (c < 0x20) return fail("control character inside a string");
            if (c != '\\') { *out += static_cast<char>(c); continue; }

            if (i_ >= s_.size()) return fail("unterminated string");
            const char e = s_[i_++];
            switch (e) {
                case '"':  *out += '"';  break;
                case '\\': *out += '\\'; break;
                case '/':  *out += '/';  break;
                case 'b':  *out += '\b'; break;
                case 'f':  *out += '\f'; break;
                case 'n':  *out += '\n'; break;
                case 'r':  *out += '\r'; break;
                case 't':  *out += '\t'; break;
                case 'u': {
                    unsigned long cp = 0;
                    if (!hex4(&cp)) return fail("invalid \\u escape");
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        unsigned long lo = 0;
                        if (!(i_ + 1 < s_.size() && s_[i_] == '\\' && s_[i_ + 1] == 'u'))
                            return fail("unpaired surrogate");
                        i_ += 2;
                        if (!hex4(&lo) || lo < 0xDC00 || lo > 0xDFFF)
                            return fail("unpaired surrogate");
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                        return fail("unpaired surrogate");
                    }
                    append_utf8(out, cp);
                    break;
                }
                default: return fail("invalid escape sequence");
            }
        }
    }

    const std::string& s_;
    std::string* why_;
    size_t i_ = 0;
};

}  // namespace detail

/**
 * Parses one flat JSON object into `out`. On failure returns false and, when
 * `why` is non-null, says what was wrong in words fit for an error response.
 */
inline bool parse_flat_object(const std::string& text, Object* out, std::string* why = nullptr) {
    out->clear();
    detail::Parser p(text, why);
    if (p.object(out)) return true;
    out->clear();
    return false;
}

/** True when the value is a number holding a whole number that fits in an int. */
inline bool as_int(const Value& v, int* out) {
    if (v.type != Value::kNumber) return false;
    if (v.number != std::floor(v.number)) return false;
    if (v.number < -2147483648.0 || v.number > 2147483647.0) return false;
    *out = static_cast<int>(v.number);
    return true;
}

}  // namespace json
