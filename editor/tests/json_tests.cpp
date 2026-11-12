#include "json.hpp"
#include <cstdlib>
#include <iostream>

void check(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    for (const auto* text : {"null", "true", "false", "0", "18446744073709551615", "-9223372036854775808",
                             "1.25", "1e3", "-0.5e-2", "\"\\u0000\\n\\t\\\\\\\"\"", "\"\\ud83d\\ude00\"", "{\"a\":[1,true,null,{}]}"}) {
        const auto value = lyko::Json::parse(text);
        check(lyko::Json::parse(value.dump()) == value, "JSON round trip failed");
    }
    for (const auto* text : {"", " ", "01", "1.", "1e", "1e+", "-", "-01", "NaN", "Infinity", "1e9999", "18446744073709551616",
                             "-9223372036854775809", "{\"a\":0,\"a\":1}", "[1,]", "{\"a\":1,}", "true false", "\"\\ud800\"", "\"\\udc00\"", "\"\\x\""}) {
        bool rejected = false;
        try { (void)lyko::Json::parse(text); } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "Malformed JSON accepted");
    }
    bool invalid_utf8 = false;
    try { (void)lyko::Json::parse(std::string("\"\xc0\xaf\"")); }
    catch (const std::invalid_argument&) { invalid_utf8 = true; }
    check(invalid_utf8, "Overlong UTF-8 accepted");
    std::cout << "Passed JSON number/Unicode/structure round trips and malformed input checks\n";
}
