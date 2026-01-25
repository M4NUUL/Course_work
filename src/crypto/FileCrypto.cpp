#include "FileCrypto.hpp"

#include <sodium.h>
#include <QFile>
#include <QByteArray>
#include <cstring>

namespace crypto {

std::vector<unsigned char> genRandomBytes(std::size_t len) {
    std::vector<unsigned char> out(len);
    if (len > 0) {
        randombytes_buf(out.data(), len);
    }
    return out;
}

static bool encryptBufferSecretbox(const std::vector<unsigned char> &key,
                                   const QByteArray &plain,
                                   QByteArray &outCipher,
                                   std::string &err)
{
    if (key.size() != crypto_secretbox_KEYBYTES) {
        err = "invalid key size for secretbox (must be 32 bytes)";
        return false;
    }

    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    randombytes_buf(nonce, sizeof(nonce));

    outCipher.resize(crypto_secretbox_NONCEBYTES + plain.size() + crypto_secretbox_MACBYTES);

    unsigned char *dst = reinterpret_cast<unsigned char*>(outCipher.data());
    std::memcpy(dst, nonce, crypto_secretbox_NONCEBYTES);

    unsigned char *c = dst + crypto_secretbox_NONCEBYTES;

    if (crypto_secretbox_easy(
            c,
            reinterpret_cast<const unsigned char*>(plain.constData()),
            static_cast<unsigned long long>(plain.size()),
            nonce,
            key.data()) != 0)
    {
        err = "crypto_secretbox_easy failed";
        return false;
    }

    return true;
}

static bool decryptBufferSecretbox(const std::vector<unsigned char> &key,
                                   const QByteArray &cipher,
                                   QByteArray &outPlain,
                                   std::string &err)
{
    if (key.size() != crypto_secretbox_KEYBYTES) {
        err = "invalid key size for secretbox (must be 32 bytes)";
        return false;
    }

    if (cipher.size() < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
        err = "cipher too short";
        return false;
    }

    const unsigned char *src = reinterpret_cast<const unsigned char*>(cipher.constData());
    const unsigned char *nonce = src;
    const unsigned char *c = src + crypto_secretbox_NONCEBYTES;
    std::size_t c_len = cipher.size() - crypto_secretbox_NONCEBYTES;

    if (c_len < crypto_secretbox_MACBYTES) {
        err = "cipher data too short";
        return false;
    }

    std::size_t p_len = c_len - crypto_secretbox_MACBYTES;
    outPlain.resize(static_cast<int>(p_len));

    if (crypto_secretbox_open_easy(
            reinterpret_cast<unsigned char*>(outPlain.data()),
            c,
            static_cast<unsigned long long>(c_len),
            nonce,
            key.data()) != 0)
    {
        err = "crypto_secretbox_open_easy failed (decryption/auth error)";
        return false;
    }

    return true;
}

bool aes256_cbc_encrypt(const std::vector<unsigned char> &key,
                        const std::vector<unsigned char> &iv,
                        const std::string &inPath,
                        const std::string &outPath,
                        std::string &err)
{
    (void)iv;

    if (sodium_init() < 0) {
        err = "sodium_init failed";
        return false;
    }

    QFile in(QString::fromStdString(inPath));
    if (!in.open(QIODevice::ReadOnly)) {
        err = "cannot open input file";
        return false;
    }
    QByteArray plain = in.readAll();
    in.close();

    QByteArray cipher;
    if (!encryptBufferSecretbox(key, plain, cipher, err)) {
        return false;
    }

    QFile out(QString::fromStdString(outPath));
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        err = "cannot open output file";
        return false;
    }
    if (out.write(cipher) != cipher.size()) {
        err = "failed to write all cipher bytes";
        return false;
    }
    out.close();
    return true;
}

bool aes256_cbc_decrypt(const std::vector<unsigned char> &key,
                        const std::vector<unsigned char> &iv,
                        const std::string &inPath,
                        const std::string &outPath,
                        std::string &err)
{
    (void)iv;

    if (sodium_init() < 0) {
        err = "sodium_init failed";
        return false;
    }

    QFile in(QString::fromStdString(inPath));
    if (!in.open(QIODevice::ReadOnly)) {
        err = "cannot open encrypted file";
        return false;
    }
    QByteArray cipher = in.readAll();
    in.close();

    QByteArray plain;
    if (!decryptBufferSecretbox(key, cipher, plain, err)) {
        return false;
    }

    QFile out(QString::fromStdString(outPath));
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        err = "cannot open output file";
        return false;
    }
    if (out.write(plain) != plain.size()) {
        err = "failed to write all plain bytes";
        return false;
    }
    out.close();
    return true;
}

}