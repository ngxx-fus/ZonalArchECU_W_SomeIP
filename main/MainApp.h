/*Project pre-config*/
#include "../AppConfig/All.h"
#include "../AppUtils/All.h"
#include "../AppESPWrap/All.h"

/*Project modules*/
#include "../AppComm/HBridge/Module.h"
#include "../AppComm/Dummy/Dummy.h"

/// @brief List of registered system services with Task parameters
struct AppService_t{
    char            Name[50];
    void            (*Service)(void *); /* Use proper function pointer type */
    int             StackSize;
    void*           Param;
    int             Priority;
    TaskHandle_t    Handle;
} ServiceList[] = {
    /*name*/        /*function ptr*/    /*Stack Size*//*ParamPtr*//*Priority*/    /*TaskHandle*/
    {"MotorCtl",    MotorCtl,           2048,         NULL,       eTask_RealTime, 0           },
    {"Chaos_Zero",  Dummy0,              2048,   NULL,   eTask_Normal,   0},
    {"Chaos_Max",   Dummy1,              2048,   NULL,   eTask_Low,      0},
    {"Chaos_Jitter",Dummy_Jitter,        2048,   NULL,   eTask_Background, 0},
};
/*Project app*/
void AppInit(void * param){
    SysEntry("AppInit");

    for(int i = 0; i < (sizeof(ServiceList)/sizeof(struct AppService_t)); i++){
        SysLog("[AppInit] Start service %s", ServiceList[i].Name);
        xTaskCreate(
            ServiceList[i].Service, 
            ServiceList[i].Name, 
            ServiceList[i].StackSize, 
            ServiceList[i].Param, 
            ServiceList[i].Priority, 
            &ServiceList[i].Handle
        );
    }

    SysExit("AppInit");
}
