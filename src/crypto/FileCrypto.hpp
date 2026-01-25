#pragma once
#include <string>
#include <vector>

namespace crypto {

/// Генерация случайных байт (libsodium randombytes_buf)
std::vector<unsigned char> genRandomBytes(std::size_t len);

/// Шифрование файла
bool aes256_cbc_encrypt(const std::vector<unsigned char> &key,
                        const std::vector<unsigned char> &iv,
                        const std::string &inPath,
                        const std::string &outPath,
                        std::string &err);

/// Расшифровка файла
bool aes256_cbc_decrypt(const std::vector<unsigned char> &key,
                        const std::vector<unsigned char> &iv,
                        const std::string &inPath,
                        const std::string &outPath,
                        std::string &err);

}
