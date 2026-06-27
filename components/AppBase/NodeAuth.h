#ifndef NODEAUTH_H
#define NODEAUTH_H

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "ReturnType.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Sets the core authentication key for the system.
 * Both ZECU and CCU must call this with the same secret key.
 */
void Auth_SetCoreKey(uint32_t key);

/*
 * Generates a 64-bit challenge (OTP) for a specific node.
 * Executed by ZECU when a CCU requests connection.
 */
DefaultRet_t Auth_GenerateChallenge(uint32_t target_id, uint32_t timestamp, uint64_t* out_challenge);

/*
 * Signs the received challenge using the core key to generate a response.
 * Executed by CCU to prove its identity.
 */
DefaultRet_t Auth_SignChallenge(uint64_t challenge, uint64_t* out_signature);

/*
 * Verifies if the received signature matches the expected result for the given challenge.
 * Executed by ZECU to authenticate the CCU.
 */
DefaultRet_t Auth_VerifySignature(uint64_t original_challenge, uint64_t received_signature);

#ifdef __cplusplus
}
#endif

#endif /* NODEAUTH_H */