#pragma once
#include <string>
#include <vector>

namespace crypto {

std::vector<unsigned char> genRandomBytes(size_t n);

bool aes256_cbc_encrypt(const std::vector<unsigned char>& key,
                        const std::vector<unsigned char>& iv,
                        const std::string& inFile,
                        const std::string& outFile,
                        std::string &err);

bool aes256_cbc_decrypt(const std::vector<unsigned char>& key,
                        const std::vector<unsigned char>& iv,
                        const std::string& inFile,
                        const std::string& outFile,
                        std::string &err);

} // namespace crypto
