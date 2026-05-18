#include <VTL_auth.h
#include <ed25519.h>
#include <stdio.h>

int VTL_auth_generate_keypair(VTL_auth_keypair_t *keypair) {
    unsigned char seed[VTL_AUTH_SEED_SIZE];
    if (ed25519_create_seed(seed)) {
        return VTL_AUTH_ERROR;
    }
    ed25519_create_keypair(keypair->public_key, keypair->private_key, seed);
    return VTL_AUTH_SUCCESS;
}

int VTL_auth_save_to_file(const char *filename, const unsigned char *data, size_t len) {
    FILE *f = fopen(filename, "wb");
    if (!f) return VTL_AUTH_ERROR;
    
    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    
    return (written == len) ? VTL_AUTH_SUCCESS : VTL_AUTH_ERROR;
}

int VTL_auth_load_from_file(const char *filename, unsigned char *data, size_t len) {
    FILE *f = fopen(filename, "rb");
    if (!f) return VTL_AUTH_ERROR;
    
    size_t read = fread(data, 1, len, f);
    fclose(f);
    
    return (read == len) ? VTL_AUTH_SUCCESS : VTL_AUTH_ERROR;
}
