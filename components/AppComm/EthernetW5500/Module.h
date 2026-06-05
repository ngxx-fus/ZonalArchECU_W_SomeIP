#ifndef __ETHERNET_W5500_MODULE_H__
#define __ETHERNET_W5500_MODULE_H__

#define SOCKET_0_MACRAW_EN 0
#define SOCKET_1_UDP_EN    1

#include "../__CommonHeaders.h"

#include "EthernetW5500.h"
#include "PacketQueue.h"

extern SemaphoreHandle_t    EthLock;
extern GenericPayload_t     RxedPacket[];
extern TaskHandle_t         W5500CommCtl_TaskHandler;


/*Module public APIs*/
ReturnCode_t IPv4SetGateWayAddress(uint32_t IPv4Addr);
ReturnCode_t Eth_SetIPv4SubnetMask(uint32_t IPv4SubnetMask);
ReturnCode_t Eth_SetSrcIPv4Address(uint32_t IPv4Addr);
ReturnCode_t Eth_SetSrcIPv4Port(uint16_t IPv4Port);
ReturnCode_t EthSetIPv4SourceAddress(uint32_t IPv4Addr);

ReturnCode_t Eth_SetUDPBufferSize(uint8_t TxSizeKB, uint8_t RxSizeKB);

ReturnCode_t Eth_GetRxPacket(PacketSlot_t** pkt);
ReturnCode_t Eth_GetTxPacket(PacketSlot_t** pkt);
ReturnCode_t Eth_GetUDPPacket(PacketSlot_t** pkt);

ReturnCode_t Eth_GetUDPInfo(PacketSlot_t* pkt, uint8_t* srcIP, uint16_t* srcPort, uint8_t* srcMAC);
ReturnCode_t Eth_GetUDPPayload(PacketSlot_t* pkt, uint8_t** payload, uint16_t* size);

typedef void (*EthRxCallback_t)(PacketSlot_t* pkt);
ReturnCode_t Eth_SetRxCallback(EthRxCallback_t cb);
void Eth_WeakRxCallback(PacketSlot_t* pkt);

/* Logging utilities */
void W5500_LogFrame(GenericPtr_t Data, EthSize_t Size, GenericPtr_t SrcMAC, GenericPtr_t DstMAC);
void W5500_LogUDPInfo(PacketSlot_t* pkt);

ReturnCode_t Eth_SendUDPPacket(uint32_t IPv4Address, uint16_t IPv4Port, uint8_t* UDP_Payload, uint16_t PayloadSize);

/*Runtime service*/
void W5500CommRuntime(void* arg);

#endif ///__ETHERNET_W5500_MODULE_H__
