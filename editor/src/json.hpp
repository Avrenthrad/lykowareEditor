#pragma once
#include <charconv>
#include <cstdint>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace lyko {
// Bounded JSON parser. Signed/fractional values remain distinct and cannot
// silently coerce to unsigned entity IDs or revisions.
class Json {
public:
    using Object = std::map<std::string, Json>;
    using Array = std::vector<Json>;
    using Value = std::variant<std::nullptr_t, bool, std::uint64_t, std::int64_t, double, std::string, Object, Array>;
    Value value = nullptr;
    Json() = default;
    Json(std::nullptr_t) : value(nullptr) {}
    Json(bool v) : value(v) {}
    Json(std::uint64_t v) : value(v) {}
    Json(std::int64_t v) : value(v) {}
    Json(double v) : value(v) {}
    Json(std::string v) : value(std::move(v)) {}
    Json(const char* v) : value(std::string(v)) {}
    Json(Object v) : value(std::move(v)) {}
    Json(Array v) : value(std::move(v)) {}
    template<class T> const T& as() const {
        auto p = std::get_if<T>(&value);
        if (!p) { throw std::invalid_argument("Incorrect JSON value type"); }
        return *p;
    }
    const Json& at(const std::string& key) const {
        const auto& o = as<Object>(); auto it = o.find(key);
        if (it == o.end()) { throw std::invalid_argument("Missing field: " + key); }
        return it->second;
    }
    bool contains(const std::string& key) const { return as<Object>().contains(key); }
    bool operator==(const Json&) const = default;
    static Json parse(std::string_view text);
    std::string dump() const;
};
namespace json_detail {
inline void utf8(std::string& out, std::uint32_t cp) {
    if (cp <= 0x7f) { out += static_cast<char>(cp); }
    else if (cp <= 0x7ff) { out += static_cast<char>(0xc0 | (cp >> 6)); out += static_cast<char>(0x80 | (cp & 63)); }
    else {
        if (cp > 0xffff) { out += static_cast<char>(0xf0 | (cp >> 18)); }
        if (cp > 0xffff) { out += static_cast<char>(0x80 | ((cp >> 12) & 63)); }
        else { out += static_cast<char>(0xe0 | (cp >> 12)); }
        out += static_cast<char>(0x80 | ((cp >> 6) & 63)); out += static_cast<char>(0x80 | (cp & 63));
    }
}
inline bool valid_utf8(std::string_view s) {
    for (std::size_t i = 0; i < s.size();) {
        auto c = static_cast<unsigned char>(s[i++]);
        if (c < 128) { continue; }
        unsigned count; std::uint32_t cp, minimum;
        if (c >= 0xc2 && c <= 0xdf) { count = 1; cp = c & 31; minimum = 0x80; }
        else if (c >= 0xe0 && c <= 0xef) { count = 2; cp = c & 15; minimum = 0x800; }
        else if (c >= 0xf0 && c <= 0xf4) { count = 3; cp = c & 7; minimum = 0x10000; }
        else { return false; }
        while (count--) {
            if (i == s.size()) { return false; }
            auto b = static_cast<unsigned char>(s[i++]);
            if ((b & 0xc0) != 0x80) { return false; }
            cp = (cp << 6) | (b & 63);
        }
        if (cp < minimum || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) { return false; }
    }
    return true;
}
class Parser {
    std::string_view text_; std::size_t pos_ = 0;
    [[noreturn]] void fail() const { throw std::invalid_argument("Invalid JSON at byte " + std::to_string(pos_)); }
    char take() { if (pos_ == text_.size()) { fail(); } return text_[pos_++]; }
    void ws() { while (pos_ < text_.size() && (text_[pos_] == ' ' || text_[pos_] == '\r' || text_[pos_] == '\n' || text_[pos_] == '\t')) { ++pos_; } }
    bool accept(char c) { ws(); if (pos_ < text_.size() && text_[pos_] == c) { ++pos_; return true; } return false; }
    std::uint32_t hex4() {
        std::uint32_t cp = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = take(); cp *= 16;
            if (c >= '0' && c <= '9') { cp += c - '0'; }
            else if (c >= 'a' && c <= 'f') { cp += c - 'a' + 10; }
            else if (c >= 'A' && c <= 'F') { cp += c - 'A' + 10; }
            else { fail(); }
        }
        return cp;
    }
    std::string string() {
        if (take() != '"') { fail(); }
        std::string out;
        for (;;) {
            auto c = static_cast<unsigned char>(take());
            if (c == '"') { return out; }
            if (c < 32) { fail(); }
            if (c != '\\') { out += static_cast<char>(c); continue; }
            switch (take()) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            case 'u': {
                auto cp = hex4();
                if (cp >= 0xd800 && cp <= 0xdbff) {
                    if (take() != '\\' || take() != 'u') { fail(); }
                    auto low = hex4(); if (low < 0xdc00 || low > 0xdfff) { fail(); }
                    cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
                } else if (cp >= 0xdc00 && cp <= 0xdfff) { fail(); }
                utf8(out, cp); break;
            }
            default: fail();
            }
        }
    }
    Json read(unsigned depth) {
        if (depth > 64) { fail(); }
        ws(); if (pos_ == text_.size()) { fail(); }
        char c = text_[pos_];
        if (c == '"') { return Json(string()); }
        if (accept('{')) {
            Json::Object object;
            if (accept('}')) { return object; }
            do {
                ws(); auto key = string(); if (!accept(':')) { fail(); }
                if (!object.emplace(std::move(key), read(depth + 1)).second) { fail(); }
                if (accept('}')) { return object; }
            } while (accept(','));
            fail();
        }
        if (accept('[')) {
            Json::Array array;
            if (accept(']')) { return array; }
            do { array.push_back(read(depth + 1)); if (accept(']')) { return array; } } while (accept(','));
            fail();
        }
        for (auto token : {std::string_view("null"), std::string_view("true"), std::string_view("false")}) {
            if (text_.substr(pos_, token.size()) == token) {
                pos_ += token.size(); if (token == "null") { return nullptr; } return token == "true";
            }
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            const auto start = pos_;
            if (c == '-') { ++pos_; }
            if (pos_ == text_.size() || text_[pos_] < '0' || text_[pos_] > '9') { fail(); }
            if (text_[pos_++] != '0') {
                while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') { ++pos_; }
            }
            bool floating = false;
            if (pos_ < text_.size() && text_[pos_] == '.') {
                floating = true; ++pos_; const auto digits = pos_;
                while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') { ++pos_; }
                if (digits == pos_) { fail(); }
            }
            if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
                floating = true; ++pos_;
                if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) { ++pos_; }
                const auto digits = pos_;
                while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') { ++pos_; }
                if (digits == pos_) { fail(); }
            }
            const auto first = text_.data() + start; const auto last = text_.data() + pos_;
            if (floating) {
                double number{}; auto [end, ec] = std::from_chars(first, last, number);
                if (ec != std::errc{} || end != last || !std::isfinite(number)) { fail(); }
                return number;
            }
            if (c == '-') {
                std::int64_t number{}; auto [end, ec] = std::from_chars(first, last, number);
                if (ec != std::errc{} || end != last) { fail(); } return number;
            }
            std::uint64_t number{}; auto [end, ec] = std::from_chars(first, last, number);
            if (ec != std::errc{} || end != last) { fail(); } return number;
        }
        fail();
    }
public:
    explicit Parser(std::string_view text) : text_(text) {}
    Json parse() { auto result = read(0); ws(); if (pos_ != text_.size()) { fail(); } return result; }
};
inline std::string quote(const std::string& text) {
    if (!valid_utf8(text)) { throw std::invalid_argument("Invalid UTF-8"); }
    std::string out = "\"";
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') { out += '\\'; out += static_cast<char>(c); }
        else if (c < 32) { out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15]; }
        else { out += static_cast<char>(c); }
    }
    return out + '"';
}
}
inline Json Json::parse(std::string_view text) {
    if (text.size() > 1024 * 1024 || !json_detail::valid_utf8(text)) { throw std::invalid_argument("JSON exceeds 1 MiB or contains invalid UTF-8"); }
    return json_detail::Parser(text).parse();
}
inline std::string Json::dump() const {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) { return "null"; }
        else if constexpr (std::is_same_v<T, bool>) { return v ? "true" : "false"; }
        else if constexpr (std::is_same_v<T, std::uint64_t> || std::is_same_v<T, std::int64_t>) { return std::to_string(v); }
        else if constexpr (std::is_same_v<T, double>) {
            if (!std::isfinite(v)) { throw std::invalid_argument("Nonfinite JSON number"); }
            char buffer[64]; auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), v);
            if (error != std::errc{}) { throw std::invalid_argument("Cannot encode number"); }
            std::string out(buffer, end);
            if (out.find_first_of(".eE") == std::string::npos) { out += ".0"; }
            return out;
        }
        else if constexpr (std::is_same_v<T, std::string>) { return json_detail::quote(v); }
        else {
            std::string out;
            if constexpr (std::is_same_v<T, Object>) {
                out = "{"; for (const auto& [key, item] : v) { if (out.size() > 1) { out += ','; } out += json_detail::quote(key) + ':' + item.dump(); } return out + '}';
            } else {
                out = "["; for (const auto& item : v) { if (out.size() > 1) { out += ','; } out += item.dump(); } return out + ']';
            }
        }
    }, value);
}
}
