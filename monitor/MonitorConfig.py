import time

# Global configuration threshold
CFG_LOG_LEVEL = 1

# Severity level definitions
LOG_DEBUG = 1
LOG_INFO = 1
LOG_ERROR = 2

# /* ========================================================================= */
# /* SYSTEM LIMITS & BEHAVIOR CONFIGURATIONS                                   */
# /* ========================================================================= */
MAX_NUM_POINT = 50
SCALE_BASE_ON_LOCAL_MAX_VAL = 1 

MAX_MOTOR_SPEED_VAL = [100, 100, 100, 100]
MIN_MOTOR_SPEED_VAL = [-100, -100, -100, -100]

MAX_DISTANCE_VAL = [5.0, 5.0, 5.0, 5.0, 5.0, 5.0]
MIN_DISTANCE_VAL = [0.1, 0.1, 0.1, 0.1, 0.1, 0.1]

MAX_NUM_FRONT_ZECU = 1
MAX_NUM_BACK_ZECU = 1
ALLOW_AUTO_REPACE_ZECU = 1


"""
/*
 * @brief   Log a debug message.
 * @param   fmt The format string.
 * @param   args Variable arguments for the format string.
 * @return  None.
 */
"""
def SysLog(fmt, *args):
    # Verify if the debug severity meets the global configuration threshold
    if LOG_DEBUG >= CFG_LOG_LEVEL:
        # Format the message if arguments are provided
        if args:
            msg = fmt % args
        # Handle the case where no arguments are passed
        else:
            msg = fmt
            
        print(f"[{time.time():.3f}] [DEBUG]: {msg}")
        
    # Exit the function
    return

"""
/*
 * @brief   Log an informational message.
 * @param   fmt The format string.
 * @param   args Variable arguments for the format string.
 * @return  None.
 */
"""
def SysInfo(fmt, *args):
    # Verify if the info severity meets the global configuration threshold
    if LOG_INFO >= CFG_LOG_LEVEL:
        # Format the message if arguments are provided
        if args:
            msg = fmt % args
        # Handle the case where no arguments are passed
        else:
            msg = fmt
            
        print(f"[{time.time():.3f}] [INFO]: {msg}")
        
    # Exit the function
    return

"""
/*
 * @brief   Log an error message.
 * @param   fmt The format string.
 * @param   args Variable arguments for the format string.
 * @return  None.
 */
"""
def SysErr(fmt, *args):
    # Verify if the error severity meets the global configuration threshold
    if LOG_ERROR >= CFG_LOG_LEVEL:
        # Format the message if arguments are provided
        if args:
            msg = fmt % args
        # Handle the case where no arguments are passed
        else:
            msg = fmt
            
        print(f"[{time.time():.3f}] [ERROR]: {msg}")
        
    # Exit the function
    return

# Determine if the script is executed directly
if __name__ == "__main__":
    print("You are calling a config file!\n")
    
    # Execute standalone functions
    SysLog("This debug message is hidden. Value: %d", 10)
    SysInfo("System started successfully at port %d", 8080)
    SysErr("Failed to initialize module: %s", "W5500")