


/*Project pre-config*/
#include "../AppConfig/All.h"
#include "../AppUtils/All.h"
#include "../AppESPWrap/All.h"

/*Project modules*/
#include "../AppComm/EthernetW5500/Module.h"
#include "../AppComm/HBridge/Module.h"
#include "../AppComm/Dummy/Dummy.h"
#include "../AppComm/UltraSonic/Module.h"

#include "HeartBeat/Module.h"

/// @brief Structure of service
typedef struct AppService_t{
    char            Name[50];
    void            (*Service)(void *); /* Use proper function pointer type */
    int             StackSize;
    void*           Param;
    int             Priority;
    TaskHandle_t    Handle;
} AppService_t;

/// @brief List of registered system services with Task parameters
extern AppService_t ServiceList[];

/// @brief Initialize application
ReturnCode_t AppInit();