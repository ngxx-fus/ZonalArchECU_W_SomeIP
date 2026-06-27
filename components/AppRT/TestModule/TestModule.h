#ifndef __TEST_MODULE_H__
#define __TEST_MODULE_H__

#include "../__CommonHeaders.h"

/**
 * @brief Initializes the TestModule hardware and software resources.
 * 
 * Validates and configures the following pins:
 * - PIN_TEST_TRIG: Configured as an input interrupt triggered on falling edge.
 * - PIN_TEST_ECHO: Configured as an output pin with an internal pull-up resistor.
 * 
 * Note: The default ISR routine is implemented internally within the module.
 * 
 * @return ReturnCode_t STAT_OKE on success, or an appropriate error code if validation fails.
 */
ReturnCode_t Test_Init();

/**
 * @brief Wakes up the test runtime task and generates an echo pulse.
 * 
 * If the runtime task is currently blocked or sleeping, this function issues a task notification 
 * to wake it up. It then pulls the PIN_TEST_ECHO line low for precisely 1ms before returning it 
 * to the high state.
 * 
 * Note: This function is invoked from the HeartBeat module upon receiving a KeepAlive packet.
 */
void Test_WakeUp();

/**
 * @brief Overrides the default ISR handler.
 * 
 * Currently unsupported in this module implementation.
 * 
 * @param ISRHandler Function pointer to the custom interrupt service routine.
 * @return ReturnCode_t Always returns STAT_ERR_UNSUPPORTED.
 */
ReturnCode_t Test_SetISRHandler(GenericPtr_t ISRHandler);

/**
 * @brief Main execution task for the TestModule.
 * 
 * Enters a blocked state (sleep) upon startup. Wakes up when a task notification is received 
 * (typically triggered by the PIN_TEST_TRIG interrupt) to dispatch a KeepAlive UDP frame 
 * via the Ethernet W5500 peripheral, similar to HeartBeat broadcast behavior.
 * 
 * @param args Pointer to task arguments (typically NULL).
 */
void Test_Runtime(void* args);

#endif
