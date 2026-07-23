#include "encryptor.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>
#include <openssl/err.h>

#include <stdexcept>
#include <memory>

namespace encryptor {

namespace {

// ----------------------------------------------------------------------------
// Fixed salt for PBKDF2, as requested.
//
// SECURITY NOTE: a fixed/embedded salt in the binary reduces the
// protection that PBKDF2 offers against dictionary/rainbow table
// attacks across different instances using the same passphrase, since
// all of them will derive the same key. In a production system it is
// recommended to generate a random salt per credential and store it
// alongside the encrypted data. Implemented here as requested.
// ----------------------------------------------------------------------------
const std::vector<uint8_t> kFixedSalt = {
    0x8f, 0x3a, 0x2c, 0x91, 0x4e, 0x77, 0xd0, 0x1b,
    0x5c, 0xa6, 0x0f, 0x22, 0xbb, 0x64, 0x9d, 0xe3
};

/// Builds an error message including the OpenSSL detail.
[[noreturn]] void throw_openssl_error(const std::string& context) {
    unsigned long err_code = ERR_get_error();
    char err_buf[256] = {0};
    ERR_error_string_n(err_code, err_buf, sizeof(err_buf));
    throw std::runtime_error(context + ": " + err_buf);
}

/// RAII helper for EVP_CIPHER_CTX.
struct CipherCtxDeleter {
    void operator()(EVP_CIPHER_CTX* ctx) const {
        if (ctx != nullptr) {
            EVP_CIPHER_CTX_free(ctx);
        }
    }
};
using CipherCtxPtr = std::unique_ptr<EVP_CIPHER_CTX, CipherCtxDeleter>;

} // namespace

std::vector<uint8_t> derive_key(const std::string& passphrase) {
    if (passphrase.empty()) {
        throw std::runtime_error("derive_key: passphrase cannot be empty");
    }

    std::vector<uint8_t> key(kKeyLength);

    int result = PKCS5_PBKDF2_HMAC(
        passphrase.data(),
        static_cast<int>(passphrase.size()),
        kFixedSalt.data(),
        static_cast<int>(kFixedSalt.size()),
        kPbkdf2Iters,
        EVP_sha256(),
        static_cast<int>(key.size()),
        key.data()
    );

    if (result != 1) {
        throw_openssl_error("derive_key: PKCS5_PBKDF2_HMAC failed");
    }

    return key;
}

EncryptedData encrypt_text(const std::string& plaintext, const std::string& passphrase) {
    // 1. Derive AES-256 key from passphrase.
    std::vector<uint8_t> key = derive_key(passphrase);

    // 2. Generate a random 12-byte IV (recommended for GCM).
    std::vector<uint8_t> iv(kIvLength);
    if (RAND_bytes(iv.data(), static_cast<int>(iv.size())) != 1) {
        throw_openssl_error("encrypt_text: failed to generate random IV");
    }

    // 3. Create and configure the encryption context.
    CipherCtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        throw_openssl_error("encrypt_text: failed to create EVP_CIPHER_CTX");
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        throw_openssl_error("encrypt_text: failed to initialize AES-256-GCM");
    }

    // Set IV length (default 12 bytes, explicitly set).
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN,
                             static_cast<int>(iv.size()), nullptr) != 1) {
        throw_openssl_error("encrypt_text: failed to configure IV length");
    }

    // Assign key and IV to the context.
    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), iv.data()) != 1) {
        throw_openssl_error("encrypt_text: failed to assign key/IV");
    }

    // 4. Encrypt the plain text.
    std::vector<uint8_t> ciphertext(plaintext.size());
    int out_len = 0;

    const auto* input_ptr = reinterpret_cast<const unsigned char*>(plaintext.data());

    if (EVP_EncryptUpdate(ctx.get(), ciphertext.data(), &out_len,
                           input_ptr, static_cast<int>(plaintext.size())) != 1) {
        throw_openssl_error("encrypt_text: EVP_EncryptUpdate failed");
    }

    int total_len = out_len;

    // 5. Finalize encryption (GCM adds no padding, but must be called anyway).
    int final_len = 0;
    if (EVP_EncryptFinal_ex(ctx.get(), ciphertext.data() + total_len, &final_len) != 1) {
        throw_openssl_error("encrypt_text: EVP_EncryptFinal_ex failed");
    }
    total_len += final_len;
    ciphertext.resize(total_len);

    // 6. Obtain the GCM auth tag.
    std::vector<uint8_t> tag(kTagLength);
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG,
                             static_cast<int>(tag.size()), tag.data()) != 1) {
        throw_openssl_error("encrypt_text: failed to obtain auth tag");
    }

    // 7. Package and return the result.
    EncryptedData result;
    result.ciphertext = std::move(ciphertext);
    result.iv = std::move(iv);
    result.tag = std::move(tag);

    return result;
}

} // namespace encryptor
