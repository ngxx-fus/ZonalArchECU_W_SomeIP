#ifndef __APP_HEART_H__
#define __APP_HEART_H__

#include "../../AppUtils/All.h"
#include "../../AppConfig/All.h"
#include "../../AppESPWrap/All.h"

/// @brief Initialize and start the High-level Heart Task for ZECU
/// @details This task cyclically gathers data from ultrasonic and motor, 
/// formats it with E2E protection, and sends a UDP Sync frame to the CCU.
void HeartCtl(void* arg);

#endif /* __APP_HEART_H__ */

/* EOF *******************************************************************************************************************/