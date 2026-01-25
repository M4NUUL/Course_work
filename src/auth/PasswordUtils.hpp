#pragma once
#include <string>
#include <vector>

namespace auth {

struct PBKDF2Result {
    std::vector<unsigned char> hash;
    std::vector<unsigned char> salt;
};

bool validatePasswordRules(const std::string &password, std::string &error);

PBKDF2Result createPasswordHash(const std::string &password, int iterations);

bool verifyPassword(const std::string &password,
                    const std::vector<unsigned char> &salt,
                    const std::vector<unsigned char> &expectedHash,
                    int iterations);

std::string toHex(const std::vector<unsigned char> &data);

std::vector<unsigned char> fromHex(const std::string &hex);

}
