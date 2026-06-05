#include "Service.h"

#ifndef MIN_STACK_SIZE
    #define MIN_STACK_SIZE 512
#endif

/// @brief List of services to be startup
AppService_t ServiceList[] = {
    /*name*/                /*function ptr*/        /*Stack Size*//*ParamPtr*//*Priority*/          /*TaskHandle*/
    {"MotorRuntime",            MotorRuntime,               2048,           NULL,       eTask_RealTime,     0},
    {"EthRuntime",              W5500CommRuntime,     		2048,           NULL,       eTask_Normal,       0},
    // {"HCSR04Runtime",           HCSR04Runtime,              2048,           NULL,       eTask_Normal,       0},
    {"HeartBeatRuntime",        HeartBeatRuntime,               2048,           NULL,       eTask_Normal,       0},
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
