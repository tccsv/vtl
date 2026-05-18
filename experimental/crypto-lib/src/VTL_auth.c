#include <VTL_auth.h>
#include <ed25519.h>

void VTL_auth_client_respond(unsigned char *signature, 
                         const unsigned char *challenge, size_t challenge_len,
                         const VTL_auth_keypair_t *keypair) {
    ed25519_sign(signature, challenge, challenge_len, keypair->public_key, keypair->private_key);
}

int VTL_auth_server_verify(const unsigned char *signature,
                        const unsigned char *challenge, size_t challenge_len,
                        const unsigned char *public_key) {
    return ed25519_verify(signature, challenge, challenge_len, public_key);
}
