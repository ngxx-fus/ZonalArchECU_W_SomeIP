#include "Service.h"

#ifndef MIN_STACK_SIZE
    #define MIN_STACK_SIZE 512
#endif

#ifndef E_SERVICE_INDEX_LIST
    #define E_SERVICE_INDEX_LIST
	typedef enum eServiceIndexList{
		eSERVICE_MORTOR_RUNTIME = 0,
		eSERVICE_ETHERNET_RUNTIME = 1,
		eSERVICE_ULTRA_SONIC_RUNTIME = 2,
		eSERVICE_HEART_BEAT_RUNTIME = 3
	};
#endif

/// @brief List of services to be startup
AppService_t ServiceList[] = {
    /*name*/                /*function ptr*/        /*Stack Size*//*ParamPtr*//*Priority*/          /*TaskHandle*/		/*Status*/
    [eSERVICE_MORTOR_RUNTIME] =
	{"MotorRuntime",            MotorRuntime,               2048,           NULL,       eTask_RealTime,     0,			eSERVICE_ENABLED},
    [eSERVICE_ETHERNET_RUNTIME] =
    {"EthRuntime",              W5500CommRuntime,     		2048,           NULL,       eTask_Normal,       0,			eSERVICE_ENABLED},
    [eSERVICE_ULTRA_SONIC_RUNTIME] =
    {"HCSR04Runtime",           HCSR04Runtime,              2048,           NULL,       eTask_Normal,       0,			eSERVICE_DISABLED},
    [eSERVICE_HEART_BEAT_RUNTIME] =
    {"HeartBeatRuntime",        HeartBeatRuntime,           2048,           NULL,       eTask_Normal,       0,			eSERVICE_ENABLED},
    // {"Chaos_Zero",       Dummy0,                 2048,           NULL,       eTask_Normal,       0},
    // {"Chaos_Max",        Dummy1,                 2048,           NULL,       eTask_Low,          0},
    // {"Chaos_Jitter",     Dummy_Jitter,           2048,           NULL,       eTask_Background,   0},
};

/// @brief Initializes and creates tasks for all services defined in ServiceList
/// @return ReturnCode_t status of the initialization process, returns STAT_OKE on completion
ReturnCode_t AppInit() {
	/* Initialize and validate each service before task creation */
	SysEntry("AppInit");

	for (int32_t i = 0; i < (sizeof(ServiceList) / sizeof(ServiceList[0])); i++) {
		AppService_t *srv = &ServiceList[i];
		#if (ZECU_DISABLE_ALL_SERVICES == 0)
			SysLog("AppInit(...): Start service %s", srv->Name);

			if(eSERVICE_DISABLED == srv->Status) {
				SysLog("AppInit(...): Service %s is disabled by SW!", srv->Name);
				continue;
			}

			if (IsNull(srv->Service)) {
				SysErr("AppInit(...): Service %s has NULL function pointer!", srv->Name);
				continue;
			}
			else if (srv->StackSize < MIN_STACK_SIZE) {
				SysErr("AppInit(...): Service %s stack size (%d) is too small!", srv->Name, srv->StackSize);
				continue;
			}
			xTaskCreate(srv->Service, srv->Name, srv->StackSize, srv->Param, srv->Priority, &srv->Handle);
		#else
			SysLog("AppInit(...): Service \"%s\" is disabled due to `ZECU_DISABLE_ALL_SERVICES!=0` macro. Clear it to re-enable!", srv->Name);
		#endif /*iodefine"?*/
	}

	SysExit("AppInit");
	return STAT_OKE;
}
