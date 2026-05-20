#ifndef __CONFIG_ALL_H__
#define __CONFIG_ALL_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1
 #define __ZECU_DISABLED__ 1
#endif

#include <stdint.h>
#include <stdlib.h>

#include "Comm/Comm.h"
#include "Pinout/Pinout.h"
#include "SystemLog.h"

#if 1 /*SAFETY SET-UP*/

    /// @brief Set-up of emergency trigger thresold distance list
    extern const uint16_t SF_ETT_Distance[];

#endif /**/

#ifdef __cplusplus
}
#endif

#endif /// __CONFIG_ALL_H__
