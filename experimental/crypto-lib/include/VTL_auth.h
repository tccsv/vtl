#ifndef LIBAUTH_H
#define LIBAUTH_H

#include <stddef.h>

#define VTL_AUTH_SUCCESS 0
#define VTL_AUTH_ERROR -1

#define VTL_AUTH_PUBLIC_KEY_SIZE 32
#define VTL_AUTH_PRIVATE_KEY_SIZE 64
#define VTL_AUTH_SIGNATURE_SIZE 64
#define VTL_AUTH_SEED_SIZE 32

typedef struct {
    unsigned char public_key[VTL_AUTH_PUBLIC_KEY_SIZE];
    unsigned char private_key[VTL_AUTH_PRIVATE_KEY_SIZE];
} VTL_auth_keypair_t;

// Создать пару ключей (открытый + закрытый)
int VTL_auth_generate_keypair(VTL_auth_keypair_t *keypair);

// Подписать данные приватным ключом
void VTL_auth_client_respond(unsigned char *signature, 
                         const unsigned char *challenge, size_t challenge_len,
                         const VTL_auth_keypair_t *keypair);

// Проверить подписанные данные открытым ключом
int VTL_auth_server_verify(const unsigned char *signature,
                        const unsigned char *challenge, size_t challenge_len,
                        const unsigned char *public_key);

// Загрузить из файла/сохранить в файл ключи
int VTL_auth_save_to_file(const char *filename, const unsigned char *data, size_t len);

int VTL_auth_load_from_file(const char *filename, unsigned char *data, size_t len);

#endif
