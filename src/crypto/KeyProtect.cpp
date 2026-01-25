#include "KeyProtect.hpp"
#include <sodium.h>

namespace keyprotect {

bool encryptWithAesGcm(const std::vector<unsigned char> &masterKey,
                       const std::vector<unsigned char> &plaintextKey,
                       std::vector<unsigned char> &encKey,
                       std::vector<unsigned char> &iv,
                       std::vector<unsigned char> &tag,
                       std::string &err)
{
    if (sodium_init() < 0) {
        err = "sodium_init failed";
        return false;
    }

    if (masterKey.size() != crypto_secretbox_KEYBYTES) {
        err = "masterKey must be 32 bytes for secretbox";
        return false;
    }

    if (plaintextKey.empty()) {
        err = "plaintextKey is empty";
        return false;
    }

    iv.resize(crypto_secretbox_NONCEBYTES);
    randombytes_buf(iv.data(), iv.size());

    encKey.resize(plaintextKey.size() + crypto_secretbox_MACBYTES);

    if (crypto_secretbox_easy(
            encKey.data(),
            plaintextKey.data(),
            static_cast<unsigned long long>(plaintextKey.size()),
            iv.data(),
            masterKey.data()) != 0)
    {
        err = "crypto_secretbox_easy failed";
        return false;
    }

    tag.clear();
    return true;
}

bool decryptWithAesGcm(const std::vector<unsigned char> &masterKey,
                       const std::vector<unsigned char> &encKey,
                       const std::vector<unsigned char> &iv,
                       const std::vector<unsigned char> &tag,
                       std::vector<unsigned char> &plaintextKey,
                       std::string &err)
{
    (void)tag;

    if (sodium_init() < 0) {
        err = "sodium_init failed";
        return false;
    }

    if (masterKey.size() != crypto_secretbox_KEYBYTES) {
        err = "masterKey must be 32 bytes for secretbox";
        return false;
    }

    if (iv.size() != crypto_secretbox_NONCEBYTES) {
        err = "iv (nonce) size invalid for secretbox";
        return false;
    }

    if (encKey.size() < crypto_secretbox_MACBYTES) {
        err = "encKey too short";
        return false;
    }

    std::size_t p_len = encKey.size() - crypto_secretbox_MACBYTES;
    plaintextKey.resize(p_len);

    if (crypto_secretbox_open_easy(
            plaintextKey.data(),
            encKey.data(),
            static_cast<unsigned long long>(encKey.size()),
            iv.data(),
            masterKey.data()) != 0)
    {
        err = "crypto_secretbox_open_easy failed (decryption/auth error)";
        return false;
    }

    return true;
}

}
