#ifndef __APP_HEART_H__
#define __APP_HEART_H__

#include "__CommonHeaders.h"

#ifndef HEART_TASK_CYCLE_MS
    /* Configure cycle time and routing table definitions */
    #define HEART_TASK_CYCLE_MS 300
#endif /*HEART_TASK_CYCLE_MS*/
/* Target CCU IP and Port configuration */
#ifndef CCU_IPV4_ADDR_STR
    #define CCU_IPV4_ADDR_STR
    #define CCU_IP_ADDR  IPv4ToUint32(10, 0, 0, 102)
#else
    #define CCU_IP_ADDR ConvertIPv4ByteArr2Uint32(CCU_IPV4_ADDR)
#endif /*CCU_IP_ADDR*/

#ifndef CCU_IPV4_PORT_STR
    #define CCU_IPV4_PORT_STR "30490"
    #define CCU_UDP_PORT 30490
#else
    #define CCU_UDP_PORT CCU_IPV4_PORT
#endif /*CCU_UDP_PORT*/

/// @brief Wake-up heartbeat thread
void HeartBeat_WakeUp();

/// @brief Initialize and start the High-level Heart Task for ZECU
/// @details This task cyclically gathers data from ultrasonic and motor, 
/// formats it with E2E protection, and sends a UDP Sync frame to the CCU.
void HeartBeatRuntime(void* arg);

#endif /* __APP_HEART_H__ */

/* EOF *******************************************************************************************************************/
