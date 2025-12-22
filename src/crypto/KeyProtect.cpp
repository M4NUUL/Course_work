#include "KeyProtect.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <vector>

namespace keyprotect {

static std::vector<unsigned char> randomBytes(size_t n) {
    std::vector<unsigned char> v(n);
    RAND_bytes(v.data(), (int)n);
    return v;
}

bool encryptWithAesGcm(const std::vector<unsigned char>& master_key,
                       const std::vector<unsigned char>& plain,
                       std::vector<unsigned char>& out_cipher,
                       std::vector<unsigned char>& out_iv,
                       std::vector<unsigned char>& out_tag,
                       std::string &err) {
    if(master_key.size() != 32) { err="master_key must be 32 bytes"; return false; }
    out_iv = randomBytes(12);
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { err="EVP_CIPHER_CTX_new failed"; return false; }
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        err="EncryptInit failed"; EVP_CIPHER_CTX_free(ctx); return false;
    }
    if (1 != EVP_EncryptInit_ex(ctx, NULL, NULL, master_key.data(), out_iv.data())) {
        err="EncryptInit key/iv failed"; EVP_CIPHER_CTX_free(ctx); return false;
    }
    int outlen = 0;
    out_cipher.resize(plain.size());
    if (1 != EVP_EncryptUpdate(ctx, out_cipher.data(), &outlen, plain.data(), (int)plain.size())) {
        err="EncryptUpdate failed"; EVP_CIPHER_CTX_free(ctx); return false;
    }
    int outlen2 = 0;
    if (1 != EVP_EncryptFinal_ex(ctx, nullptr, &outlen2)) {
        // ok for GCM
    }
    out_tag.resize(16);
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, out_tag.data())) {
        err="Get tag failed"; EVP_CIPHER_CTX_free(ctx); return false;
    }
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool decryptWithAesGcm(const std::vector<unsigned char>& master_key,
                       const std::vector<unsigned char>& cipher,
                       const std::vector<unsigned char>& iv,
                       const std::vector<unsigned char>& tag,
                       std::vector<unsigned char>& out_plain,
                       std::string &err) {
    if(master_key.size() != 32) { err="master_key must be 32 bytes"; return false; }
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { err="EVP_CIPHER_CTX_new failed"; return false; }
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        err="DecryptInit failed"; EVP_CIPHER_CTX_free(ctx); return false;
    }
    if (1 != EVP_DecryptInit_ex(ctx, NULL, NULL, master_key.data(), iv.data())) {
        err="DecryptInit key/iv failed"; EVP_CIPHER_CTX_free(ctx); return false;
    }
    out_plain.resize(cipher.size());
    int outlen=0;
    if (1 != EVP_DecryptUpdate(ctx, out_plain.data(), &outlen, cipher.data(), (int)cipher.size())) {
        err="DecryptUpdate failed"; EVP_CIPHER_CTX_free(ctx); return false;
    }
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, (int)tag.size(), (void*)tag.data())) {
        err="Set TAG failed"; EVP_CIPHER_CTX_free(ctx); return false;
    }
    int ret = EVP_DecryptFinal_ex(ctx, nullptr, &outlen);
    EVP_CIPHER_CTX_free(ctx);
    if (ret <= 0) { err="DecryptFinal failed - tag mismatch"; return false; }
    return true;
}

} // namespace keyprotect
