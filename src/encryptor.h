#ifndef ENCRYPTOR_H
#define ENCRYPTOR_H

#include <string>
#include <vector>
#include <cstdint>

// ============================================================================
// AES-256-GCM encryption module
// ----------------------------------------------------------------------------
// Derives a 256-bit key from a passphrase using PBKDF2-HMAC-SHA256
// and encrypts arbitrary text strings using AES-256 in GCM mode
// (authenticated encryption).
// ============================================================================

/// Result of an encryption operation.
/// All fields are stored as raw bytes (not Base64/Hex),
/// ready to be serialized/persisted by the caller as needed.
struct EncryptedData {
    std::vector<uint8_t> ciphertext; // Encrypted text
    std::vector<uint8_t> iv;         // Initialization vector (12 bytes, GCM)
    std::vector<uint8_t> tag;        // GCM auth tag (16 bytes)
};

namespace encryptor {

// Fixed lengths used by the module (in bytes).
constexpr std::size_t kKeyLength   = 32; // AES-256 -> 32 bytes = 256 bits
constexpr std::size_t kIvLength    = 12; // Recommended IV size for GCM
constexpr std::size_t kTagLength   = 16; // Standard GCM auth tag size
constexpr int         kPbkdf2Iters = 10000;

/// Derives an AES-256 key from a passphrase using
/// PBKDF2-HMAC-SHA256 with a fixed salt (defined internally in the .cpp).
///
/// @param passphrase Input passphrase.
/// @return Derived key of kKeyLength bytes.
/// @throws std::runtime_error if derivation fails.
std::vector<uint8_t> derive_key(const std::string& passphrase);

/// Encrypts an arbitrary text string using AES-256-GCM.
///
/// @param plaintext  Plain text to encrypt.
/// @param passphrase Passphrase used to derive the key.
/// @return EncryptedData struct with ciphertext, IV, and auth tag.
/// @throws std::runtime_error if any OpenSSL error occurs
///         (IV generation, key derivation, or encryption).
EncryptedData encrypt_text(const std::string& plaintext, const std::string& passphrase);

} // namespace encryptor

#endif // ENCRYPTOR_H
