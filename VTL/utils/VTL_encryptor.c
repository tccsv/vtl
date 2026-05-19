#include <VTL/utils/VTL_encryptor.h>
#include <string.h>

// Ключ шифрования (в реальности хранить безопасно)
static const char ENCRYPTION_KEY[] = "VTL_SECRET_KEY_2024";

static void xor_transform(const char* src, char* dest, size_t len) {
    size_t key_len = strlen(ENCRYPTION_KEY);
    for (size_t i = 0; i < len; i++) {
        dest[i] = src[i] ^ ENCRYPTION_KEY[i % key_len];
    }
}

void VTL_Encrypt(const VTL_StandartString src, VTL_EncryptedString dest) {
    size_t len = strlen(src);
    xor_transform(src, dest, len);
    dest[len] = '\0';
}

void VTL_Decrypt(const VTL_EncryptedString src, VTL_StandartString dest) {
    // XOR шифрование симметричное: повторное применение = расшифровка
    size_t len = strlen(src);
    xor_transform(src, dest, len);
    dest[len] = '\0';
}