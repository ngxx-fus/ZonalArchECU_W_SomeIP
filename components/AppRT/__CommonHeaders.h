#ifndef __APP_RT_COMMON_HEADERS_H__

#include "../AppConfig/All.h"
#include "Routing/All.h"
#include "../AppBase/All.h"
#include "../AppESPWrap/All.h"

#ifndef E_APP_SERVICE_STATUS
    #define E_APP_SERVICE_STATUS
    typedef enum e_AppServiceStatus {
        eSERVICE_DISABLED           = 0,
        eSERVICE_ENABLED            = 1,
        eSERVICE_INITIALIZED        = 3,
    } AppServiceStatus_t;

#endif
    
#ifndef ST_APP_SERVICE
    #define ST_APP_SERVICE

    /// @brief Structure of service
    typedef struct AppService_t{
        char            Name[50];
        void            (*Service)(void *); /* Use proper function pointer type */
        int32_t         StackSize;
        void*           Param;
        int32_t         Priority;
        TaskHandle_t    Handle;
        AppServiceStatus_t Status;
    } AppService_t;

    extern AppService_t ServiceList[];

#endif

#ifndef P_SERVICE_INDEX_LIST
    #define P_SERVICE_INDEX_LIST
    extern int32_t ServiceIndexList[];
#endif 

#ifndef E_SERVICE_INDEX_LIST
    #define E_SERVICE_INDEX_LIST
	typedef enum e_ServiceIndexList {
		eSERVICE_MORTOR_RUNTIME         = 0,
		eSERVICE_ETHERNET_RUNTIME       = 1,
		eSERVICE_ULTRA_SONIC_RUNTIME    = 2,
		eSERVICE_HEART_BEAT_RUNTIME     = 3,
        eSERVICE_LOCATION_RUNTIME		= 4,
        eSERVICE_TEST_RUNTIME           = 5
	} ServiceIndexList_t;
#endif

#endif /**/
