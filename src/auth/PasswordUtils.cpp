#include "PasswordUtils.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cctype>

namespace auth {

static bool isAsciiPrintable(const std::string &s) {
    for (unsigned char c : s) {
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

bool validatePasswordRules(const std::string &password, std::string &error) {
    if (password.size() < 8) {
        error = "Пароль должен быть не короче 8 символов";
        return false;
    }
    if (!isAsciiPrintable(password)) {
        error = "Пароль должен состоять из печатных ASCII-символов";
        return false;
    }
    bool hasLower = false;
    bool hasUpper = false;
    bool hasDigit = false;
    for (unsigned char c : password) {
        if (std::islower(c)) hasLower = true;
        else if (std::isupper(c)) hasUpper = true;
        else if (std::isdigit(c)) hasDigit = true;
    }
    if (!hasLower || !hasUpper || !hasDigit) {
        error = "Пароль должен содержать строчные, прописные буквы и цифры";
        return false;
    }
    return true;
}

PBKDF2Result createPasswordHash(const std::string &password, int iterations) {
    PBKDF2Result res;
    res.salt.resize(16);
    if (RAND_bytes(res.salt.data(), static_cast<int>(res.salt.size())) != 1) {
        res.salt.clear();
        return res;
    }
    res.hash.resize(32);
    int ok = PKCS5_PBKDF2_HMAC(
        password.data(),
        static_cast<int>(password.size()),
        res.salt.data(),
        static_cast<int>(res.salt.size()),
        iterations,
        EVP_sha256(),
        static_cast<int>(res.hash.size()),
        res.hash.data()
    );
    if (ok != 1) {
        res.salt.clear();
        res.hash.clear();
    }
    return res;
}

bool verifyPassword(const std::string &password,
                    const std::vector<unsigned char> &salt,
                    const std::vector<unsigned char> &expectedHash,
                    int iterations) {
    if (salt.empty() || expectedHash.empty()) return false;
    std::vector<unsigned char> calc(expectedHash.size());
    int ok = PKCS5_PBKDF2_HMAC(
        password.data(),
        static_cast<int>(password.size()),
        salt.data(),
        static_cast<int>(salt.size()),
        iterations,
        EVP_sha256(),
        static_cast<int>(calc.size()),
        calc.data()
    );
    if (ok != 1) return false;
    if (calc.size() != expectedHash.size()) return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < calc.size(); ++i) {
        diff |= static_cast<unsigned char>(calc[i] ^ expectedHash[i]);
    }
    return diff == 0;
}

static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

std::string toHex(const std::vector<unsigned char> &data) {
    static const char *digits = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (unsigned char b : data) {
        out.push_back(digits[(b >> 4) & 0x0F]);
        out.push_back(digits[b & 0x0F]);
    }
    return out;
}

std::vector<unsigned char> fromHex(const std::string &hex) {
    std::vector<unsigned char> out;
    if (hex.size() % 2 != 0) return out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = hexValue(hex[i]);
        int lo = hexValue(hex[i + 1]);
        if (hi < 0 || lo < 0) {
            out.clear();
            return out;
        }
        unsigned char b = static_cast<unsigned char>((hi << 4) | lo);
        out.push_back(b);
    }
    return out;
}

}
