#ifndef __GLOBAL_INIT_H__
#define __GLOBAL_INIT_H__

#pragma once

#include <stdint.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Magic number representing the final, fully initialized state */
#define GLOBAL_INIT_LEVEL_DONE      (255U)

/* Global atomic variable tracking the initialization sequence */
extern atomic_uint_fast8_t GlobalInit_Level;

/**
 * @brief Retrieves the current global initialization level.
 * @return uint8_t Current initialization level.
 */
uint8_t GlobalInit_GetLevel(void);

/**
 * @brief Safely increments the global initialization level by 1.
 * @return uint8_t The new initialization level after incrementing.
 */
uint8_t GlobalInit_MoveNextLevel(void);

/**
 * @brief Marks the global initialization sequence as completely done (Level 255).
 * @return uint8_t The final done state (255).
 */
uint8_t GlobalInit_Done(void);

#ifdef __cplusplus
}
#endif

#endif /* __GLOBAL_INIT_H__ */