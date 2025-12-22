#include "FileCrypto.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <fstream>
#include <vector>

namespace crypto {

std::vector<unsigned char> genRandomBytes(size_t n) {
    std::vector<unsigned char> v(n);
    RAND_bytes(v.data(), (int)n);
    return v;
}

bool aes256_cbc_encrypt(const std::vector<unsigned char>& key,
                        const std::vector<unsigned char>& iv,
                        const std::string& inFile,
                        const std::string& outFile,
                        std::string &err) {
    std::ifstream ifs(inFile, std::ios::binary);
    if (!ifs) { err = "Cannot open input file"; return false; }
    std::ofstream ofs(outFile, std::ios::binary);
    if (!ofs) { err = "Cannot open output file"; return false; }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { err = "EVP_CIPHER_CTX_new failed"; return false; }

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data())) {
        err = "EncryptInit failed"; EVP_CIPHER_CTX_free(ctx); return false;
    }

    const size_t BUFSIZE = 4096;
    std::vector<unsigned char> inbuf(BUFSIZE);
    std::vector<unsigned char> outbuf(BUFSIZE + EVP_CIPHER_block_size(EVP_aes_256_cbc()));

    int outlen;
    while (ifs.good()) {
        ifs.read((char*)inbuf.data(), BUFSIZE);
        std::streamsize readBytes = ifs.gcount();
        if (readBytes > 0) {
            if (1 != EVP_EncryptUpdate(ctx, outbuf.data(), &outlen, inbuf.data(), (int)readBytes)) {
                err = "EncryptUpdate failed"; EVP_CIPHER_CTX_free(ctx); return false;
            }
            ofs.write((char*)outbuf.data(), outlen);
        }
    }

    if (1 != EVP_EncryptFinal_ex(ctx, outbuf.data(), &outlen)) {
        err = "EncryptFinal failed"; EVP_CIPHER_CTX_free(ctx); return false;
    }
    ofs.write((char*)outbuf.data(), outlen);
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool aes256_cbc_decrypt(const std::vector<unsigned char>& key,
                        const std::vector<unsigned char>& iv,
                        const std::string& inFile,
                        const std::string& outFile,
                        std::string &err) {
    std::ifstream ifs(inFile, std::ios::binary);
    if (!ifs) { err = "Cannot open input file"; return false; }
    std::ofstream ofs(outFile, std::ios::binary);
    if (!ofs) { err = "Cannot open output file"; return false; }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { err = "EVP_CIPHER_CTX_new failed"; return false; }
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data())) {
        err = "DecryptInit failed"; EVP_CIPHER_CTX_free(ctx); return false;
    }

    const size_t BUFSIZE = 4096;
    std::vector<unsigned char> inbuf(BUFSIZE);
    std::vector<unsigned char> outbuf(BUFSIZE + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    int outlen;
    while (ifs.good()) {
        ifs.read((char*)inbuf.data(), BUFSIZE);
        std::streamsize readBytes = ifs.gcount();
        if (readBytes > 0) {
            if (1 != EVP_DecryptUpdate(ctx, outbuf.data(), &outlen, inbuf.data(), (int)readBytes)) {
                err = "DecryptUpdate failed"; EVP_CIPHER_CTX_free(ctx); return false;
            }
            ofs.write((char*)outbuf.data(), outlen);
        }
    }
    if (1 != EVP_DecryptFinal_ex(ctx, outbuf.data(), &outlen)) {
        err = "DecryptFinal failed (maybe wrong key)"; EVP_CIPHER_CTX_free(ctx); return false;
    }
    ofs.write((char*)outbuf.data(), outlen);
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

} // namespace crypto
