#ifndef __LOCATION_ATGM336H_H__
#define __LOCATION_ATGM336H_H__

#include "__CommonHeaders.h"

extern volatile uint32_t Loc_Long;
extern volatile uint32_t Loc_Lat;

ReturnCode_t Loc_ResetDevice();
ReturnCode_t Loc_ReadLocation();
ReturnCode_t Loc_Init();
void Loc_Runtime(void* arg);

#endif