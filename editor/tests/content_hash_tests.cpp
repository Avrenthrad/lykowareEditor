#include "content_hash.hpp"
#include <cstdlib>
#include <iostream>
void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}
int main() {
  check(lyko::content_sha256(std::string_view{}) == lyko::content_sha256(""),
        "default empty view");
  check(lyko::content_sha256("") ==
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "empty SHA-256 vector");
  check(lyko::content_sha256("abc") ==
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "abc vector");
  check(lyko::content_sha256(
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
        "multiblock vector");
  check(lyko::content_sha256(std::string(1000000, 'a')) ==
            "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0",
        "million-byte vector");
  std::string binary;
  for (unsigned i = 0; i < 256; ++i) {
    binary.push_back(static_cast<char>(i));
  }
  check(lyko::content_sha256(binary) ==
            "40aff2e9d2d8922e47afd4648e6967497158785fbd1da870e7110266bf944880",
        "all byte values");
  check(lyko::valid_sha256(lyko::content_sha256(binary)) &&
            !lyko::valid_sha256(std::string(64, 'G')) &&
            !lyko::valid_sha256("abc"),
        "strict lowercase digest syntax");
  std::cout << "Passed standard SHA-256 vectors and binary input\n";
}
