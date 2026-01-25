#pragma once
#include <string>
#include <vector>

namespace keyprotect {

bool encryptWithAesGcm(const std::vector<unsigned char> &masterKey,
                       const std::vector<unsigned char> &plaintextKey,
                       std::vector<unsigned char> &encKey,
                       std::vector<unsigned char> &iv,
                       std::vector<unsigned char> &tag,
                       std::string &err);

bool decryptWithAesGcm(const std::vector<unsigned char> &masterKey,
                       const std::vector<unsigned char> &encKey,
                       const std::vector<unsigned char> &iv,
                       const std::vector<unsigned char> &tag,
                       std::vector<unsigned char> &plaintextKey,
                       std::string &err);

}
