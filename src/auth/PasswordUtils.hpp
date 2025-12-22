#pragma once
#include <string>
#include <vector>

namespace auth {

struct PBKDF2Result {
    std::vector<unsigned char> hash;
    std::vector<unsigned char> salt;
    int iterations;
};

bool validatePasswordRules(const std::string &password, std::string &errMsg);
std::vector<unsigned char> generateSalt(size_t len = 16);
std::vector<unsigned char> pbkdf2_hmac_sha256(const std::string &password,
                                              const std::vector<unsigned char> &salt,
                                              int iterations,
                                              size_t dkLen);
PBKDF2Result createPasswordHash(const std::string &password, int iterations = 100000);
bool verifyPassword(const std::string &password,
                    const std::vector<unsigned char> &salt,
                    const std::vector<unsigned char> &hash,
                    int iterations);
std::string toHex(const std::vector<unsigned char> &v);
std::vector<unsigned char> fromHex(const std::string &hex);

} // namespace auth
