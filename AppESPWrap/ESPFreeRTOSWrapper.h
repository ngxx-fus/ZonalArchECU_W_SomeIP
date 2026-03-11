/// @file   espMultiTaskWrapper.h
/// @brief  Wrapper macros for FreeRTOS multitasking on ESP32 (Task, Mutex, Delay).

#ifndef __ESP_MULTITASK_WRAPPER__
#define __ESP_MULTITASK_WRAPPER__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PRINT_HEADER_COMPILE_MESSAGE
#pragma message ("AppESPWrap/ESPFreeRTOSWrapper.h")
#endif

#include "freertos/FreeRTOS.h"   /// Core FreeRTOS definitions
#include "freertos/task.h"       /// Task management
#include "freertos/semphr.h"     /// Semaphores and Mutexes

#ifndef EnterCriticalSection
    /// Enter critical section (disable interrupts) - Use with care!
    #define EnterCriticalSection(mutex_ptr)   taskENTER_CRITICAL(mutex_ptr)
#endif

#ifndef ExitCriticalSection
    /// Exit critical section (restore interrupts)
    #define ExitCriticalSection(mutex_ptr)    taskEXIT_CRITICAL(mutex_ptr)
#endif

/// Convert Milliseconds to Ticks (Auto handle portTICK_PERIOD_MS)
#define MsToTicks(ms)           (pdMS_TO_TICKS(ms))

/// Delay task for X milliseconds (Blocking)
#define DelayMs(ms)             vTaskDelay(MsToTicks(ms))

/// Yield CPU to other tasks of same priority
#define TaskYield()             taskYIELD()

/// Create task pinned to a specific Core (0 or 1)
/// @param func: Function pointer
/// @param name: String name
/// @param stack: Stack size (bytes)
/// @param param: Pointer to arguments
/// @param prio: Priority (0 is lowest)
/// @param handle: TaskHandle_t*
/// @param core: 0 or 1
#define CreateTaskPinned(func, name, stack, param, prio, handle, core) \
    xTaskCreatePinnedToCore(func, name, stack, param, prio, handle, core)

/// Create task specifically on CPU 0 (Protocol/System Core usually)
/// @param func: Function pointer
/// @param name: String name
/// @param stack: Stack size (bytes)
/// @param param: Pointer to arguments
/// @param prio: Priority (0 is lowest)
/// @param handle: TaskHandle_t*
#define CreateTaskCPU0(func, name, stack, param, prio, handle) \
    xTaskCreatePinnedToCore(func, name, stack, param, prio, handle, 0)

/// Create task specifically on CPU 1 (App/User Core usually)
/// @param func: Function pointer
/// @param name: String name
/// @param stack: Stack size (bytes)
/// @param param: Pointer to arguments
/// @param prio: Priority (0 is lowest)
/// @param handle: TaskHandle_t*
#define CreateTaskCPU1(func, name, stack, param, prio, handle) \
    xTaskCreatePinnedToCore(func, name, stack, param, prio, handle, 1)

/// Create task (No Core pinning - FreeRTOS decides)
/// @param func: Function pointer
/// @param name: String name
/// @param stack: Stack size (bytes)
/// @param param: Pointer to arguments
/// @param prio: Priority (0 is lowest)
/// @param handle: TaskHandle_t*
#define CreateTaskAny(func, name, stack, param, prio, handle) \
    xTaskCreate(func, name, stack, param, prio, handle)

/// Delete a task (NULL for current task)
#define DeleteTask(handle)      vTaskDelete(handle)

/// Wait for notification (Block indefinitely)
#define WaitForNotify(val_ptr)  ulTaskNotifyTake(pdTRUE, portMAX_DELAY)

/// Send notification to a task
#define NotifyTask(handle)      xTaskNotifyGive(handle)

/// Create a standard Mutex
#define CreateMutex()           xSemaphoreCreateMutex()

/// Lock Mutex (Block indefinitely until obtained)
#define MutexLock(mutex)        xSemaphoreTake(mutex, portMAX_DELAY)

/// Unlock Mutex
#define MutexUnlock(mutex)      xSemaphoreGive(mutex)

/// Lock Mutex with Timeout (ms) - Returns pdTRUE if successful
#define MutexLockTimeout(mutex, ms) xSemaphoreTake(mutex, MsToTicks(ms))

/// Delete Semaphore/Mutex
#define DeleteSem(sem)          vSemaphoreDelete(sem)


#ifdef __cplusplus
}
#endif

#endif // __ESP_MULTITASK_WRAPPER__