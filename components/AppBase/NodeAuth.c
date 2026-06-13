#include "NodeAuth.h"

/* Internal state for the pre-shared Core Key */
static uint32_t g_core_key = 0;

/*
 * Internal helper to generate a 64-bit hash from the Core Key and a 64-bit Challenge.
 * NOTE: For production, replace this with a secure MAC (Message Authentication Code) like SipHash.
 */
static uint64_t Auth_CalculateMAC(uint32_t core_key, uint64_t challenge) {
    uint64_t mac;
    uint64_t key_64;
    
    key_64 = ((uint64_t)core_key << 32) | core_key;
    mac = challenge ^ key_64;
    mac = (mac << 13) | (mac >> 51); /* Circular shift left */
    mac *= 0x9E3779B97F4A7C15ULL;    /* Multiply by 64-bit golden ratio prime */
    mac ^= (mac >> 27);
    
    /* Return the computed 64-bit MAC */
    return mac;
}

void Auth_SetCoreKey(uint32_t key) {
    g_core_key = key;
}

DefaultRet_t Auth_GenerateChallenge(uint32_t target_id, uint32_t timestamp, uint64_t* out_challenge) {
    /* Control flow: validate output pointer */
    if (out_challenge == NULL) {
        /* Return error on null pointer */
        return STAT_ERR_NULL;
    }

    /* * Combine the target ID (32-bit) and the timestamp (32-bit) into a 64-bit challenge.
     * The timestamp acts as the 'Nonce' ensuring freshness.
     */
    *out_challenge = ((uint64_t)timestamp << 32) | (uint64_t)target_id;

    /* Return success */
    return STAT_OKE;
}

DefaultRet_t Auth_SignChallenge(uint64_t challenge, uint64_t* out_signature) {
    /* Control flow: validate output pointer */
    if (out_signature == NULL) {
        /* Return error on null pointer */
        return STAT_ERR_NULL;
    }

    /* Control flow: verify core key is initialized */
    if (g_core_key == 0) {
        /* Return invalid state error */
        return STAT_ERR_INVALID_STATE;
    }

    *out_signature = Auth_CalculateMAC(g_core_key, challenge);

    /* Return success */
    return STAT_OKE;
}

DefaultRet_t Auth_VerifySignature(uint64_t original_challenge, uint64_t received_signature) {
    uint64_t expected_signature;

    /* Control flow: verify core key is initialized */
    if (g_core_key == 0) {
        /* Return invalid state error */
        return STAT_ERR_INVALID_STATE;
    }

    expected_signature = Auth_CalculateMAC(g_core_key, original_challenge);

    /* Control flow: compare expected signature with the received one */
    if (expected_signature != received_signature) {
        /* Return permission error if mismatch (imposter detected) */
        return STAT_ERR_PERMISSION;
    }

    /* Return success */
    return STAT_OKE;
}