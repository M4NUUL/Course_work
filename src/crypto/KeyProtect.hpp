#pragma once
#include <vector>
#include <string>

namespace keyprotect {

bool encryptWithAesGcm(const std::vector<unsigned char>& master_key,
                       const std::vector<unsigned char>& plain,
                       std::vector<unsigned char>& out_cipher,
                       std::vector<unsigned char>& out_iv,
                       std::vector<unsigned char>& out_tag,
                       std::string &err);

bool decryptWithAesGcm(const std::vector<unsigned char>& master_key,
                       const std::vector<unsigned char>& cipher,
                       const std::vector<unsigned char>& iv,
                       const std::vector<unsigned char>& tag,
                       std::vector<unsigned char>& out_plain,
                       std::string &err);

} // namespace keyprotect
