#include "PasswordUtils.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <sstream>
#include <iomanip>
#include <regex>

namespace auth {

bool validatePasswordRules(const std::string &password, std::string &errMsg) {
    if (password.size() < 10) { errMsg = "Пароль должен быть не меньше 10 символов"; return false; }
    if (password.find(' ') != std::string::npos) { errMsg = "Пароль не должен содержать пробелов"; return false; }
    std::regex lower("[a-z]");
    std::regex upper("[A-Z]");
    std::regex digit("\\d");
    std::regex special("[!@#\\$%\\^&\\*\\(\\)\\-_=\\+\\[\\]\\{\\}\\|;:'\",.<>\\/?]");
    if (!std::regex_search(password, lower)) { errMsg = "Пароль должен содержать строчную букву"; return false; }
    if (!std::regex_search(password, upper)) { errMsg = "Пароль должен содержать заглавную букву"; return false; }
    if (!std::regex_search(password, digit)) { errMsg = "Пароль должен содержать цифру"; return false; }
    if (!std::regex_search(password, special)) { errMsg = "Пароль должен содержать специальный символ"; return false; }
    return true;
}

std::vector<unsigned char> generateSalt(size_t len) {
    std::vector<unsigned char> salt(len);
    if (1 != RAND_bytes(salt.data(), (int)len)) {
        for (size_t i = 0; i < len; ++i) salt[i] = static_cast<unsigned char>(i & 0xFF);
    }
    return salt;
}

std::vector<unsigned char> pbkdf2_hmac_sha256(const std::string &password,
                                              const std::vector<unsigned char> &salt,
                                              int iterations,
                                              size_t dkLen) {
    std::vector<unsigned char> out(dkLen);
    PKCS5_PBKDF2_HMAC(password.c_str(), (int)password.size(),
                      salt.data(), (int)salt.size(),
                      iterations, EVP_sha256(), (int)dkLen, out.data());
    return out;
}

PBKDF2Result createPasswordHash(const std::string &password, int iterations) {
    PBKDF2Result res;
    res.salt = generateSalt(16);
    res.iterations = iterations;
    res.hash = pbkdf2_hmac_sha256(password, res.salt, iterations, 32);
    return res;
}

bool verifyPassword(const std::string &password,
                    const std::vector<unsigned char> &salt,
                    const std::vector<unsigned char> &hash,
                    int iterations) {
    auto newhash = pbkdf2_hmac_sha256(password, salt, iterations, hash.size());
    return CRYPTO_memcmp(hash.data(), newhash.data(), (size_t)hash.size()) == 0;
}

std::string toHex(const std::vector<unsigned char> &v) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto c : v) {
        oss << std::setw(2) << (int)c;
    }
    return oss.str();
}

std::vector<unsigned char> fromHex(const std::string &hex) {
    std::vector<unsigned char> out;
    out.reserve(hex.size()/2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        unsigned int byte = 0;
        std::istringstream iss(hex.substr(i,2));
        iss >> std::hex >> byte;
        out.push_back(static_cast<unsigned char>(byte));
    }
    return out;
}

} // namespace auth
