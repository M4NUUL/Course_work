// tools/pbkdf2_gen.cpp
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <string>

std::string toHex(const std::vector<unsigned char>& v) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : v) oss << std::setw(2) << (int)c;
    return oss.str();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: pbkdf2_gen <password> [iterations]\n";
        return 1;
    }
    std::string password = argv[1];
    int iterations = 100000;
    if (argc >= 3) iterations = std::atoi(argv[2]);

    std::vector<unsigned char> salt(16);
    if (1 != RAND_bytes(salt.data(), (int)salt.size())) {
        std::cerr << "RAND_bytes failed\n";
        return 1;
    }

    std::vector<unsigned char> hash(32);
    PKCS5_PBKDF2_HMAC(password.c_str(), (int)password.size(),
                      salt.data(), (int)salt.size(),
                      iterations, EVP_sha256(), (int)hash.size(), hash.data());

    std::cout << "salt=" << toHex(salt) << "\n";
    std::cout << "hash=" << toHex(hash) << "\n";
    std::cout << "iterations=" << iterations << "\n";
    return 0;
}
