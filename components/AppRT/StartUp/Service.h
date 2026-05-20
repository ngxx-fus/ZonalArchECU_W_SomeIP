


/*Project pre-config*/
#include "../__CommonHeaders.h"

/*Project modules*/
#include "../AppComm/SharedAPIs.h"

#include "HeartBeat/Module.h"

/// @brief Structure of service
typedef struct AppService_t{
    char            Name[50];
    void            (*Service)(void *); /* Use proper function pointer type */
    int32_t             StackSize;
    void*           Param;
    int32_t             Priority;
    TaskHandle_t    Handle;
} AppService_t;

/// @brief List of registered system services with Task parameters
extern AppService_t ServiceList[];

/// @brief Initialize application
ReturnCode_t AppInit();

