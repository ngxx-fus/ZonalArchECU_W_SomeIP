#include "./All.h"

void SafeFlagSet(SafeFlag_t *f, uint32_t i) {
    atomic_fetch_or(f, Mask32(i));
}

void SafeFlagClear(SafeFlag_t *f, uint32_t i) {
    atomic_fetch_and(f, InvMask32(i));
}

void SafeFlagToggle(SafeFlag_t *f, uint32_t i) {
    atomic_fetch_xor(f, Mask32(i));
}

DefaultRet_t SafeFlagHas(SafeFlag_t *f, uint32_t i) {
    // atomic_load ensures we read the value atomically
    uint32_t val = atomic_load(f);
    return (val & Mask32(i)) ? 1 : 0;
}

uint32_t GenerateRandomNumber(uint32_t SeedInput) {
    // --- 1. Persistent state ---
    static uint32_t last_known_random_state = 104729u; // initialized with a prime
    static int last_known_seed_input = 0;

    uint32_t x;

    // --- 2. Select initial seed ---
    if (last_known_seed_input == SeedInput) {
        x = last_known_random_state;
    } else {
        // start from new seed, add prime noise
        x = (uint32_t)SeedInput ^ 0xA5A5A5A5u;
    }
    last_known_seed_input = SeedInput;

    // --- 3. Prime constants for nonlinear mixing ---
    const uint32_t P1 = 7919u;
    const uint32_t P2 = 104729u;
    const uint32_t P3 = 65537u;
    const uint32_t P4 = 439u;
    const uint32_t P5 = 1223u;

    // --- 4. Chaotic transformations ---
    x = (x * P1 + P3) ^ (x >> 11);
    x = (x << 7) - (x >> 5) + P4;
    x ^= (x * P2);
    x = (x ^ (x >> 17)) + (x << 13);

    // --- 5. Differential and derivative noise ---
    uint32_t dx = x - ((x >> 1) | (x << 31));
    uint32_t ddx = dx ^ (dx >> 3) ^ (dx << 5);

    // --- 6. Amplify chaos using primes ---
    x ^= (ddx * P5);
    x ^= (dx << 9) | (dx >> 23);

    // --- 7. Update persistent state ---
    last_known_random_state = x ^ (x >> 16) ^ (x << 7) ^ P2;

    // --- 8. Return full 32-bit random number ---
    return last_known_random_state;
}

#if (ESP_ERR_CONVERT_EN == 1)
    DefaultRet_t ESPReturnType2DefaultReturnType(int espErr) {
        switch (espErr) {
            case ESP_OK:                        return STAT_OKE;
            case ESP_FAIL:                      return STAT_ERR;
            case ESP_ERR_NO_MEM:                return STAT_ERR_MALLOC_FAILED;
            case ESP_ERR_INVALID_ARG:           return STAT_ERR_INVALID_ARG;
            case ESP_ERR_INVALID_STATE:         return STAT_ERR_INVALID_STATE; // Mapped to explicit state err
            case ESP_ERR_INVALID_SIZE:          return STAT_ERR_INVALID_SIZE;
            case ESP_ERR_NOT_FOUND:             return STAT_ERR_NOT_FOUND;
            case ESP_ERR_NOT_SUPPORTED:         return STAT_ERR_UNSUPPORTED;
            case ESP_ERR_TIMEOUT:               return STAT_ERR_TIMEOUT;
            case ESP_ERR_INVALID_RESPONSE:      return STAT_ERR_IO;
            case ESP_ERR_INVALID_CRC:           return STAT_ERR_CRC;
            case ESP_ERR_INVALID_VERSION:       return STAT_ERR_INVALID_ARG;
            case ESP_ERR_INVALID_MAC:           return STAT_ERR_INVALID_ARG;
            case ESP_ERR_NOT_FINISHED:          return STAT_ERR_BUSY;
            default:
                // Ranges based on esp_err.h
                if (espErr >= 0x3000 && espErr < 0x4000) return STAT_ERR_IO;          // WiFi
                if (espErr >= 0x4000 && espErr < 0x6000) return STAT_ERR_UNSUPPORTED; // Mesh
                if (espErr >= 0x6000 && espErr < 0xc000) return STAT_ERR_IO;          // Flash
                if (espErr >= 0xc000 && espErr < 0xd000) return STAT_ERR_UNSUPPORTED; // Crypto
                if (espErr >= 0xd000)                    return STAT_ERR_PERMISSION;  // MemProt
                
                return STAT_ERR; // Unknown error
        }
    }
#endif /// (ESP_ERR_CONVERT_EN == 1)

const char* DefaultReturnType2Str(DefaultRet_t ret) {
    switch (ret) {
        case STAT_OKE:                  return STR_STAT_OKE;
        case STAT_ERR:                  return STR_STAT_ERR;
        case STAT_ERR_NULL:             return STR_STAT_ERR_NULL;
        case STAT_ERR_MALLOC_FAILED:    return STR_STAT_ERR_MALLOC_FAILED;
        case STAT_ERR_TIMEOUT:          return STR_STAT_ERR_TIMEOUT;
        case STAT_ERR_BUSY:             return STR_STAT_ERR_BUSY;
        case STAT_ERR_INVALID_ARG:      return STR_STAT_ERR_INVALID_ARG;
        case STAT_ERR_INVALID_STATE:    return STR_STAT_ERR_INVALID_STATE;
        case STAT_ERR_INVALID_SIZE:     return STR_STAT_ERR_INVALID_SIZE;
        case STAT_ERR_OVERFLOW:         return STR_STAT_ERR_OVERFLOW;
        case STAT_ERR_UNDERFLOW:        return STR_STAT_ERR_UNDERFLOW;
        case STAT_ERR_NOT_FOUND:        return STR_STAT_ERR_NOT_FOUND;
        case STAT_ERR_ALREADY_EXISTS:   return STR_STAT_ERR_ALREADY_EXISTS;
        case STAT_ERR_NOT_IMPLEMENTED:  return STR_STAT_ERR_NOT_IMPLEMENTED;
        case STAT_ERR_UNSUPPORTED:      return STR_STAT_ERR_UNSUPPORTED;
        case STAT_ERR_IO:               return STR_STAT_ERR_IO;
        case STAT_ERR_PERMISSION:       return STR_STAT_ERR_PERMISSION;
        case STAT_ERR_CRC:              return STR_STAT_ERR_CRC;
        case STAT_ERR_INIT_FAILED:      return STR_STAT_ERR_INIT_FAILED;
        case STAT_ERR_PSRAM_FAILED:     return STR_STAT_ERR_PSRAM_FAILED;
        default:                        return STR_STAT_UNKNOWN;
    }
}

