// ---------------------------------------------------------------------------
// Tests for the JSON reader in server/include/json.hpp.
//
// The reader sits on the one place untrusted bytes enter the server, so it gets
// its own test: every malformed input here must be refused, never crash, and
// never be half-accepted.
//
//   make test        builds and runs this beside the engine tests
// ---------------------------------------------------------------------------
#include <cstdio>
#include <string>

#include "json.hpp"

static int failures = 0;

static void check(bool cond, const char* what) {
    std::printf("  [%s] %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) ++failures;
}

static bool parses(const std::string& text) {
    json::Object o;
    return json::parse_flat_object(text, &o);
}

int main() {
    std::printf("Test 1: a typical route request\n");
    {
        json::Object o;
        std::string why;
        const bool ok = json::parse_flat_object(
            R"( { "width": 100, "height":100, "cells":"0012", "start": 303,
                  "end":9696, "reps": 5, "flag": true, "nothing": null } )", &o, &why);
        check(ok, "parses");
        int v = 0;
        check(ok && json::as_int(o["width"], &v) && v == 100, "reads an integer");
        check(ok && o["cells"].type == json::Value::kString && o["cells"].text == "0012",
              "reads a string");
        check(ok && o["flag"].type == json::Value::kBool && o["flag"].boolean, "reads a boolean");
        check(ok && o["nothing"].type == json::Value::kNull, "reads null");
    }

    std::printf("\nTest 2: whole numbers only where an integer is wanted\n");
    {
        json::Object o;
        json::parse_flat_object(R"({"a":1.5,"b":1e2,"c":-7,"d":3000000000,"e":"5","f":0})", &o);
        int v = 0;
        check(!json::as_int(o["a"], &v), "1.5 is not a whole number");
        check(json::as_int(o["b"], &v) && v == 100, "1e2 is the whole number 100");
        check(json::as_int(o["c"], &v) && v == -7, "negative integers read back");
        check(!json::as_int(o["d"], &v), "a number above INT_MAX is refused");
        check(!json::as_int(o["e"], &v), "a string is not a number");
        check(json::as_int(o["f"], &v) && v == 0, "zero reads back");
    }

    std::printf("\nTest 3: malformed JSON is refused\n");
    {
        check(!parses(""), "empty body");
        check(!parses("   "), "whitespace only");
        check(!parses("[]"), "an array is not an object");
        check(!parses("42"), "a bare number");
        check(!parses(R"({"a":1)"), "missing closing brace");
        check(!parses(R"({"a":1,})"), "trailing comma");
        check(!parses(R"({,"a":1})"), "leading comma");
        check(!parses(R"({"a" 1})"), "missing colon");
        check(!parses(R"({a:1})"), "unquoted key");
        check(!parses(R"({"a":1} x)"), "text after the object");
        check(!parses(R"({"a":1}{"b":2})"), "two objects");
        check(!parses(R"({"a":"unterminated})"), "unterminated string");
        check(!parses(R"({"a":tru})"), "misspelt literal");
        check(!parses(R"({"a":1,"a":2})"), "duplicate key");
    }

    std::printf("\nTest 4: nesting is refused, not parsed\n");
    {
        check(!parses(R"({"a":{"b":1}})"), "a nested object");
        check(!parses(R"({"a":[1,2]})"), "a nested array");
        std::string deep = "{\"a\":";
        for (int i = 0; i < 100000; ++i) deep += '[';
        check(!parses(deep), "100,000 opening brackets do not overflow the stack");
    }

    std::printf("\nTest 5: numbers follow the JSON grammar\n");
    {
        check(!parses(R"({"a":01})"), "leading zero");
        check(!parses(R"({"a":+1})"), "leading plus");
        check(!parses(R"({"a":.5})"), "no digit before the point");
        check(!parses(R"({"a":1.})"), "no digit after the point");
        check(!parses(R"({"a":1e})"), "empty exponent");
        check(!parses(R"({"a":0x10})"), "hexadecimal");
        check(!parses(R"({"a":inf})"), "infinity");
        check(!parses(R"({"a":nan})"), "not-a-number");
        check(!parses(R"({"a":-})"), "a lone minus");
        check(!parses(R"({"a":1e999})"), "a number too large for a double");
        check(parses(R"({"a":-0.5e-3})"), "a valid exponent form is accepted");
    }

    std::printf("\nTest 6: string escapes\n");
    {
        json::Object o;
        const bool ok = json::parse_flat_object(
            R"({"a":"x\ny\t\"q\"\\ Aé😀"})", &o);
        check(ok, "valid escapes parse");
        check(ok && o["a"].text == "x\ny\t\"q\"\\ A\xC3\xA9\xF0\x9F\x98\x80",
              "they decode to the right UTF-8 bytes");
        check(!parses(R"({"a":"\q"})"), "an unknown escape");
        check(!parses(R"({"a":"\u12"})"), "a short \\u escape");
        check(!parses(R"({"a":"\ud800"})"), "an unpaired high surrogate");
        check(!parses(R"({"a":"\udc00"})"), "an unpaired low surrogate");
        check(!parses(std::string("{\"a\":\"line\nbreak\"}")), "a raw newline inside a string");
    }

    std::printf("\nTest 7: size\n");
    {
        const std::string big(70000, '0');
        json::Object o;
        const bool ok = json::parse_flat_object("{\"cells\":\"" + big + "\"}", &o);
        check(ok && o["cells"].text.size() == 70000, "a 70,000-character string parses");

        std::string many = "{";
        for (int i = 0; i < 40; ++i) {
            if (i) many += ',';
            many += "\"k" + std::to_string(i) + "\":1";
        }
        many += "}";
        check(!parses(many), "more than 32 fields are refused");
    }

    std::printf("\n%s (%d failure%s)\n",
                failures == 0 ? "ALL JSON TESTS PASSED" : "JSON TESTS FAILED",
                failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
