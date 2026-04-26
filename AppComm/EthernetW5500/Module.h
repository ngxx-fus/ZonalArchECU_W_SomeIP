#ifndef __ETHERNET_W5500_MODULE_H__
#define __ETHERNET_W5500_MODULE_H__

#include "EthernetW5500.h"
#include "RxedPacketQueue.h"

extern SemaphoreHandle_t    EthLock;
extern GenericPayload_t     RxedPacket[];
extern TaskHandle_t         W5500CommCtl_TaskHandler;
/*Module public APIs*/


// ReturnCode_t EthSetIPv4AddressHost(uint32_t IPv4Addr);
// ReturnCode_t EthSetIPv4AddressGateWay(uint32_t IPv4Addr);
// ReturnCode_t EthSetIPv4SubnetMask(uint32_t IPv4SubnetMask);
// ReturnCode_t EthSetIPv4SourceAddress(uint32_t IPv4Addr);
// ReturnCode_t EthSetIPv4SourcePort(uint16_t IPv4Port);

/*Runtime service*/
void W5500CommCtl(void* arg);

#endif ///__ETHERNET_W5500_MODULE_H__